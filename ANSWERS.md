# Assignment 2 – Answers

**Roll No:** MT26151  
**Name:** Aman Bhandari

## Questions Attempted

I attempted all questions from Q1 to Q16.

## Design Decisions

### Q1 – Journal

`Journal` is derived directly from `Resource` because it is a print resource with its own subscription-based pricing model. It stores ISSN, issues per year, and subscription length.

### Q2 – EBook

`EBook` is derived from `ElectronicResource` because it follows the same electronic licensing and seat-based pricing model.

There is some duplicated book-related information between `Book` and `EBook`, mainly authors and ISBN. This duplication could be reduced by introducing a common intermediate base class containing shared publication metadata.

### Q3 – AudioBook and Thesis

`AudioBook` is derived from `ElectronicResource` because it is accessed electronically and uses seat-based licensing.

`Thesis` is derived directly from `Resource` because it is a separate library resource type and is normally free of cost.

### Q4 – Hardcover Pricing

Hardcover books apply a 20% surcharge before any bulk discount is applied.

### Q5 – Bulk Discounts

Print resources receive a 10% discount when at least 10 copies are purchased. For electronic resources, the first 50 seats are charged at full price and every additional seat is charged at half price.

### Q6 – Taxes

Separate configurable tax rates are maintained for print and electronic resources. Tax is added after the base resource cost is calculated. Budget and quota checks use the post-tax cost, while reports store both pre-tax and post-tax values.

### Q7 – Title Limits

Each category quota can contain a maximum number of distinct titles. A set of purchased resource IDs is maintained so buying more copies of an already purchased title does not increase the title count.

### Q8 – Cancellation

Cancellation does not delete the original order. A separate cancellation record is added to the history. Holdings, unit usage, spend usage, title usage, and budget amount are refunded.

### Q9 – Department Budgets

Each department can have a separate `Budget` object. Purchase requests identify the department to which the order should be charged. A fallback budget is retained for compatibility.

### Q10 – Year-End Rollover

A new budget is generated from the existing one. A configurable percentage of the unspent amount is added to the next year's total budget. Quotas are copied but usage is reset.

### Q11 – All-or-Nothing Batch

For atomic batch processing, temporary copies of the relevant budgets are used to validate the complete batch first. If any request fails, no holdings or real budget values are changed.

### Q12 – Vendors

Multiple vendor offers can be stored for a resource. The vendor with the lowest unit price is selected automatically. The resource's normal pricing rules, such as hardcover surcharge, subscription length, electronic platform fee, and bulk discounts, are still applied. The selected vendor is stored in the purchase record.

### Q13 – Catalogue Searches

The catalogue supports searching by author, ISBN/ISSN, and inclusive publication-year range. Author search is case-insensitive.

### Q14 – Lending

Print resources maintain borrowers and cannot have more simultaneous loans than the number of copies held.

Electronic resources maintain active user sessions and cannot exceed the number of licensed seats.

### Q15 – Currency

`Money` stores a currency code in addition to the amount. Addition, subtraction, and comparisons between different currencies throw `std::invalid_argument`.

### Q16 – Assertions

The previous custom test macros were replaced with standard C++ `assert` calls. Exception tests use a small `assertThrows` helper that internally verifies the expected exception using `assert`.

## Testing

The complete project was built using:

```bash
cmake -S . -B build
cmake --build build
./build/bookmgmt_tests
```

The test program completes successfully with exit code `0`.
