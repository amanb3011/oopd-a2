#pragma once
// AcquisitionManager: turns purchase requests into orders, enforcing the
// Budget's quotas, updating Catalog holdings and keeping an order history.

#include <iosfwd>
#include <string>
#include <vector>
#include <map>

#include "bookmgmt/Budget.h"
#include "bookmgmt/Catalog.h"

namespace bookmgmt {

enum class Department {
    ComputerScience,
    Physics,
    ElectricalEngineering
};

const char* departmentName(Department d);

struct PurchaseRequest {
    std::string resourceId;
    int quantity;
    Department department = Department::ComputerScience;
};

struct PurchaseRecord {
    int orderNo;
    std::string resourceId;
    std::string title;
    ResourceCategory category;
    int quantity;
    Money preTaxCost;
    Money cost;
    bool approved;
    std::string reason;

    bool cancellation = false;
    int originalOrderNo = 0;
    Department department = Department::ComputerScience;
};
class AcquisitionManager {
public:
    AcquisitionManager(Catalog& catalog, Budget& budget);

    void setTaxRates(int printPercent, int electronicPercent);

    int printTaxRate() const { return printTaxPercent_; }
    int electronicTaxRate() const { return electronicTaxPercent_; }

    // Price of a request without buying anything. Throws NotFoundError.
    Money quote(const std::string& id, int quantity) const;

    // True if the purchase would be approved; if not, `reason` explains why.
    bool canPurchase(
    const std::string& id,
    int quantity,
    std::string* reason
    ) const;

    bool canPurchase(
    const std::string& id,
    int quantity,
    Department department = Department::ComputerScience,
    std::string* reason = nullptr
    ) const;

    const PurchaseRecord& cancel(int orderNo);
    
    const PurchaseRecord& purchase(
    const std::string& id,
    int quantity,
    Department department = Department::ComputerScience
);

    // Buys immediately. Throws NotFoundError, QuotaExceededError,
    // BudgetExceededError or std::invalid_argument. On success the budget
    // and holdings are updated and the record is added to history.
    PurchaseRecord& record(
    const Resource* r,
    const std::string& id,
    int qty,
    Money preTaxCost,
    Money postTaxCost,
    bool approved,
    std::string reason,
    Department department = Department::ComputerScience
);
    // Processes requests in order; each is approved or rejected on its own
    // (never throws for a rejected request). Every outcome is recorded.
    // EXTENSION POINT: priority ordering, all-or-nothing batches, ...
    std::vector<PurchaseRecord> processBatch(
        const std::vector<PurchaseRequest>& reqs,
        bool allOrNothing = false
    );

    const std::vector<PurchaseRecord>& history() const { return history_; }
    Money totalSpent() const;

    void printReport(std::ostream& os) const;
    void setDepartmentBudget(Department d, Budget budget);

    Budget& departmentBudget(Department d);
    const Budget& departmentBudget(Department d) const;

private:
    PurchaseRecord& record(const Resource* r, const std::string& id, int qty,
                           Money cost, bool approved, std::string reason);

    Catalog& catalog_;
    Money applyTax(const Resource& r, Money preTaxCost) const;
    Budget& budget_;
    std::vector<PurchaseRecord> history_;
    std::map<Department, Budget> departmentBudgets_;
    int nextOrderNo_ = 1;
    int printTaxPercent_ = 0;
    int electronicTaxPercent_ = 0;
};

}  // namespace bookmgmt
