#pragma once

#include <string>

#include "bookmgmt/ElectronicResource.h"

namespace bookmgmt {

// AudioBook derives from ElectronicResource because it is a digital
// resource and can reuse electronic-resource licensing and pricing.
class AudioBook : public ElectronicResource {
public:
    AudioBook(std::string id,
              std::string title,
              std::string publisher,
              int year,
              Money pricePerSeat,
              std::string accessUrl,
              std::string narrator,
              int durationMinutes,
              LicenseModel license = LicenseModel::AnnualSubscription,
              Money platformFee = Money{});

    const std::string& narrator() const { return narrator_; }
    int durationMinutes() const { return durationMinutes_; }

protected:
    void printDetails(std::ostream& os) const override;

private:
    std::string narrator_;
    int durationMinutes_;
};

}  // namespace bookmgmt