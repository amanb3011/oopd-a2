// Minimal self-contained test runner (no external framework needed).

#include <iostream>
#include <cassert>
#include <sstream>
#include <stdexcept>
#include <string>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;






template <typename Exception, typename Func>
void assertThrows(Func&& func) {
    bool thrown = false;

    try {
        func();
    }
    catch (const Exception&) {
        thrown = true;
    }
    catch (...) {
    }

    assert(thrown);
}

static void testMoney() {
    assert(Money::of(12, 5).toString() == "12.05");
    assert(Money::of(-3, 50).toString() == "-3.50");
    assert(Money::fromMinor(7).toString() == "0.07");
    assert(Money::of(10) + Money::of(0, 50) == Money::fromMinor(1050));
    assert(Money::of(3) * 4 == Money::of(12));
    assert(Money::of(1) < Money::of(2));
    assertThrows<std::invalid_argument>([&]() {
        (void)(Money::of(1, 100));
    });
}

static void testResourcesAndCost() {
    Book b("B1", "T", {"A", "B", "C"}, "isbn", "P", 2020, Money::of(100));
    assert(b.category() == ResourceCategory::Book);
    assert(!b.isDigital());
    assert(b.costFor(3) == Money::of(300));
    assertThrows<std::invalid_argument>([&]() {
        (void)(b.costFor(0));
    });
    assert(joinAuthors(b.authors()) == "A, B and C");

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

assert(paperback.costFor(2) == Money::of(200));
assert(hardcover.costFor(2) == Money::of(240));

    ElectronicResource e("R1", "DB", "P", 2026, Money::of(10), "url",
                         LicenseModel::AnnualSubscription, Money::of(100));
    assert(e.isDigital());
    assert(e.costFor(5) == Money::of(150));

    assert(e.category() == ResourceCategory::ElectronicResource);

    // Polymorphism through a base-class reference
    const Resource& r = e;
    assert(r.costFor(1) == Money::of(110));
    std::ostringstream os;
    os << r;
    assert(os.str().find("platform fee: 100.00") != std::string::npos);

    assertThrows<std::invalid_argument>([&]() {
        (void)(Book("", "T", {}, "", "", 2000, Money::of(1)));
    });
    assertThrows<std::invalid_argument>([&]() {
        (void)(Book("B", "T", {}, "", "", 2000, Money::fromMinor(-1)));
    });

    Journal j("J1", "Nature", "0028-0836", 52,
          "Springer Nature", 2026, Money::of(500), 2);

    assert(j.category() == ResourceCategory::Journal);
    assert(!j.isDigital());
    assert(j.issn() == "0028-0836");
    assert(j.issuesPerYear() == 52);
    assert(j.subscriptionYears() == 2);
    assert(j.costFor(3) == Money::of(3000));

    assertThrows<std::invalid_argument>([&]() {
        (void)(Journal("J2", "Test Journal", "1234-5678", 12,
                "Publisher", 2026, Money::of(100), 0));
    });    
    
    std::ostringstream journalOut;
    journalOut << j;

    assert(journalOut.str().find("issn: 0028-0836") != std::string::npos);
    assert(journalOut.str().find("issues per year: 52") != std::string::npos);
    assert(journalOut.str().find("subscription years: 2") != std::string::npos);

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

    assert(eb.category() == ResourceCategory::EBook);
    assert(eb.isDigital());
    assert(eb.authors().size() == 1);
    assert(eb.isbn() == "978-1491903995");
    assert(eb.format() == FileFormat::EPUB);
    assert(eb.drmProtected());

    assert(eb.costFor(5) == Money::of(200));

    std::ostringstream ebookOut;
    ebookOut << eb;

    assert(ebookOut.str().find("access url: https://example.com/ebook")
        != std::string::npos);

    assert(ebookOut.str().find("authors: Scott Meyers")
        != std::string::npos);

    assert(ebookOut.str().find("isbn: 978-1491903995")
        != std::string::npos);

    assert(ebookOut.str().find("file format: EPUB")
        != std::string::npos);

    assert(ebookOut.str().find("drm protected: yes")
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

    assert(ab.category() == ResourceCategory::ElectronicResource);
    assert(ab.isDigital());
    assert(ab.narrator() == "Simon Prebble");
    assert(ab.durationMinutes() == 660);

    // ElectronicResource pricing is inherited:
    // 100 platform fee + 30 * 2 seats = 160
    assert(ab.costFor(2) == Money::of(160));

    std::ostringstream audioOut;
    audioOut << ab;

    assert(audioOut.str().find("narrator: Simon Prebble") != std::string::npos);
    assert(audioOut.str().find("duration minutes: 660") != std::string::npos);

    Thesis thesis(
        "T1",
        "Low Power VLSI Design",
        "IIIT Delhi",
        "M.Tech",
        "Dr. Sharma",
        "IIIT Delhi",
        2026
    );

    assert(thesis.category() == ResourceCategory::Thesis);
    assert(!thesis.isDigital());
    assert(thesis.university() == "IIIT Delhi");
    assert(thesis.degree() == "M.Tech");
    assert(thesis.supervisor() == "Dr. Sharma");

    // default unit price is zero
    assert(thesis.costFor(1) == Money::of(0));

    std::ostringstream thesisOut;
    thesisOut << thesis;

    assert(thesisOut.str().find("university: IIIT Delhi") != std::string::npos);
    assert(thesisOut.str().find("degree: M.Tech") != std::string::npos);
    assert(thesisOut.str().find("supervisor: Dr. Sharma") != std::string::npos);

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

assert(bulkPaperback.costFor(9) == Money::of(900));
assert(bulkPaperback.costFor(10) == Money::of(900));

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

assert(bulkHardcover.costFor(10) == Money::of(1080));

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

assert(bulkJournal.costFor(9) == Money::of(1800));
assert(bulkJournal.costFor(10) == Money::of(1800));

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

assert(bulkElectronic.costFor(50) == Money::of(600));
assert(bulkElectronic.costFor(60) == Money::of(650));

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
    assert(c.byCategory(ResourceCategory::Thesis).size() == 1);

    assert(c.byCategory(ResourceCategory::EBook).size() == 1);
    assert(c.size() == 6);
    assert(c.contains("B1"));
    assert(c.find("nope") == nullptr);
    assertThrows<NotFoundError>([&]() {
        (void)(c.get("nope"));
    });
    assertThrows<DuplicateIdError>([&]() {
        (void)(c.emplace<Book>("B1", "dup", std::vector<std::string>{}, "", "", 1,
                                 Money::of(1)));
    });

    assert(c.searchTitle("clean").size() == 2);
    assert(c.byCategory(ResourceCategory::ElectronicResource).size() == 2);
    assert(c.where([](const Resource& r) { return r.isDigital(); }).size() == 3);  

    assert(c.holdings("B1") == 0);
    c.addHoldings("B1", 3);
    assert(c.holdings("B1") == 3);
    assertThrows<std::invalid_argument>([&]() {
        (void)(c.addHoldings("B1", -5));
    });

    c.remove("R1");
    assert(c.size() == 5);
    assertThrows<NotFoundError>([&]() {
        (void)(c.remove("R1"));
    });
}

static void testBudget() {
    Budget b(Money::of(1000));
    b.setQuota(ResourceCategory::Book, {5, Money::of(400)});
    b.setQuota(ResourceCategory::Journal, {3, Money::of(600)});
    b.setQuota(ResourceCategory::EBook, {10, Money::of(500)});

    assert(b.check(ResourceCategory::Book, 2, Money::of(200)).empty());
    assert(!b.check(ResourceCategory::Book, 6, Money::of(10)).empty());   // units
    assert(!b.check(ResourceCategory::Book, 1, Money::of(401)).empty());  // spend
    assert(!b.check(ResourceCategory::ElectronicResource, 1, Money::of(1001)).empty());  // overall
    assert(b.check(ResourceCategory::ElectronicResource, 1, Money::of(900)).empty());    // no quota

    assert(b.check(ResourceCategory::EBook, 5, Money::of(300)).empty());

    assert(!b.check(ResourceCategory::EBook, 11, Money::of(100)).empty());

    assert(!b.check(ResourceCategory::EBook, 1, Money::of(600)).empty());

    b.commit(ResourceCategory::Book, 4, Money::of(300));
    assert(b.spent() == Money::of(300));

    assert(*b.unitsRemaining(ResourceCategory::Book) == 1);
    assert(*b.spendRemaining(ResourceCategory::Book) == Money::of(100));
    assert(!b.unitsRemaining(ResourceCategory::ElectronicResource).has_value());
    assert(b.check(ResourceCategory::Journal, 2, Money::of(500)).empty());
    assert(!b.check(ResourceCategory::Journal, 4, Money::of(100)).empty());
    assert(!b.check(ResourceCategory::Journal, 1, Money::of(700)).empty());

    assertThrows<QuotaExceededError>([&]() {
        (void)(b.commit(ResourceCategory::Book, 2, Money::of(10)));
    });
    assertThrows<BudgetExceededError>([&]() {
        (void)(b.commit(ResourceCategory::ElectronicResource, 1, Money::of(800)));
    });
    assertThrows<std::invalid_argument>([&]() {
        (void)(b.commit(ResourceCategory::ElectronicResource, 0, Money::of(1)));
    });
    assert(b.spent() == Money::of(300));  // failed commits changed nothing
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

    assert(acq.quote("R1", 5) == Money::of(100));
    std::string why;
    assert(acq.canPurchase("B1", 3, &why) && why.empty());
    assert(!acq.canPurchase("B1", 4, &why) && !why.empty());
    assert(!acq.canPurchase("nope", 1, &why));

    const auto& rec = acq.purchase("B1", 2);
    assert(rec.approved && rec.cost == Money::of(200) && rec.orderNo == 1);
    assert(c.holdings("B1") == 2);

    assertThrows<QuotaExceededError>([&]() {
        (void)(acq.purchase("B1", 2));
    });
    assertThrows<NotFoundError>([&]() {
        (void)(acq.purchase("nope", 1));
    });
    assert(acq.history().size() == 1);  // exceptions don't record

    auto res = acq.processBatch({{"R1", 10}, {"R1", 100}, {"B1", 1}, {"zzz", 1}, {"B1", 0}});
    assert(res.size() == 5);
    assert(res[0].approved && res[0].cost == Money::of(150));
    assert(!res[1].approved);  // 1050 > remaining 150
    assert(res[2].approved);
    assert(!res[3].approved && res[3].reason.find("not found") != std::string::npos);
    assert(!res[4].approved);
    assert(acq.totalSpent() == Money::of(450));
    assert(b.spent() == acq.totalSpent());
    assert(c.holdings("R1") == 10 && c.holdings("B1") == 3);
    assert(acq.history().size() == 6);
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

    assert(acq.printTaxRate() == 10);
    assert(acq.electronicTaxRate() == 20);

    assert(acq.quote("TB1", 1) == Money::of(110));
    assert(acq.quote("TE1", 1) == Money::of(120));

    const auto& bookOrder = acq.purchase("TB1", 1);

    assert(bookOrder.preTaxCost == Money::of(100));
    assert(bookOrder.cost == Money::of(110));

    assert(b.usageFor(ResourceCategory::Book).spent == Money::of(110));

    assertThrows<QuotaExceededError>([&]() {
        (void)(acq.purchase("TB1", 1));
    });

    assertThrows<std::invalid_argument>([&]() {
        (void)(acq.setTaxRates(-1, 20));
    });
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

    assert(b.usageFor(ResourceCategory::Book).titles == 1);

    acq.purchase("QT2", 1);

    assert(b.usageFor(ResourceCategory::Book).titles == 2);

    assertThrows<QuotaExceededError>([&]() {
        (void)(acq.purchase("QT3", 1));
    });

    assert(b.usageFor(ResourceCategory::Book).titles == 2);
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

    assert(c.holdings("C1") == 3);
    assert(b.usageFor(ResourceCategory::Book).units == 3);
    assert(b.usageFor(ResourceCategory::Book).titles == 1);

    const auto& cancellation = acq.cancel(orderNo);

    assert(cancellation.cancellation);
    assert(cancellation.originalOrderNo == orderNo);

    assert(c.holdings("C1") == 0);
    assert(b.usageFor(ResourceCategory::Book).units == 0);
    assert(b.usageFor(ResourceCategory::Book).spent == Money::of(0));
    assert(b.usageFor(ResourceCategory::Book).titles == 0);

    assert(acq.history().size() == 2);
    assert(acq.totalSpent() == Money::of(0));

    assertThrows<std::invalid_argument>([&]() {
        (void)(acq.cancel(orderNo));
    });
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

    assert(
        acq.departmentBudget(
            Department::ComputerScience
        ).usageFor(ResourceCategory::Book).units == 3
    );

    assert(
        acq.departmentBudget(
            Department::Physics
        ).usageFor(ResourceCategory::Book).units == 0
    );

    acq.purchase(
        "DB1",
        1,
        Department::Physics
    );

    assert(
        acq.departmentBudget(
            Department::Physics
        ).usageFor(ResourceCategory::Book).units == 1
    );

    assertThrows<QuotaExceededError>([&]() {
        (void)(acq.purchase(
            "DB1",
            2,
            Department::Physics
        ));
    });
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

    assert(next.total() == Money::of(1300));
    assert(next.spent() == Money::of(0));
    assert(next.remaining() == Money::of(1300));

    auto q = next.quotaFor(ResourceCategory::Book);

    assert(q.has_value());
    assert(q->maxUnits == 10);
    assert(q->maxSpend == Money::of(1000));
    assert(q->maxTitles == 5);

    assert(next.usageFor(ResourceCategory::Book).units == 0);
    assert(next.usageFor(ResourceCategory::Book).titles == 0);

    assertThrows<std::invalid_argument>([&]() {
        (void)(b.rollover(-1));
    });

    assertThrows<std::invalid_argument>([&]() {
        (void)(b.rollover(101));
    });
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

    assert(rejected.size() == 2);
    assert(!rejected[0].approved);
    assert(!rejected[1].approved);

    assert(c.holdings("AB1") == 0);
    assert(c.holdings("AB2") == 0);

    assert(
        b.usageFor(ResourceCategory::Book).units == 0
    );

    std::vector<PurchaseRequest> goodBatch = {
        {"AB1", 1},
        {"AB2", 2}
    };

    auto approved =
        acq.processBatch(goodBatch, true);

    assert(approved.size() == 2);
    assert(approved[0].approved);
    assert(approved[1].approved);

    assert(c.holdings("AB1") == 1);
    assert(c.holdings("AB2") == 2);

    assert(
        b.usageFor(ResourceCategory::Book).units == 3
    );
}

static void testVendors() {
    Catalog c;

    c.emplace<Book>(
        "V1",
        "Vendor Book",
        std::vector<std::string>{"Author"},
        "vendor-isbn",
        "Publisher",
        2026,
        Money::of(500)
    );

    Budget b(Money::of(10000));

    b.setQuota(
        ResourceCategory::Book,
        {20, Money::of(10000), 5}
    );

    AcquisitionManager acq(c, b);

    acq.addVendor(
        "V1",
        "Vendor A",
        Money::of(450)
    );

    acq.addVendor(
        "V1",
        "Vendor B",
        Money::of(400)
    );

    acq.addVendor(
        "V1",
        "Vendor C",
        Money::of(425)
    );

    assert(
        acq.quote("V1", 2) ==
        Money::of(800)
    );

    const auto& order =
        acq.purchase("V1", 2);

    assert(order.vendor == "Vendor B");
    assert(order.preTaxCost == Money::of(800));
    assert(order.cost == Money::of(800));

    assert(c.holdings("V1") == 2);

    assertThrows<std::invalid_argument>([&]() {
        (void)(acq.addVendor(
            "NOPE",
            "Vendor",
            Money::of(100)
        ));
    });
}

static void testCatalogSearches() {
    Catalog c;

    c.emplace<Book>(
        "SB1",
        "Search Book",
        std::vector<std::string>{
            "Robert C. Martin"
        },
        "isbn-book",
        "Publisher",
        2010,
        Money::of(100)
    );

    c.emplace<EBook>(
        "SE1",
        "Search EBook",
        std::vector<std::string>{
            "Scott Meyers"
        },
        "isbn-ebook",
        "Publisher",
        2015,
        Money::of(20),
        "url",
        FileFormat::PDF,
        false
    );

    c.emplace<Journal>(
        "SJ1",
        "Search Journal",
        "issn-journal",
        12,
        "Publisher",
        2020,
        Money::of(50)
    );

    auto martin =
        c.searchAuthor("martin");

    assert(martin.size() == 1);
    assert(martin[0]->id() == "SB1");

    auto scott =
        c.searchAuthor("SCOTT");

    assert(scott.size() == 1);
    assert(scott[0]->id() == "SE1");

    auto isbnBook =
        c.searchIdentifier("isbn-book");

    assert(isbnBook.size() == 1);
    assert(isbnBook[0]->id() == "SB1");

    auto isbnEBook =
        c.searchIdentifier("isbn-ebook");

    assert(isbnEBook.size() == 1);
    assert(isbnEBook[0]->id() == "SE1");

    auto issn =
        c.searchIdentifier("issn-journal");

    assert(issn.size() == 1);
    assert(issn[0]->id() == "SJ1");

    auto years =
        c.searchYearRange(2010, 2016);

    assert(years.size() == 2);

    assertThrows<std::invalid_argument>([&]() {
        (void)(c.searchYearRange(2020, 2010));
    });
}

static void testLending() {
    Catalog c;

    c.emplace<Book>(
        "LB1",
        "Lending Book",
        std::vector<std::string>{"Author"},
        "lend-isbn",
        "Publisher",
        2026,
        Money::of(100)
    );

    c.emplace<ElectronicResource>(
        "LE1",
        "Lending Electronic",
        "Publisher",
        2026,
        Money::of(50),
        "url"
    );

    c.addHoldings("LB1", 2);
    c.addHoldings("LE1", 2);

    c.borrowPrint("LB1", "P1");
    c.borrowPrint("LB1", "P2");

    assert(c.borrowedCount("LB1") == 2);

    assertThrows<std::invalid_argument>([&]() {
        (void)(c.borrowPrint("LB1", "P3"));
    });

    c.returnPrint("LB1", "P1");

    assert(c.borrowedCount("LB1") == 1);

    c.borrowPrint("LB1", "P3");

    assert(c.borrowedCount("LB1") == 2);

    assertThrows<std::invalid_argument>([&]() {
        (void)(c.borrowPrint("LE1", "P4"));
    });

    c.openSession("LE1", "P1");
    c.openSession("LE1", "P2");

    assert(c.activeSessions("LE1") == 2);

    assertThrows<std::invalid_argument>([&]() {
        (void)(c.openSession("LE1", "P3"));
    });

    c.closeSession("LE1", "P1");

    assert(c.activeSessions("LE1") == 1);

    c.openSession("LE1", "P3");

    assert(c.activeSessions("LE1") == 2);

    assertThrows<std::invalid_argument>([&]() {
        (void)(c.openSession("LB1", "P4"));
    });
}

static void testMoneyCurrencies() {
    Money inrA =
        Money::of(100, 0, "INR");

    Money inrB =
        Money::of(50, 0, "INR");

    Money usd =
        Money::of(100, 0, "USD");

    assert(inrA.currency() == "INR");
    assert(usd.currency() == "USD");

    assert(
        inrA + inrB ==
        Money::of(150, 0, "INR")
    );

    assert(
        inrA - inrB ==
        Money::of(50, 0, "INR")
    );

    assert(
        inrA > inrB
    );

    assertThrows<std::invalid_argument>([&]() {
        (void)(inrA + usd);
    });

    assertThrows<std::invalid_argument>([&]() {
        (void)(inrA - usd);
    });

    assertThrows<std::invalid_argument>([&]() {
        (void)(inrA == usd);
    });

    assertThrows<std::invalid_argument>([&]() {
        (void)(inrA < usd);
    });

    assertThrows<std::invalid_argument>([&]() {
        (void)(inrA > usd);
    });

    Book usdHardcover(
        "USD-B", "USD Book", {"Author"}, "usd-isbn", "Publisher",
        2026, Money::of(100, 0, "USD"), 1, Binding::Hardcover
    );
    Money usdBookCost = usdHardcover.costFor(1);
    assert(usdBookCost == Money::of(120, 0, "USD"));
    assert(usdBookCost.currency() == "USD");

    ElectronicResource usdElectronic(
        "USD-E", "USD Electronic", "Publisher", 2026,
        Money::of(10, 0, "USD"), "url"
    );
    Money usdElectronicCost = usdElectronic.costFor(2);
    assert(usdElectronicCost == Money::of(20, 0, "USD"));

    Budget usdBudget(Money::of(1000, 0, "USD"));
    usdBudget.setQuota(
        ResourceCategory::Book,
        {10, Money::of(500, 0, "USD")}
    );
    usdBudget.commit(
        ResourceCategory::Book,
        1,
        Money::of(100, 0, "USD"),
        "USD-B"
    );
    assert(usdBudget.spent() == Money::of(100, 0, "USD"));
    assert(usdBudget.rollover(50).total().currency() == "USD");
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
    testVendors();
    testCatalogSearches();
    testLending();
    testMoneyCurrencies();
    return 0;
}
