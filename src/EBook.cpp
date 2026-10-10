#include "bookmgmt/EBook.h"

#include <ostream>
#include <utility>

namespace bookmgmt {

const char* fileFormatName(FileFormat format) {
    switch (format) {
        case FileFormat::PDF: return "PDF";
        case FileFormat::EPUB: return "EPUB";
        case FileFormat::HTML: return "HTML";
    }

    return "Unknown";
}

EBook::EBook(std::string id,
             std::string title,
             std::vector<std::string> authors,
             std::string isbn,
             std::string publisher,
             int year,
             Money pricePerSeat,
             std::string accessUrl,
             FileFormat format,
             bool drmProtected,
             LicenseModel license,
             Money platformFee)
    : ElectronicResource(
          std::move(id),
          std::move(title),
          std::move(publisher),
          year,
          pricePerSeat,
          std::move(accessUrl),
          license,
          platformFee),
      authors_(std::move(authors)),
      isbn_(std::move(isbn)),
      format_(format),
      drmProtected_(drmProtected) {}

// EBook and Book both contain bibliographic information such as authors
// and ISBN. This duplication could be avoided by moving these common
// fields into a shared bibliographic base/helper class.

void EBook::printDetails(std::ostream& os) const {
    ElectronicResource::printDetails(os);

    os << "  authors: ";

    for (std::size_t i = 0; i < authors_.size(); ++i) {
        if (i > 0)
            os << (i + 1 == authors_.size() ? " and " : ", ");

        os << authors_[i];
    }

    os << "\n"
       << "  isbn: " << isbn_ << "\n"
       << "  file format: " << fileFormatName(format_) << "\n"
       << "  drm protected: " << (drmProtected_ ? "yes" : "no") << "\n";
}

}  // namespace bookmgmt