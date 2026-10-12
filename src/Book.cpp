#include "bookmgmt/Book.h"

#include <ostream>
#include <stdexcept>

namespace bookmgmt {

std::string joinAuthors(const std::vector<std::string>& authors) {
    std::string out;
    for (std::size_t i = 0; i < authors.size(); ++i) {
        if (i > 0) out += (i + 1 == authors.size()) ? " and " : ", ";
        out += authors[i];
    }
    return out;
}

Book::Book(std::string id, std::string title, std::vector<std::string> authors,
           std::string isbn, std::string publisher, int year, Money unitPrice,
           int edition, Binding binding)
    : Resource(std::move(id), std::move(title), std::move(publisher), year, unitPrice),
      authors_(std::move(authors)),
      isbn_(std::move(isbn)),
      edition_(edition),
      binding_(binding) {
    if (edition_ < 1) throw std::invalid_argument("edition must be >= 1");
}

Money Book::costFor(int quantity) const {
    requirePositive(quantity);

    Money total = unitPrice() * quantity;

    // Hardcover costs 20% more.
    if (binding_ == Binding::Hardcover) {
        std::int64_t surcharge = total.minorUnits() / 5;
        total = Money::fromMinor(total.minorUnits() + surcharge);
    }

    // Print-item bulk discount: 10% off for 10 or more copies.
    if (quantity >= 10) {
        std::int64_t discount = total.minorUnits() / 10;
        total = Money::fromMinor(total.minorUnits() - discount);
    }

    return total;
}

Money Book::costForWithUnitPrice(
    int quantity,
    Money unitPrice) const {

    if (quantity <= 0)
        throw std::invalid_argument(
            "quantity must be positive"
        );

    Money total = unitPrice * quantity;

    if (binding_ == Binding::Hardcover) {
        total = Money::fromMinor(
            total.minorUnits() +
            total.minorUnits() / 5
        );
    }

    if (quantity >= 10) {
        total = Money::fromMinor(
            total.minorUnits() -
            total.minorUnits() / 10
        );
    }

    return total;
}

void Book::printDetails(std::ostream& os) const {
    os << "  authors: " << joinAuthors(authors_) << "\n"
       << "  isbn: " << isbn_ << "\n"
       << "  edition: " << edition_ << "\n"
       << "  binding: " << (binding_ == Binding::Hardcover ? "Hardcover" : "Paperback")
       << "\n";
}

}  // namespace bookmgmt
