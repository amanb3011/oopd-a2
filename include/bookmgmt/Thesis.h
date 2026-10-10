#pragma once

#include <string>

#include "bookmgmt/Resource.h"

namespace bookmgmt {

// Thesis derives directly from Resource because Book-specific fields such
// as ISBN, edition and binding are not naturally required for a thesis.
// Its price defaults to zero because theses are usually free of cost.
class Thesis : public Resource {
public:
    Thesis(std::string id,
           std::string title,
           std::string university,
           std::string degree,
           std::string supervisor,
           std::string publisher,
           int year,
           Money unitPrice = Money{});

    const std::string& university() const { return university_; }
    const std::string& degree() const { return degree_; }
    const std::string& supervisor() const { return supervisor_; }

    ResourceCategory category() const override {
        return ResourceCategory::Thesis;
    }

protected:
    void printDetails(std::ostream& os) const override;

private:
    std::string university_;
    std::string degree_;
    std::string supervisor_;
};

}  // namespace bookmgmt