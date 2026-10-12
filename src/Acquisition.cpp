#include "bookmgmt/Acquisition.h"

#include <cstdint>
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <utility>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

const char* departmentName(Department d) {
    switch (d) {
        case Department::ComputerScience:
            return "Computer Science";
        case Department::Physics:
            return "Physics";
        case Department::ElectricalEngineering:
            return "Electrical Engineering";
    }

    return "Unknown";
}

AcquisitionManager::AcquisitionManager(Catalog& catalog, Budget& budget)
    : catalog_(catalog), budget_(budget) {}

void AcquisitionManager::addVendor(
    const std::string& resourceId,
    const std::string& vendor,
    Money unitPrice) {

    if (!catalog_.contains(resourceId))
        throw std::invalid_argument("resource not found: " + resourceId);

    if (vendor.empty())
        throw std::invalid_argument("vendor name must not be empty");

    if (unitPrice.isNegative())
        throw std::invalid_argument("vendor price must not be negative");

    if (unitPrice.currency() != catalog_.get(resourceId).unitPrice().currency())
        throw std::invalid_argument(
            "vendor price currency must match resource currency"
        );

    vendors_[resourceId].push_back(VendorOffer{vendor, unitPrice});
}

const VendorOffer* AcquisitionManager::cheapestVendor(
    const std::string& resourceId) const {

    auto it = vendors_.find(resourceId);

    if (it == vendors_.end() || it->second.empty())
        return nullptr;

    const VendorOffer* cheapest = &it->second.front();

    for (const auto& offer : it->second) {
        if (offer.unitPrice < cheapest->unitPrice)
            cheapest = &offer;
    }

    return cheapest;
}

Money AcquisitionManager::vendorCost(
    const Resource& r,
    const VendorOffer* vendor,
    int quantity) const {

    if (!vendor)
        return r.costFor(quantity);

    return r.costForWithUnitPrice(
        quantity,
        vendor->unitPrice
    );
}

void AcquisitionManager::setDepartmentBudget(
    Department d,
    Budget budget) {

    if (budget.total().currency() != budget_.total().currency())
        throw std::invalid_argument(
            "department budget currency must match acquisition budget currency"
        );

    departmentBudgets_.insert_or_assign(d, std::move(budget));
}

Budget& AcquisitionManager::departmentBudget(Department d) {
    auto it = departmentBudgets_.find(d);

    if (it == departmentBudgets_.end())
        throw std::invalid_argument("department budget not found");

    return it->second;
}

const Budget& AcquisitionManager::departmentBudget(Department d) const {
    auto it = departmentBudgets_.find(d);

    if (it == departmentBudgets_.end())
        throw std::invalid_argument("department budget not found");

    return it->second;
}

Money AcquisitionManager::quote(
    const std::string& id,
    int quantity) const {

    const Resource& r = catalog_.get(id);
    const VendorOffer* vendor = cheapestVendor(id);
    const Money preTax = vendorCost(r, vendor, quantity);

    return applyTax(r, preTax);
}

void AcquisitionManager::setTaxRates(
    int printPercent,
    int electronicPercent) {

    if (printPercent < 0 || electronicPercent < 0)
        throw std::invalid_argument("tax rate must not be negative");

    printTaxPercent_ = printPercent;
    electronicTaxPercent_ = electronicPercent;
}

Money AcquisitionManager::applyTax(
    const Resource& r,
    Money preTaxCost) const {

    const int rate =
        r.isDigital() ? electronicTaxPercent_ : printTaxPercent_;

    const std::int64_t tax =
        (preTaxCost.minorUnits() * rate) / 100;

    return Money::fromMinor(
        preTaxCost.minorUnits() + tax,
        preTaxCost.currency()
    );
}

bool AcquisitionManager::canPurchase(
    const std::string& id,
    int quantity,
    std::string* reason) const {

    return canPurchase(
        id,
        quantity,
        Department::ComputerScience,
        reason
    );
}

bool AcquisitionManager::canPurchase(
    const std::string& id,
    int quantity,
    Department department,
    std::string* reason) const {

    std::string why;

    const Resource* r = catalog_.find(id);

    if (!r) {
        why = "resource not found: " + id;
    } else if (quantity <= 0) {
        why = "quantity must be positive";
    } else {
        const VendorOffer* vendor = cheapestVendor(id);
        const Money preTax = vendorCost(*r, vendor, quantity);
        const Money postTax = applyTax(*r, preTax);

        auto deptIt = departmentBudgets_.find(department);

        if (deptIt != departmentBudgets_.end()) {
            why = deptIt->second.check(
                r->category(),
                quantity,
                postTax,
                id
            );
        } else {
            why = budget_.check(
                r->category(),
                quantity,
                postTax,
                id
            );
        }
    }

    if (reason)
        *reason = why;

    return why.empty();
}

PurchaseRecord& AcquisitionManager::record(
    const Resource* r,
    const std::string& id,
    int qty,
    Money preTaxCost,
    Money postTaxCost,
    bool approved,
    std::string reason,
    Department department) {

    const VendorOffer* offer =
        r ? cheapestVendor(id) : nullptr;

    history_.push_back(PurchaseRecord{
        nextOrderNo_++,
        id,
        r ? r->title() : std::string("(unknown)"),
        r ? r->category() : ResourceCategory::Book,
        qty,
        preTaxCost,
        postTaxCost,
        approved,
        std::move(reason),
        false,
        0,
        department,
        offer ? offer->vendor : std::string("")
    });

    return history_.back();
}

const PurchaseRecord& AcquisitionManager::purchase(
    const std::string& id,
    int quantity,
    Department department) {

    const Resource& r = catalog_.get(id);

    const VendorOffer* vendor = cheapestVendor(id);
    const Money preTaxCost = vendorCost(r, vendor, quantity);
    const Money postTaxCost = applyTax(r, preTaxCost);

    auto deptIt = departmentBudgets_.find(department);

    if (deptIt != departmentBudgets_.end()) {
        deptIt->second.commit(
            r.category(),
            quantity,
            postTaxCost,
            id
        );
    } else {
        budget_.commit(
            r.category(),
            quantity,
            postTaxCost,
            id
        );
    }

    catalog_.addHoldings(id, quantity);

    return record(
        &r,
        id,
        quantity,
        preTaxCost,
        postTaxCost,
        true,
        "",
        department
    );
}

const PurchaseRecord& AcquisitionManager::cancel(int orderNo) {
    const PurchaseRecord* original = nullptr;

    for (const auto& rec : history_) {
        if (rec.orderNo == orderNo) {
            original = &rec;
            break;
        }
    }

    if (!original)
        throw std::invalid_argument("order not found");

    if (!original->approved || original->cancellation)
        throw std::invalid_argument("order cannot be cancelled");

    for (const auto& rec : history_) {
        if (rec.cancellation && rec.originalOrderNo == orderNo)
            throw std::invalid_argument("order already cancelled");
    }

    const std::string resourceId = original->resourceId;
    const std::string title = original->title;
    const ResourceCategory category = original->category;
    const int quantity = original->quantity;
    const Money preTaxCost = original->preTaxCost;
    const Money postTaxCost = original->cost;
    const Department department = original->department;
    const std::string vendor = original->vendor;

    auto deptIt = departmentBudgets_.find(department);

    if (deptIt != departmentBudgets_.end()) {
        deptIt->second.refund(
            category,
            quantity,
            postTaxCost,
            resourceId
        );
    } else {
        budget_.refund(
            category,
            quantity,
            postTaxCost,
            resourceId
        );
    }

    catalog_.addHoldings(resourceId, -quantity);

    history_.push_back(PurchaseRecord{
        nextOrderNo_++,
        resourceId,
        title,
        category,
        quantity,
        preTaxCost,
        postTaxCost,
        true,
        "",
        true,
        orderNo,
        department,
        vendor
    });

    return history_.back();
}

std::vector<PurchaseRecord> AcquisitionManager::processBatch(
    const std::vector<PurchaseRequest>& reqs,
    bool allOrNothing) {

    std::vector<PurchaseRecord> results;
    results.reserve(reqs.size());

    if (!allOrNothing) {
        for (const auto& req : reqs) {
            const Resource* r = catalog_.find(req.resourceId);

            Money preTaxCost;
            Money postTaxCost;
            std::string why;

            if (!r) {
                why = "resource not found: " + req.resourceId;
            } else if (req.quantity <= 0) {
                why = "quantity must be positive";
            } else {
                const VendorOffer* vendor =
                    cheapestVendor(req.resourceId);

                preTaxCost =
                    vendorCost(*r, vendor, req.quantity);

                postTaxCost =
                    applyTax(*r, preTaxCost);

                auto deptIt =
                    departmentBudgets_.find(req.department);

                if (deptIt != departmentBudgets_.end()) {
                    why = deptIt->second.check(
                        r->category(),
                        req.quantity,
                        postTaxCost,
                        req.resourceId
                    );
                } else {
                    why = budget_.check(
                        r->category(),
                        req.quantity,
                        postTaxCost,
                        req.resourceId
                    );
                }
            }

            if (why.empty()) {
                results.push_back(
                    purchase(
                        req.resourceId,
                        req.quantity,
                        req.department
                    )
                );
            } else {
                results.push_back(
                    record(
                        r,
                        req.resourceId,
                        req.quantity,
                        preTaxCost,
                        postTaxCost,
                        false,
                        why,
                        req.department
                    )
                );
            }
        }

        return results;
    }

    Budget tempFallback = budget_;
    auto tempDepartments = departmentBudgets_;
    std::string batchFailure;

    for (const auto& req : reqs) {
        const Resource* r = catalog_.find(req.resourceId);

        if (!r) {
            batchFailure =
                "resource not found: " + req.resourceId;
            break;
        }

        if (req.quantity <= 0) {
            batchFailure = "quantity must be positive";
            break;
        }

        const VendorOffer* vendor =
            cheapestVendor(req.resourceId);

        const Money preTaxCost =
            vendorCost(*r, vendor, req.quantity);

        const Money postTaxCost =
            applyTax(*r, preTaxCost);

        try {
            auto deptIt =
                tempDepartments.find(req.department);

            if (deptIt != tempDepartments.end()) {
                deptIt->second.commit(
                    r->category(),
                    req.quantity,
                    postTaxCost,
                    req.resourceId
                );
            } else {
                tempFallback.commit(
                    r->category(),
                    req.quantity,
                    postTaxCost,
                    req.resourceId
                );
            }
        }
        catch (const std::exception& e) {
            batchFailure = e.what();
            break;
        }
    }

    if (!batchFailure.empty()) {
        for (const auto& req : reqs) {
            const Resource* r =
                catalog_.find(req.resourceId);

            Money preTaxCost;
            Money postTaxCost;

            if (r && req.quantity > 0) {
                const VendorOffer* vendor =
                    cheapestVendor(req.resourceId);

                preTaxCost =
                    vendorCost(*r, vendor, req.quantity);

                postTaxCost =
                    applyTax(*r, preTaxCost);
            }

            results.push_back(
                record(
                    r,
                    req.resourceId,
                    req.quantity,
                    preTaxCost,
                    postTaxCost,
                    false,
                    "batch rejected: " + batchFailure,
                    req.department
                )
            );
        }

        return results;
    }

    for (const auto& req : reqs) {
        results.push_back(
            purchase(
                req.resourceId,
                req.quantity,
                req.department
            )
        );
    }

    return results;
}

Money AcquisitionManager::totalSpent() const {
    Money sum = Money::fromMinor(0, budget_.total().currency());

    for (const auto& rec : history_) {
        if (!rec.approved)
            continue;

        if (rec.cancellation)
            sum -= rec.cost;
        else
            sum += rec.cost;
    }

    return sum;
}

void AcquisitionManager::printReport(std::ostream& os) const {
    os << "Order history (" << history_.size() << " orders)\n";

    Money totalPreTax =
        Money::fromMinor(0, budget_.total().currency());
    Money totalPostTax =
        Money::fromMinor(0, budget_.total().currency());

    for (const auto& rec : history_) {
        os << "  #"
           << std::setw(3)
           << std::left
           << rec.orderNo
           << " "
           << (
                rec.cancellation
                    ? "CANCELLED"
                    : (rec.approved ? "APPROVED" : "REJECTED")
              )
           << "  "
           << std::setw(6)
           << rec.resourceId
           << " x"
           << std::setw(3)
           << rec.quantity
           << " pre-tax "
           << std::setw(10)
           << rec.preTaxCost.toString()
           << " post-tax "
           << std::setw(10)
           << rec.cost.toString()
           << "  "
           << rec.title
           << "  dept: "
           << departmentName(rec.department);

        if (!rec.vendor.empty()) {
            os << "  vendor: " << rec.vendor;
        }

        if (!rec.approved)
            os << "\n        reason: " << rec.reason;

        os << "\n";

        if (rec.approved) {
            if (rec.cancellation) {
                totalPreTax -= rec.preTaxCost;
                totalPostTax -= rec.cost;
            } else {
                totalPreTax += rec.preTaxCost;
                totalPostTax += rec.cost;
            }
        }
    }

    os << "Total pre-tax: " << totalPreTax << "\n";
    os << "Total post-tax: " << totalPostTax << "\n";
}

}  // namespace bookmgmt
