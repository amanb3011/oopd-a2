#include "bookmgmt/Budget.h"

#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <cstdint>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

namespace {

// Every category, in the order Budget::print() lists them.
const ResourceCategory kAllCategories[] = {
    ResourceCategory::Book,
    ResourceCategory::Journal,
    ResourceCategory::ElectronicResource,
    ResourceCategory::EBook,
    ResourceCategory::Thesis
};

}  // namespace

Budget::Budget(Money total)
    : total_(total),
      spent_(Money::fromMinor(0, total.currency())) {
    if (total_.isNegative())
        throw std::invalid_argument("budget must not be negative");
}

void Budget::setQuota(ResourceCategory c, Quota q) {
    if (q.maxUnits < 0 || q.maxSpend.isNegative())
        throw std::invalid_argument("quota limits must not be negative");

    if (q.maxTitles < -1)
        throw std::invalid_argument("title quota must be -1 or non-negative");

    if (q.maxSpend.currency() != total_.currency())
        throw std::invalid_argument("quota currency must match budget currency");

    quotas_[c] = q;
}

void Budget::removeQuota(ResourceCategory c) {
    quotas_.erase(c);
}

std::optional<Quota> Budget::quotaFor(ResourceCategory c) const {
    auto it = quotas_.find(c);

    if (it == quotas_.end())
        return std::nullopt;

    return it->second;
}

Usage Budget::usageFor(ResourceCategory c) const {
    auto it = usage_.find(c);

    return it == usage_.end()
        ? Usage{0, Money::fromMinor(0, total_.currency()), 0}
        : it->second;
}

std::optional<int> Budget::unitsRemaining(ResourceCategory c) const {
    auto q = quotaFor(c);

    if (!q)
        return std::nullopt;

    return q->maxUnits - usageFor(c).units;
}

std::optional<Money> Budget::spendRemaining(ResourceCategory c) const {
    auto q = quotaFor(c);

    if (!q)
        return std::nullopt;

    return q->maxSpend - usageFor(c).spent;
}

Budget::Failure Budget::evaluate(
    ResourceCategory c,
    int units,
    Money cost,
    const std::string& resourceId,
    std::string& why) const {

    if (units <= 0) {
        why = "quantity must be positive";
        return Failure::BadInput;
    }

    if (cost.isNegative()) {
        why = "cost must not be negative";
        return Failure::BadInput;
    }

    if (auto left = unitsRemaining(c);
        left && units > *left) {

        why =
            std::string(categoryName(c)) +
            " unit quota exceeded: requested " +
            std::to_string(units) +
            ", " +
            std::to_string(*left) +
            " remaining";

        return Failure::Quota;
    }

    auto q = quotaFor(c);

    if (q && q->maxTitles >= 0 && !resourceId.empty()) {
        auto it = purchasedTitles_.find(c);

        bool alreadyPurchased =
            it != purchasedTitles_.end() &&
            it->second.find(resourceId) != it->second.end();

        int currentTitles =
            it == purchasedTitles_.end()
                ? 0
                : static_cast<int>(it->second.size());

        if (!alreadyPurchased &&
            currentTitles >= q->maxTitles) {

            why =
                std::string(categoryName(c)) +
                " title quota exceeded";

            return Failure::Quota;
        }
    }

    if (auto left = spendRemaining(c);
        left && cost > *left) {

        why =
            std::string(categoryName(c)) +
            " spend quota exceeded: cost " +
            cost.toString() +
            ", " +
            left->toString() +
            " remaining";

        return Failure::Quota;
    }

    if (cost > remaining()) {
        why =
            "overall budget exceeded: cost " +
            cost.toString() +
            ", " +
            remaining().toString() +
            " remaining";

        return Failure::Overall;
    }

    why.clear();
    return Failure::None;
}

std::string Budget::check(
    ResourceCategory c,
    int units,
    Money cost,
    const std::string& resourceId) const {

    std::string why;

    evaluate(
        c,
        units,
        cost,
        resourceId,
        why
    );

    return why;
}

void Budget::commit(
    ResourceCategory c,
    int units,
    Money cost,
    const std::string& resourceId) {

    std::string why;

    switch (evaluate(
        c,
        units,
        cost,
        resourceId,
        why)) {

        case Failure::None:
            break;

        case Failure::BadInput:
            throw std::invalid_argument(why);

        case Failure::Quota:
            throw QuotaExceededError(why);

        case Failure::Overall:
            throw BudgetExceededError(why);
    }

    Usage& u = usage_.try_emplace(
        c,
        Usage{0, Money::fromMinor(0, total_.currency()), 0}
    ).first->second;

    u.units += units;
    u.spent += cost;

    if (!resourceId.empty()) {
    auto& titles = purchasedTitles_[c];
    const auto result = titles.insert(resourceId);

    if (result.second)
        ++u.titles;

    titleUnits_[c][resourceId] += units;
}
    spent_ += cost;
}

void Budget::refund(ResourceCategory c, int units, Money cost,
                    const std::string& resourceId) {
    Usage& u = usage_.try_emplace(
        c,
        Usage{0, Money::fromMinor(0, total_.currency()), 0}
    ).first->second;

    u.units -= units;
    u.spent -= cost;
    spent_ -= cost;

    if (!resourceId.empty()) {
        auto categoryIt = titleUnits_.find(c);

        if (categoryIt != titleUnits_.end()) {
            auto titleIt = categoryIt->second.find(resourceId);

            if (titleIt != categoryIt->second.end()) {
                titleIt->second -= units;

                if (titleIt->second <= 0) {
                    categoryIt->second.erase(titleIt);

                    auto purchasedIt = purchasedTitles_.find(c);

                    if (purchasedIt != purchasedTitles_.end()) {
                        if (purchasedIt->second.erase(resourceId) > 0)
                            --u.titles;
                    }
                }
            }
        }
    }
}

Budget Budget::rollover(int carryPercent) const {
    if (carryPercent < 0 || carryPercent > 100)
        throw std::invalid_argument("carry percent must be between 0 and 100");

    const Money unspent = remaining();

    const std::int64_t carriedMinor =
        (unspent.minorUnits() * carryPercent) / 100;

    Budget nextYear(
        Money::fromMinor(total_.minorUnits() + carriedMinor, total_.currency())
    );

    for (const auto& entry : quotas_) {
        nextYear.setQuota(
            entry.first,
            entry.second
        );
    }

    return nextYear;
}

void Budget::print(std::ostream& os) const {
    os << "Budget: total " << total_
       << ", spent " << spent_
       << ", remaining " << remaining()
       << "\n";

    os << std::left
       << std::setw(22) << "  Category"
       << std::setw(18) << "Units used/max"
       << std::setw(18) << "Titles used/max"
       << "Spend used/max\n";

    for (ResourceCategory c : kAllCategories) {
        const Usage u = usageFor(c);
        const auto q = quotaFor(c);

        const std::string units =
            std::to_string(u.units) +
            "/" +
            (q
                ? std::to_string(q->maxUnits)
                : "-");

        const std::string titles =
            std::to_string(u.titles) +
            "/" +
            (
                q && q->maxTitles >= 0
                    ? std::to_string(q->maxTitles)
                    : "-"
            );

        const std::string spend =
            u.spent.toString() +
            "/" +
            (
                q
                    ? q->maxSpend.toString()
                    : "-"
            );

        os << "  "
           << std::setw(20) << categoryName(c)
           << std::setw(18) << units
           << std::setw(18) << titles
           << spend
           << "\n";
    }
}

}  // namespace bookmgmt