#include "DiscountCalculator.h"

#include <stdexcept>

double DiscountCalculator::calculateDiscountPercentage(
    double total,
    CustomerType customerType
) const {
    // ИМПЛ:
    // 1. Одбити негативан износ.
    // 2. Одредити попуст према типу купца.
    // 3. Додати 5% ако је total >= 1000.
    // 4. Ограничити попуст на највише 20%.
	if (total < 0) {
		throw std::invalid_argument("Total amount cannot be negative");
	}
	double discount = 0.0;
	switch (customerType) {
	case CustomerType::Regular:
		discount = 0.0;
		break;
	case CustomerType::Silver:
		discount = 5.0;
		break;
	case CustomerType::Gold:
		discount = 10.0;
		break;
	case CustomerType::Platinum:
		discount = 15.0;
		break;
	default:
		throw std::invalid_argument("Invalid customer type");
	}
	if (total >= 1000) {
		discount += 5.0;
	}
	if (discount > 20.0) {
		discount = 20.0;
	}
	return discount;
}

double DiscountCalculator::applyDiscount(
    double total,
    CustomerType customerType
) const {
    // ИМПЛ: Израчунати коначан износ након попуста.
	double discountPercentage = calculateDiscountPercentage(total, customerType);
	double discountAmount = total * (discountPercentage / 100.0);
	return total - discountAmount;
}
