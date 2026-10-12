// Demo: builds a catalog, configures budgets/quotas/taxes,
// and demonstrates acquisition features.

#include <iostream>
#include <vector>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

int main() {
    Catalog catalog;

    catalog.emplace<Book>(
        "B001",
        "Clean Code",
        std::vector<std::string>{"Robert C. Martin"},
        "978-0132350884",
        "Prentice Hall",
        2008,
        Money::of(450)
    );

    catalog.emplace<Book>(
        "B002",
        "The C++ Programming Language",
        std::vector<std::string>{"Bjarne Stroustrup"},
        "978-0321563842",
        "Addison-Wesley",
        2013,
        Money::of(1200),
        4,
        Binding::Hardcover
    );

    catalog.emplace<ElectronicResource>(
        "R001",
        "IEEE Xplore Digital Library",
        "IEEE",
        2026,
        Money::of(150),
        "https://ieeexplore.example",
        LicenseModel::AnnualSubscription,
        Money::of(2000)
    );

    catalog.emplace<ElectronicResource>(
        "R002",
        "MATLAB Campus Licence",
        "MathWorks",
        2026,
        Money::of(400),
        "https://licensing.example/matlab",
        LicenseModel::Perpetual
    );

    catalog.emplace<Journal>(
        "J001",
        "Nature",
        "0028-0836",
        52,
        "Springer Nature",
        2026,
        Money::of(500),
        2
    );

    catalog.emplace<EBook>(
        "E001",
        "Effective Modern C++",
        std::vector<std::string>{"Scott Meyers"},
        "978-1491903995",
        "O'Reilly",
        2014,
        Money::of(20),
        "https://example.com/effective-modern-cpp",
        FileFormat::EPUB,
        true,
        LicenseModel::AnnualSubscription,
        Money::of(100)
    );

    catalog.emplace<AudioBook>(
        "A001",
        "1984 Audiobook",
        "Penguin",
        2020,
        Money::of(30),
        "https://example.com/1984-audio",
        "Simon Prebble",
        660,
        LicenseModel::AnnualSubscription,
        Money::of(100)
    );

    catalog.emplace<Thesis>(
        "T001",
        "Low Power VLSI Design",
        "IIIT Delhi",
        "M.Tech",
        "Dr. Sharma",
        "IIIT Delhi",
        2026
    );

    catalog.emplace<Book>(
        "BHC",
        "Hardcover Design Book",
        std::vector<std::string>{"Example Author"},
        "978-hardcover",
        "Example Publisher",
        2026,
        Money::of(500),
        1,
        Binding::Hardcover
    );

    std::cout << "=== Catalog ===\n";

    for (const Resource* r : catalog.all())
        std::cout << r->summary() << "\n";

    std::cout
        << "\n=== Details of R001 ===\n"
        << catalog.get("R001");

    // Fallback/global budget
    Budget budget(Money::of(20000));

    budget.setQuota(
        ResourceCategory::Book,
        {10, Money::of(8000), 3}
    );

    budget.setQuota(
        ResourceCategory::Journal,
        {5, Money::of(5000), 2}
    );

    budget.setQuota(
        ResourceCategory::ElectronicResource,
        {40, Money::of(12000), 3}
    );

    budget.setQuota(
        ResourceCategory::EBook,
        {20, Money::of(3000), 2}
    );

    budget.setQuota(
        ResourceCategory::Thesis,
        {10, Money::of(1000), 2}
    );

    AcquisitionManager acq(catalog, budget);

    // Department budgets
    Budget csBudget(Money::of(10000));

    csBudget.setQuota(
        ResourceCategory::Book,
        {10, Money::of(5000), 3}
    );

    csBudget.setQuota(
        ResourceCategory::Journal,
        {5, Money::of(3000), 2}
    );

    csBudget.setQuota(
        ResourceCategory::ElectronicResource,
        {20, Money::of(7000), 3}
    );

    Budget physicsBudget(Money::of(5000));

    physicsBudget.setQuota(
        ResourceCategory::Book,
        {5, Money::of(3000), 2}
    );

    acq.setDepartmentBudget(
        Department::ComputerScience,
        csBudget
    );

    acq.setDepartmentBudget(
        Department::Physics,
        physicsBudget
    );

    // Print = 5%, electronic = 18%
    acq.setTaxRates(5, 18);

            acq.addVendor(
            "B001",
            "University Books",
            Money::of(430)
        );

        acq.addVendor(
            "B001",
            "Campus Store",
            Money::of(400)
        );

        acq.addVendor(
            "B001",
            "Academic Supplier",
            Money::of(420)
        );
    // Cancellation demo
    const auto& cancelDemoOrder =
        acq.purchase(
            "BHC",
            1,
            Department::ComputerScience
        );

    std::cout
        << "\nCancelling order #"
        << cancelDemoOrder.orderNo
        << "\n";

    acq.cancel(cancelDemoOrder.orderNo);

    // Quotes
    std::cout << "\n=== Quotes ===\n";

    std::cout
        << "5 seats of E001 = "
        << acq.quote("E001", 5)
        << "\n";

    std::cout
        << "5 copies of B002 = "
        << acq.quote("B002", 5)
        << "\n";

    std::cout
        << "20 seats of R001 = "
        << acq.quote("R001", 20)
        << "\n";

    std::cout
        << "2 seats of A001 = "
        << acq.quote("A001", 2)
        << "\n";

    std::cout
        << "1 copy of T001 = "
        << acq.quote("T001", 1)
        << "\n";

    std::cout
        << "2 hardcover copies of BHC = "
        << acq.quote("BHC", 2)
        << "\n";

    std::cout
        << "10 copies of B001 with bulk discount = "
        << acq.quote("B001", 10)
        << "\n";

    std::cout
        << "60 seats of R001 with seat discount = "
        << acq.quote("R001", 60)
        << "\n";

    // Normal batch processing
    acq.processBatch({
        {"B001", 4, Department::ComputerScience},
        {"B002", 5, Department::ComputerScience},
        {"B001", 1, Department::ComputerScience},
        {"R001", 20, Department::ComputerScience},
        {"R002", 25, Department::ComputerScience},
        {"R002", 15, Department::ComputerScience},
        {"R002", 5, Department::ComputerScience},
        {"X999", 1, Department::ComputerScience},
    });

    // All-or-nothing batch
    std::cout << "\n=== All-or-Nothing Batch ===\n";

    std::vector<PurchaseRequest> atomicBatch = {
        {"J001", 1, Department::ComputerScience},
        {"B002", 10, Department::ComputerScience}
    };

    auto atomicResults =
        acq.processBatch(atomicBatch, true);

    for (const auto& r : atomicResults) {
        std::cout
            << r.resourceId
            << ": "
            << (r.approved ? "approved" : "rejected")
            << "\n";
    }

     std::cout << "\n=== Catalog Searches ===\n";

    auto authorResults =
        catalog.searchAuthor("Martin");

    std::cout
        << "Author search (Martin): "
        << authorResults.size()
        << "\n";

    auto idResults =
        catalog.searchIdentifier(
            "978-0132350884"
        );

    std::cout
        << "ISBN search: "
        << idResults.size()
        << "\n";

    auto yearResults =
        catalog.searchYearRange(
            2008,
            2014
        );

    std::cout
        << "Year range 2008-2014: "
        << yearResults.size()
        << "\n";

    std::cout << "\n=== Acquisition Report ===\n";

    acq.printReport(std::cout);

    std::cout << "\n=== Global Budget ===\n";

    budget.print(std::cout);

    std::cout << "\n=== Computer Science Budget ===\n";

    acq.departmentBudget(
        Department::ComputerScience
    ).print(std::cout);

    std::cout << "\n=== Physics Budget ===\n";

    acq.departmentBudget(
        Department::Physics
    ).print(std::cout);

    // Year-end rollover
    std::cout
        << "\n=== Next Year Computer Science Budget ===\n";

    Budget nextYearBudget =
        acq.departmentBudget(
            Department::ComputerScience
        ).rollover(50);

    nextYearBudget.print(std::cout);

    // Holdings
    std::cout << "\n=== Holdings ===\n";

    for (const Resource* r : catalog.all()) {
        std::cout
            << "  "
            << r->id()
            << ": "
            << catalog.holdings(r->id())
            << (r->isDigital() ? " seats" : " copies")
            << "\n";
    }

    // Exception demo
    std::cout
        << "\n=== Direct purchase that breaks a quota ===\n";

    try {
        acq.purchase(
            "B002",
            10,
            Department::Physics
        );
    }
    catch (const QuotaExceededError& e) {
        std::cout
            << "QuotaExceededError: "
            << e.what()
            << "\n";
    }

    return 0;
}