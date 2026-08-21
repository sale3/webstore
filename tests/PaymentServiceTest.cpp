#include "PaymentService.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <tuple>


class PaymentServiceTest : public ::testing::Test {
    protected:
    PaymentService service_{1000.0};
};

//konstruktor test
class PaymentServiceConstructorTest : public ::testing::Test, public ::testing::WithParamInterface<double> {};

TEST_P(PaymentServiceConstructorTest, ThrowsOnNonPositiveTransactionLimit) {
    //Arrange
    const double invalidLimit = GetParam();
    //Act & Assert
    EXPECT_THROW(
        PaymentService(invalidLimit),
        std::invalid_argument
    );
}

INSTANTIATE_TEST_SUITE_P(
    InvalidTransactionLimits,
    PaymentServiceConstructorTest,
    ::testing::Values(0.0, -1.0, -100.0)
);

class PaymentServiceProcessPaymentTest : public PaymentServiceTest, public ::testing::WithParamInterface<std::tuple<double, bool, bool>> {
    protected:
    void SetUp() override {
        const auto& [amount, serviceAvailable, expectedResult] = GetParam();
        service_.setServiceAvailable(serviceAvailable);
    }
};

TEST_P(PaymentServiceProcessPaymentTest, ReturnsExpectedResult) {
    //Arrange
    const auto& [amount, serviceAvailable, expectedResult] = GetParam();
    //Act
    const bool result = service_.processPayment(amount);
    //Assert
    EXPECT_EQ(result, expectedResult);
}

INSTANTIATE_TEST_SUITE_P(
    ProcessPaymentScenarios,
    PaymentServiceProcessPaymentTest,
    ::testing::Values(
        std::make_tuple(0.0, true, false),       //  == 0 
        std::make_tuple(-50.0, true, false),     // <0 
        std::make_tuple(1500.0, true, false),    // >1000
        std::make_tuple(1000.0, true, true),     // == 1000
        std::make_tuple(500.0, false, false),    // nedostupan service
        std::make_tuple(500.0, true, true)       // valid 
    )
);


//fixtures
TEST_F(PaymentServiceTest, NewServiceIsAvailableByDefault) {
    //Act & Assert
    EXPECT_TRUE(service_.isServiceAvailable());
}

TEST_F(PaymentServiceTest, NewServiceHasNoProcessedPayments) {
    //Act & Assert
    EXPECT_EQ(service_.getProcessedPaymentCount(), 0);
    EXPECT_DOUBLE_EQ(service_.getLastProcessedAmount(), 0.0);
}

TEST_F(PaymentServiceTest, SuccessfulPaymentUpdatesStatistics) {
    //Act
    const bool result = service_.processPayment(250.0);
    //Assert
    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(service_.getLastProcessedAmount(), 250.0);
    EXPECT_EQ(service_.getProcessedPaymentCount(), 1);
}

TEST_F(PaymentServiceTest, ProcessedPaymentCountAccumulates) {
    //Act
    service_.processPayment(100.0);
    service_.processPayment(200.0);
    //Assert
    EXPECT_EQ(service_.getProcessedPaymentCount(), 2);
    EXPECT_DOUBLE_EQ(service_.getLastProcessedAmount(), 200.0);
}

TEST_F(PaymentServiceTest, FailedPaymentDoesNotUpdateStatistics) {
    //Arrange
    service_.processPayment(100.0);
    //Act
    const bool result = service_.processPayment(-50.0);
    //Assert
    EXPECT_FALSE(result);
    EXPECT_EQ(service_.getProcessedPaymentCount(), 1);
    EXPECT_DOUBLE_EQ(service_.getLastProcessedAmount(), 100.0);
}