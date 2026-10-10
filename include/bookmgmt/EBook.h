#pragma once

#include <string>
#include <vector>

#include "bookmgmt/ElectronicResource.h"

namespace bookmgmt {

enum class FileFormat {
    PDF,
    EPUB,
    HTML
};

class EBook : public ElectronicResource {
public:
    EBook(std::string id,
          std::string title,
          std::vector<std::string> authors,
          std::string isbn,
          std::string publisher,
          int year,
          Money pricePerSeat,
          std::string accessUrl,
          FileFormat format,
          bool drmProtected,
          LicenseModel license = LicenseModel::AnnualSubscription,
          Money platformFee = Money{});

    const std::vector<std::string>& authors() const { return authors_; }
    const std::string& isbn() const { return isbn_; }
    FileFormat format() const { return format_; }
    bool drmProtected() const { return drmProtected_; }

    ResourceCategory category() const override {
        return ResourceCategory::EBook;
    }

protected:
    void printDetails(std::ostream& os) const override;

private:
    std::vector<std::string> authors_;
    std::string isbn_;
    FileFormat format_;
    bool drmProtected_;
};

const char* fileFormatName(FileFormat format);

}  // namespace bookmgmt