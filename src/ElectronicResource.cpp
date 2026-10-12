#include "bookmgmt/ElectronicResource.h"

#include <ostream>
#include <stdexcept>

namespace bookmgmt {

const char* licenseName(LicenseModel m) {
    switch (m) {
        case LicenseModel::Perpetual: return "Perpetual";
        case LicenseModel::AnnualSubscription: return "Annual subscription";
    }
    return "Unknown";
}

ElectronicResource::ElectronicResource(std::string id, std::string title,
                                       std::string publisher, int year,
                                       Money pricePerSeat, std::string accessUrl,
                                       LicenseModel license, Money platformFee)
    : Resource(std::move(id), std::move(title), std::move(publisher), year, pricePerSeat),
      accessUrl_(std::move(accessUrl)),
      license_(license),
      platformFee_(platformFee) {
    if (platformFee_.isNegative())
        throw std::invalid_argument("platform fee must not be negative");

    if (platformFee_.isZero() &&
        platformFee_.currency() != unitPrice().currency()) {
        platformFee_ = Money::fromMinor(0, unitPrice().currency());
    } else if (platformFee_.currency() != unitPrice().currency()) {
        throw std::invalid_argument(
            "platform fee currency must match seat price currency"
        );
    }
}

Money ElectronicResource::costFor(int seats) const {
    requirePositive(seats);

    int fullPriceSeats = seats > 50 ? 50 : seats;
    int halfPriceSeats = seats > 50 ? seats - 50 : 0;

    std::int64_t seatCost =
        unitPrice().minorUnits() * fullPriceSeats;

    seatCost +=
        (unitPrice().minorUnits() * halfPriceSeats) / 2;

    return Money::fromMinor(
        platformFee_.minorUnits() + seatCost,
        unitPrice().currency()
    );
}

Money ElectronicResource::costForWithUnitPrice(
    int seats,
    Money unitPrice) const {

    if (seats <= 0)
        throw std::invalid_argument(
            "quantity must be positive"
        );

    const int fullPriceSeats =
        seats > 50 ? 50 : seats;

    const int halfPriceSeats =
        seats > 50 ? seats - 50 : 0;

    std::int64_t seatCost =
        unitPrice.minorUnits() *
        fullPriceSeats;

    seatCost +=
        (unitPrice.minorUnits() *
         halfPriceSeats) / 2;

    if (unitPrice.currency() != platformFee_.currency()) {
        throw std::invalid_argument(
            "vendor price currency must match resource currency"
        );
    }

    return Money::fromMinor(
        platformFee_.minorUnits() +
        seatCost,
        unitPrice.currency()
    );
}

void ElectronicResource::printDetails(std::ostream& os) const {
    os << "  access url: " << accessUrl_ << "\n"
       << "  license: " << licenseName(license_) << "\n"
       << "  platform fee: " << platformFee_ << "\n";
}

}  // namespace bookmgmt
