#include "DiscountCalculator.h"
#include <gtest/gtest.h>

class DiscountCalculatorTest : public ::testing::Test {
    protected:
    const DiscountCalculator calculator_{};
};

// Customer type parameterized tests
// tuple<total, customerType, expectedPercentage>
class DiscountCalculatorCustomerTypeTest : public DiscountCalculatorTest, public ::testing::WithParamInterface<std::tuple<double, CustomerType, double>> {
    protected:

};

TEST_P(DiscountCalculatorCustomerTypeTest, CalculatesCorrectDiscountPercentage){

    //Arrange
    const auto [total, customerType, expectedPercentage] = GetParam();

    //Act
    const double percentage = calculator_.calculateDiscountPercentage(total, customerType);

    //Assert
    EXPECT_DOUBLE_EQ(percentage, expectedPercentage);


};

INSTANTIATE_TEST_SUITE_P(
    CustomerTypeTests,
    DiscountCalculatorCustomerTypeTest, 
    ::testing::Values(
        std::make_tuple(200.0, CustomerType::Regular, 0.0),
        std::make_tuple(200.0, CustomerType::Silver, 5.0),
        std::make_tuple(200.0, CustomerType::Gold, 10.0),
        std::make_tuple(200.0, CustomerType::Platinum, 15.0),
        std::make_tuple(1000.0, CustomerType::Regular, 5.0),
        std::make_tuple(999.99, CustomerType::Regular, 0.0),
        std::make_tuple(1500.0, CustomerType::Platinum, 20.0)
    )
);


// Negative total parameterized tests (applying discount, calculating percentage)
class DiscountCalculatorNegativeTotalTest : public DiscountCalculatorTest, public ::testing::WithParamInterface<double> {
    protected:

};

TEST_P(DiscountCalculatorNegativeTotalTest, ThrowsOnNegativeTotalInPercentageCalculation) {
    //Arrange
    const double negativeTotal = GetParam();
    //Act & Assert
    EXPECT_THROW(
        calculator_.calculateDiscountPercentage(negativeTotal, CustomerType::Gold),
        std::invalid_argument
    );


}

TEST_P(DiscountCalculatorNegativeTotalTest, ThrowsOnNegativeTotalInApplyDiscount) {
    //Arrange
    const double negativeTotal = GetParam();
    //Act & Assert
    EXPECT_THROW(
        calculator_.applyDiscount(negativeTotal, CustomerType::Gold),
        std::invalid_argument
    );  
}


INSTANTIATE_TEST_SUITE_P(
    NegativeTotalTests,
    DiscountCalculatorNegativeTotalTest,
    ::testing::Values(
        -0.01,
        -100.0,
        -999.99
    )
);

// Apply bonus parameterized tests

class DiscountCalculatorBonusApplyTest : public DiscountCalculatorTest, public ::testing::WithParamInterface<std::tuple<double, CustomerType, double>> {
    protected:
};


TEST_P(DiscountCalculatorBonusApplyTest, ReturnsCorrectFinalAmount) {
    // Arrange
    const auto [total, customerType, expectedFinalAmount] = GetParam();
    // Act
    const double finalAmount = calculator_.applyDiscount(
        total,
        customerType
    );
    // Assert
    EXPECT_DOUBLE_EQ(finalAmount, expectedFinalAmount);
}

INSTANTIATE_TEST_SUITE_P(
    BonusApplyTests,
    DiscountCalculatorBonusApplyTest,
    ::testing::Values(
        std::make_tuple(200.0, CustomerType::Gold, 180.0),
        std::make_tuple(1200.0, CustomerType::Silver, 1080.0))
);