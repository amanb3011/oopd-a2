#include "bookmgmt/Acquisition.h"

#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <cstdint>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

AcquisitionManager::AcquisitionManager(Catalog& catalog, Budget& budget)
    : catalog_(catalog), budget_(budget) {}

Money AcquisitionManager::quote(const std::string& id, int quantity) const {
    const Resource& r = catalog_.get(id);
    const Money preTax = r.costFor(quantity);
    return applyTax(r, preTax);
}

void AcquisitionManager::setTaxRates(int printPercent, int electronicPercent) {
    if (printPercent < 0 || electronicPercent < 0)
        throw std::invalid_argument("tax rate must not be negative");

    printTaxPercent_ = printPercent;
    electronicTaxPercent_ = electronicPercent;
}

Money AcquisitionManager::applyTax(const Resource& r, Money preTaxCost) const {
    const int rate = r.isDigital()
        ? electronicTaxPercent_
        : printTaxPercent_;

    const std::int64_t tax =
        (preTaxCost.minorUnits() * rate) / 100;

    return Money::fromMinor(preTaxCost.minorUnits() + tax);
}

bool AcquisitionManager::canPurchase(const std::string& id, int quantity,
                                     std::string* reason) const {
    std::string why;

    if (const Resource* r = catalog_.find(id)) {
        if (quantity <= 0) {
            why = "quantity must be positive";
        } else {
            const Money preTax = r->costFor(quantity);
            const Money postTax = applyTax(*r, preTax);

            why = budget_.check(
                r->category(),
                quantity,
                postTax,
                id
            );
        }
    } else {
        why = "resource not found: " + id;
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
    std::string reason) {

    history_.push_back(PurchaseRecord{
        nextOrderNo_++,
        id,
        r ? r->title() : std::string("(unknown)"),
        r ? r->category() : ResourceCategory::Book,
        qty,
        preTaxCost,
        postTaxCost,
        approved,
        std::move(reason)
    });

    return history_.back();
}

const PurchaseRecord& AcquisitionManager::purchase(
    const std::string& id,
    int quantity) {

    const Resource& r = catalog_.get(id);

    const Money preTaxCost = r.costFor(quantity);
    const Money postTaxCost = applyTax(r, preTaxCost);

    budget_.commit(
    r.category(),
    quantity,
    postTaxCost,
    id
    )  ;

    catalog_.addHoldings(id, quantity);

    return record(
        &r,
        id,
        quantity,
        preTaxCost,
        postTaxCost,
        true,
        {}
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

    budget_.refund(
        category,
        quantity,
        postTaxCost,
        resourceId
    );

    catalog_.addHoldings(
        resourceId,
        -quantity
    );

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
        orderNo
    });

    return history_.back();
}

std::vector<PurchaseRecord> AcquisitionManager::processBatch(
    const std::vector<PurchaseRequest>& reqs) {

    std::vector<PurchaseRecord> results;
    results.reserve(reqs.size());

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
            preTaxCost = r->costFor(req.quantity);
            postTaxCost = applyTax(*r, preTaxCost);

            why = budget_.check(
            r->category(),
            req.quantity,
            postTaxCost,
            req.resourceId
        );
        }

        if (why.empty()) {
            results.push_back(
                purchase(req.resourceId, req.quantity)
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
                    why
                )
            );
        }
    }

    return results;
}

Money AcquisitionManager::totalSpent() const {
    Money sum;

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

    Money totalPreTax;
    Money totalPostTax;

    for (const auto& rec : history_) {
        os << "  #" << std::setw(3) << std::left << rec.orderNo << " "
           << (rec.cancellation
        ? "CANCELLED"
        : (rec.approved ? "APPROVED" : "REJECTED")) << "  "
           << std::setw(6) << rec.resourceId
           << " x" << std::setw(3) << rec.quantity
           << " pre-tax " << std::setw(10)
           << rec.preTaxCost.toString()
           << " post-tax " << std::setw(10)
           << rec.cost.toString()
           << "  " << rec.title;

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
