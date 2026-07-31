#include "DiscountCalculator.h"

#include <stdexcept>

double DiscountCalculator::calculateDiscountPercentage(
    double total,
    CustomerType customerType
) const {
    // TODO:
    // 1. Одбити негативан износ.
    // 2. Одредити попуст према типу купца.
    // 3. Додати 5% ако је total >= 1000.
    // 4. Ограничити попуст на највише 20%.
    static_cast<void>(total);
    static_cast<void>(customerType);

    throw std::logic_error(
        "Discount calculation is not implemented"
    );
}

double DiscountCalculator::applyDiscount(
    double total,
    CustomerType customerType
) const {
    // TODO: Израчунати коначан износ након попуста.
    static_cast<void>(total);
    static_cast<void>(customerType);

    throw std::logic_error(
        "Discount application is not implemented"
    );
}
