#pragma once

enum class CustomerType {
    Regular,
    Silver,
    Gold,
    Platinum
};

class DiscountCalculator {
public:
    double calculateDiscountPercentage(
        double total,
        CustomerType customerType
    ) const;

    double applyDiscount(
        double total,
        CustomerType customerType
    ) const;
};
