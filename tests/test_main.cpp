// Minimal self-contained test runner (no external framework needed).

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        ++g_checks;                                                              \
        if (!(cond)) {                                                           \
            ++g_failures;                                                        \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #cond \
                      << "\n";                                                   \
        }                                                                        \
    } while (0)

#define CHECK_THROWS(expr, ExType)            \
    do {                                      \
        bool thrown_ = false;                 \
        try {                                 \
            (void)(expr);                     \
        } catch (const ExType&) {             \
            thrown_ = true;                   \
        } catch (...) {                       \
        }                                     \
        CHECK(thrown_ && "expected " #ExType); \
    } while (0)

static void testMoney() {
    CHECK(Money::of(12, 5).toString() == "12.05");
    CHECK(Money::of(-3, 50).toString() == "-3.50");
    CHECK(Money::fromMinor(7).toString() == "0.07");
    CHECK(Money::of(10) + Money::of(0, 50) == Money::fromMinor(1050));
    CHECK(Money::of(3) * 4 == Money::of(12));
    CHECK(Money::of(1) < Money::of(2));
    CHECK_THROWS(Money::of(1, 100), std::invalid_argument);
}

static void testResourcesAndCost() {
    Book b("B1", "T", {"A", "B", "C"}, "isbn", "P", 2020, Money::of(100));
    CHECK(b.category() == ResourceCategory::Book);
    CHECK(!b.isDigital());
    CHECK(b.costFor(3) == Money::of(300));
    CHECK_THROWS(b.costFor(0), std::invalid_argument);
    CHECK(joinAuthors(b.authors()) == "A, B and C");

    Book paperback(
    "BP1",
    "Paperback Test",
    {"Author"},
    "isbn-p",
    "Publisher",
    2026,
    Money::of(100),
    1,
    Binding::Paperback
);

Book hardcover(
    "BH1",
    "Hardcover Test",
    {"Author"},
    "isbn-h",
    "Publisher",
    2026,
    Money::of(100),
    1,
    Binding::Hardcover
);

CHECK(paperback.costFor(2) == Money::of(200));
CHECK(hardcover.costFor(2) == Money::of(240));

    ElectronicResource e("R1", "DB", "P", 2026, Money::of(10), "url",
                         LicenseModel::AnnualSubscription, Money::of(100));
    CHECK(e.isDigital());
    CHECK(e.costFor(5) == Money::of(150));

    CHECK(e.category() == ResourceCategory::ElectronicResource);

    // Polymorphism through a base-class reference
    const Resource& r = e;
    CHECK(r.costFor(1) == Money::of(110));
    std::ostringstream os;
    os << r;
    CHECK(os.str().find("platform fee: 100.00") != std::string::npos);

    CHECK_THROWS(Book("", "T", {}, "", "", 2000, Money::of(1)), std::invalid_argument);
    CHECK_THROWS(Book("B", "T", {}, "", "", 2000, Money::fromMinor(-1)),
                 std::invalid_argument);

    Journal j("J1", "Nature", "0028-0836", 52,
          "Springer Nature", 2026, Money::of(500), 2);

    CHECK(j.category() == ResourceCategory::Journal);
    CHECK(!j.isDigital());
    CHECK(j.issn() == "0028-0836");
    CHECK(j.issuesPerYear() == 52);
    CHECK(j.subscriptionYears() == 2);
    CHECK(j.costFor(3) == Money::of(3000));

    CHECK_THROWS(
        Journal("J2", "Test Journal", "1234-5678", 12,
                "Publisher", 2026, Money::of(100), 0),
        std::invalid_argument
    );    
    
    std::ostringstream journalOut;
    journalOut << j;

    CHECK(journalOut.str().find("issn: 0028-0836") != std::string::npos);
    CHECK(journalOut.str().find("issues per year: 52") != std::string::npos);
    CHECK(journalOut.str().find("subscription years: 2") != std::string::npos);

        EBook eb(
        "E1",
        "Effective Modern C++",
        {"Scott Meyers"},
        "978-1491903995",
        "O'Reilly",
        2014,
        Money::of(20),
        "https://example.com/ebook",
        FileFormat::EPUB,
        true,
        LicenseModel::AnnualSubscription,
        Money::of(100)
    );

    CHECK(eb.category() == ResourceCategory::EBook);
    CHECK(eb.isDigital());
    CHECK(eb.authors().size() == 1);
    CHECK(eb.isbn() == "978-1491903995");
    CHECK(eb.format() == FileFormat::EPUB);
    CHECK(eb.drmProtected());

    CHECK(eb.costFor(5) == Money::of(200));

    std::ostringstream ebookOut;
    ebookOut << eb;

    CHECK(ebookOut.str().find("access url: https://example.com/ebook")
        != std::string::npos);

    CHECK(ebookOut.str().find("authors: Scott Meyers")
        != std::string::npos);

    CHECK(ebookOut.str().find("isbn: 978-1491903995")
        != std::string::npos);

    CHECK(ebookOut.str().find("file format: EPUB")
        != std::string::npos);

    CHECK(ebookOut.str().find("drm protected: yes")
        != std::string::npos);

        AudioBook ab(
    "A1",
    "1984 Audiobook",
    "Penguin",
    2020,
    Money::of(30),
    "https://example.com/1984",
    "Simon Prebble",
    660,
    LicenseModel::AnnualSubscription,
    Money::of(100)
);

    CHECK(ab.category() == ResourceCategory::ElectronicResource);
    CHECK(ab.isDigital());
    CHECK(ab.narrator() == "Simon Prebble");
    CHECK(ab.durationMinutes() == 660);

    // ElectronicResource pricing is inherited:
    // 100 platform fee + 30 * 2 seats = 160
    CHECK(ab.costFor(2) == Money::of(160));

    std::ostringstream audioOut;
    audioOut << ab;

    CHECK(audioOut.str().find("narrator: Simon Prebble") != std::string::npos);
    CHECK(audioOut.str().find("duration minutes: 660") != std::string::npos);

    Thesis thesis(
        "T1",
        "Low Power VLSI Design",
        "IIIT Delhi",
        "M.Tech",
        "Dr. Sharma",
        "IIIT Delhi",
        2026
    );

    CHECK(thesis.category() == ResourceCategory::Thesis);
    CHECK(!thesis.isDigital());
    CHECK(thesis.university() == "IIIT Delhi");
    CHECK(thesis.degree() == "M.Tech");
    CHECK(thesis.supervisor() == "Dr. Sharma");

    // default unit price is zero
    CHECK(thesis.costFor(1) == Money::of(0));

    std::ostringstream thesisOut;
    thesisOut << thesis;

    CHECK(thesisOut.str().find("university: IIIT Delhi") != std::string::npos);
    CHECK(thesisOut.str().find("degree: M.Tech") != std::string::npos);
    CHECK(thesisOut.str().find("supervisor: Dr. Sharma") != std::string::npos);

    Book bulkPaperback(
    "BP10",
    "Bulk Paperback",
    {"Author"},
    "isbn-bulk-p",
    "Publisher",
    2026,
    Money::of(100),
    1,
    Binding::Paperback
);

CHECK(bulkPaperback.costFor(9) == Money::of(900));
CHECK(bulkPaperback.costFor(10) == Money::of(900));

Book bulkHardcover(
    "BH10",
    "Bulk Hardcover",
    {"Author"},
    "isbn-bulk-h",
    "Publisher",
    2026,
    Money::of(100),
    1,
    Binding::Hardcover
);

CHECK(bulkHardcover.costFor(10) == Money::of(1080));

Journal bulkJournal(
    "J10",
    "Bulk Journal",
    "1234-5678",
    12,
    "Publisher",
    2026,
    Money::of(100),
    2
);

CHECK(bulkJournal.costFor(9) == Money::of(1800));
CHECK(bulkJournal.costFor(10) == Money::of(1800));

ElectronicResource bulkElectronic(
    "ER50",
    "Bulk Electronic",
    "Publisher",
    2026,
    Money::of(10),
    "url",
    LicenseModel::AnnualSubscription,
    Money::of(100)
);

CHECK(bulkElectronic.costFor(50) == Money::of(600));
CHECK(bulkElectronic.costFor(60) == Money::of(650));

}

static void testCatalog() {
    Catalog c;
    c.emplace<Book>("B1", "Clean Code", std::vector<std::string>{"M"}, "i", "P", 2008,
                    Money::of(1));
    c.emplace<Book>("B2", "Clean Architecture", std::vector<std::string>{"M"}, "i", "P",
                    2017, Money::of(1));
    c.emplace<ElectronicResource>("R1", "ACM Digital Library", "ACM", 2026, Money::of(1),
                                  "url");
    c.emplace<EBook>(
    "E1",
    "EBook Test",
    std::vector<std::string>{"Author"},
    "isbn-ebook",
    "Publisher",
    2025,
    Money::of(10),
    "ebook-url",
    FileFormat::PDF,
    false
);

    c.emplace<AudioBook>(
    "A1",
    "Audiobook Test",
    "Publisher",
    2025,
    Money::of(10),
    "audio-url",
    "Narrator",
    300
);

c.emplace<Thesis>(
    "T1",
    "Thesis Test",
    "IIIT Delhi",
    "M.Tech",
    "Supervisor",
    "IIIT Delhi",
    2026
);
    CHECK(c.byCategory(ResourceCategory::Thesis).size() == 1);

    CHECK(c.byCategory(ResourceCategory::EBook).size() == 1);
    CHECK(c.size() == 6);
    CHECK(c.contains("B1"));
    CHECK(c.find("nope") == nullptr);
    CHECK_THROWS(c.get("nope"), NotFoundError);
    CHECK_THROWS(c.emplace<Book>("B1", "dup", std::vector<std::string>{}, "", "", 1,
                                 Money::of(1)),
                 DuplicateIdError);

    CHECK(c.searchTitle("clean").size() == 2);
    CHECK(c.byCategory(ResourceCategory::ElectronicResource).size() == 2);
    CHECK(c.where([](const Resource& r) { return r.isDigital(); }).size() == 3);  

    CHECK(c.holdings("B1") == 0);
    c.addHoldings("B1", 3);
    CHECK(c.holdings("B1") == 3);
    CHECK_THROWS(c.addHoldings("B1", -5), std::invalid_argument);

    c.remove("R1");
    CHECK(c.size() == 5);
    CHECK_THROWS(c.remove("R1"), NotFoundError);
}

static void testBudget() {
    Budget b(Money::of(1000));
    b.setQuota(ResourceCategory::Book, {5, Money::of(400)});
    b.setQuota(ResourceCategory::Journal, {3, Money::of(600)});
    b.setQuota(ResourceCategory::EBook, {10, Money::of(500)});

    CHECK(b.check(ResourceCategory::Book, 2, Money::of(200)).empty());
    CHECK(!b.check(ResourceCategory::Book, 6, Money::of(10)).empty());   // units
    CHECK(!b.check(ResourceCategory::Book, 1, Money::of(401)).empty());  // spend
    CHECK(!b.check(ResourceCategory::ElectronicResource, 1, Money::of(1001)).empty());  // overall
    CHECK(b.check(ResourceCategory::ElectronicResource, 1, Money::of(900)).empty());    // no quota

    CHECK(b.check(ResourceCategory::EBook, 5, Money::of(300)).empty());

    CHECK(!b.check(ResourceCategory::EBook, 11, Money::of(100)).empty());

    CHECK(!b.check(ResourceCategory::EBook, 1, Money::of(600)).empty());

    b.commit(ResourceCategory::Book, 4, Money::of(300));
    CHECK(b.spent() == Money::of(300));

    CHECK(*b.unitsRemaining(ResourceCategory::Book) == 1);
    CHECK(*b.spendRemaining(ResourceCategory::Book) == Money::of(100));
    CHECK(!b.unitsRemaining(ResourceCategory::ElectronicResource).has_value());
    CHECK(b.check(ResourceCategory::Journal, 2, Money::of(500)).empty());
    CHECK(!b.check(ResourceCategory::Journal, 4, Money::of(100)).empty());
    CHECK(!b.check(ResourceCategory::Journal, 1, Money::of(700)).empty());

    CHECK_THROWS(b.commit(ResourceCategory::Book, 2, Money::of(10)), QuotaExceededError);
    CHECK_THROWS(b.commit(ResourceCategory::ElectronicResource, 1, Money::of(800)),
                 BudgetExceededError);
    CHECK_THROWS(b.commit(ResourceCategory::ElectronicResource, 0, Money::of(1)),
                 std::invalid_argument);
    CHECK(b.spent() == Money::of(300));  // failed commits changed nothing
}

static void testAcquisition() {
    Catalog c;
    c.emplace<Book>("B1", "Book", std::vector<std::string>{"A"}, "i", "P", 2020,
                    Money::of(100));
    c.emplace<ElectronicResource>("R1", "DB", "P", 2026, Money::of(10), "url",
                                  LicenseModel::AnnualSubscription, Money::of(50));
    Budget b(Money::of(500));
    b.setQuota(ResourceCategory::Book, {3, Money::of(1000)});
    AcquisitionManager acq(c, b);

    CHECK(acq.quote("R1", 5) == Money::of(100));
    std::string why;
    CHECK(acq.canPurchase("B1", 3, &why) && why.empty());
    CHECK(!acq.canPurchase("B1", 4, &why) && !why.empty());
    CHECK(!acq.canPurchase("nope", 1, &why));

    const auto& rec = acq.purchase("B1", 2);
    CHECK(rec.approved && rec.cost == Money::of(200) && rec.orderNo == 1);
    CHECK(c.holdings("B1") == 2);

    CHECK_THROWS(acq.purchase("B1", 2), QuotaExceededError);
    CHECK_THROWS(acq.purchase("nope", 1), NotFoundError);
    CHECK(acq.history().size() == 1);  // exceptions don't record

    auto res = acq.processBatch({{"R1", 10}, {"R1", 100}, {"B1", 1}, {"zzz", 1}, {"B1", 0}});
    CHECK(res.size() == 5);
    CHECK(res[0].approved && res[0].cost == Money::of(150));
    CHECK(!res[1].approved);  // 1050 > remaining 150
    CHECK(res[2].approved);
    CHECK(!res[3].approved && res[3].reason.find("not found") != std::string::npos);
    CHECK(!res[4].approved);
    CHECK(acq.totalSpent() == Money::of(450));
    CHECK(b.spent() == acq.totalSpent());
    CHECK(c.holdings("R1") == 10 && c.holdings("B1") == 3);
    CHECK(acq.history().size() == 6);
}

static void testTaxes() {
    Catalog c;

    c.emplace<Book>(
        "TB1",
        "Tax Book",
        std::vector<std::string>{"Author"},
        "tax-isbn",
        "Publisher",
        2026,
        Money::of(100)
    );

    c.emplace<ElectronicResource>(
        "TE1",
        "Tax Electronic",
        "Publisher",
        2026,
        Money::of(100),
        "url"
    );

    Budget b(Money::of(10000));

    b.setQuota(
        ResourceCategory::Book,
        {10, Money::of(115)}
    );

    b.setQuota(
        ResourceCategory::ElectronicResource,
        {10, Money::of(120)}
    );

    AcquisitionManager acq(c, b);

    acq.setTaxRates(10, 20);

    CHECK(acq.printTaxRate() == 10);
    CHECK(acq.electronicTaxRate() == 20);

    CHECK(acq.quote("TB1", 1) == Money::of(110));
    CHECK(acq.quote("TE1", 1) == Money::of(120));

    const auto& bookOrder = acq.purchase("TB1", 1);

    CHECK(bookOrder.preTaxCost == Money::of(100));
    CHECK(bookOrder.cost == Money::of(110));

    CHECK(b.usageFor(ResourceCategory::Book).spent == Money::of(110));

    CHECK_THROWS(
        acq.purchase("TB1", 1),
        QuotaExceededError
    );

    CHECK_THROWS(
        acq.setTaxRates(-1, 20),
        std::invalid_argument
    );
}

static void testTitleQuota() {
    Catalog c;

    c.emplace<Book>(
        "QT1",
        "Title One",
        std::vector<std::string>{"Author"},
        "isbn1",
        "Publisher",
        2026,
        Money::of(100)
    );

    c.emplace<Book>(
        "QT2",
        "Title Two",
        std::vector<std::string>{"Author"},
        "isbn2",
        "Publisher",
        2026,
        Money::of(100)
    );

    c.emplace<Book>(
        "QT3",
        "Title Three",
        std::vector<std::string>{"Author"},
        "isbn3",
        "Publisher",
        2026,
        Money::of(100)
    );

    Budget b(Money::of(10000));

    b.setQuota(
        ResourceCategory::Book,
        {20, Money::of(5000), 2}
    );

    AcquisitionManager acq(c, b);

    acq.purchase("QT1", 2);
    acq.purchase("QT1", 1);

    CHECK(b.usageFor(ResourceCategory::Book).titles == 1);

    acq.purchase("QT2", 1);

    CHECK(b.usageFor(ResourceCategory::Book).titles == 2);

    CHECK_THROWS(
        acq.purchase("QT3", 1),
        QuotaExceededError
    );

    CHECK(b.usageFor(ResourceCategory::Book).titles == 2);
}

static void testCancellation() {
    Catalog c;

    c.emplace<Book>(
        "C1",
        "Cancellation Book",
        std::vector<std::string>{"Author"},
        "cancel-isbn",
        "Publisher",
        2026,
        Money::of(100)
    );

    Budget b(Money::of(5000));

    b.setQuota(
        ResourceCategory::Book,
        {10, Money::of(2000), 2}
    );

    AcquisitionManager acq(c, b);

    const auto& order = acq.purchase("C1", 3);
    const int orderNo = order.orderNo;

    CHECK(c.holdings("C1") == 3);
    CHECK(b.usageFor(ResourceCategory::Book).units == 3);
    CHECK(b.usageFor(ResourceCategory::Book).titles == 1);

    const auto& cancellation = acq.cancel(orderNo);

    CHECK(cancellation.cancellation);
    CHECK(cancellation.originalOrderNo == orderNo);

    CHECK(c.holdings("C1") == 0);
    CHECK(b.usageFor(ResourceCategory::Book).units == 0);
    CHECK(b.usageFor(ResourceCategory::Book).spent == Money::of(0));
    CHECK(b.usageFor(ResourceCategory::Book).titles == 0);

    CHECK(acq.history().size() == 2);
    CHECK(acq.totalSpent() == Money::of(0));

    CHECK_THROWS(
        acq.cancel(orderNo),
        std::invalid_argument
    );
}

static void testDepartmentBudgets() {
    Catalog c;

    c.emplace<Book>(
        "DB1",
        "Department Book",
        std::vector<std::string>{"Author"},
        "dept-isbn",
        "Publisher",
        2026,
        Money::of(100)
    );

    Budget fallback(Money::of(10000));

    AcquisitionManager acq(c, fallback);

    Budget csBudget(Money::of(1000));
    csBudget.setQuota(
        ResourceCategory::Book,
        {5, Money::of(500), 2}
    );

    Budget physicsBudget(Money::of(300));
    physicsBudget.setQuota(
        ResourceCategory::Book,
        {2, Money::of(200), 1}
    );

    acq.setDepartmentBudget(
        Department::ComputerScience,
        csBudget
    );

    acq.setDepartmentBudget(
        Department::Physics,
        physicsBudget
    );

    acq.purchase(
        "DB1",
        3,
        Department::ComputerScience
    );

    CHECK(
        acq.departmentBudget(
            Department::ComputerScience
        ).usageFor(ResourceCategory::Book).units == 3
    );

    CHECK(
        acq.departmentBudget(
            Department::Physics
        ).usageFor(ResourceCategory::Book).units == 0
    );

    acq.purchase(
        "DB1",
        1,
        Department::Physics
    );

    CHECK(
        acq.departmentBudget(
            Department::Physics
        ).usageFor(ResourceCategory::Book).units == 1
    );

    CHECK_THROWS(
        acq.purchase(
            "DB1",
            2,
            Department::Physics
        ),
        QuotaExceededError
    );
}

static void testBudgetRollover() {
    Budget b(Money::of(1000));

    b.setQuota(
        ResourceCategory::Book,
        {10, Money::of(1000), 5}
    );

    b.commit(
        ResourceCategory::Book,
        2,
        Money::of(400),
        "RB1"
    );

    Budget next = b.rollover(50);

    CHECK(next.total() == Money::of(1300));
    CHECK(next.spent() == Money::of(0));
    CHECK(next.remaining() == Money::of(1300));

    auto q = next.quotaFor(ResourceCategory::Book);

    CHECK(q.has_value());
    CHECK(q->maxUnits == 10);
    CHECK(q->maxSpend == Money::of(1000));
    CHECK(q->maxTitles == 5);

    CHECK(next.usageFor(ResourceCategory::Book).units == 0);
    CHECK(next.usageFor(ResourceCategory::Book).titles == 0);

    CHECK_THROWS(
        b.rollover(-1),
        std::invalid_argument
    );

    CHECK_THROWS(
        b.rollover(101),
        std::invalid_argument
    );
}

static void testAllOrNothingBatch() {
    Catalog c;

    c.emplace<Book>(
        "AB1",
        "Batch Book One",
        std::vector<std::string>{"Author"},
        "batch-1",
        "Publisher",
        2026,
        Money::of(100)
    );

    c.emplace<Book>(
        "AB2",
        "Batch Book Two",
        std::vector<std::string>{"Author"},
        "batch-2",
        "Publisher",
        2026,
        Money::of(100)
    );

    Budget b(Money::of(1000));

    b.setQuota(
        ResourceCategory::Book,
        {3, Money::of(1000), 5}
    );

    AcquisitionManager acq(c, b);

    std::vector<PurchaseRequest> badBatch = {
        {"AB1", 2},
        {"AB2", 2}
    };

    auto rejected =
        acq.processBatch(badBatch, true);

    CHECK(rejected.size() == 2);
    CHECK(!rejected[0].approved);
    CHECK(!rejected[1].approved);

    CHECK(c.holdings("AB1") == 0);
    CHECK(c.holdings("AB2") == 0);

    CHECK(
        b.usageFor(ResourceCategory::Book).units == 0
    );

    std::vector<PurchaseRequest> goodBatch = {
        {"AB1", 1},
        {"AB2", 2}
    };

    auto approved =
        acq.processBatch(goodBatch, true);

    CHECK(approved.size() == 2);
    CHECK(approved[0].approved);
    CHECK(approved[1].approved);

    CHECK(c.holdings("AB1") == 1);
    CHECK(c.holdings("AB2") == 2);

    CHECK(
        b.usageFor(ResourceCategory::Book).units == 3
    );
}

int main() {
    testMoney();
    testResourcesAndCost();
    testCatalog();
    testBudget();
    testAcquisition();
    testTaxes();
    testTitleQuota();
    testCancellation();
    testDepartmentBudgets();
    testBudgetRollover();
    testAllOrNothingBatch();
    std::cout << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
