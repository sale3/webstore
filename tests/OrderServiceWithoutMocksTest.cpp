#include "InventoryService.h"
#include "Logger.h"
#include "NotificationService.h"
#include "OrderService.h"
#include "PaymentService.h"

#include <gtest/gtest.h>

class OrderServiceWithoutMocksTest : public ::testing::Test {
protected:
    static constexpr int kProductId = 1;
    static constexpr int kInitialStock = 10;
    static constexpr double kTransactionLimit = 1000.0;
    static constexpr double kValidAmount = 50.0;
    static constexpr double kAmountOverLimit = 5000.0;

    void SetUp() override {
    //Initial state
    ASSERT_EQ(paymentService_.getProcessedPaymentCount(), 0);
    ASSERT_DOUBLE_EQ(paymentService_.getLastProcessedAmount(), 0.0);
    ASSERT_EQ(notificationService_.getNotificationCount(), 0);
    ASSERT_TRUE(notificationService_.getConfirmedOrderIds().empty());
    ASSERT_EQ(logger_.getMessageCount(), 0);

    inventoryService_.setStock(kProductId, kInitialStock);
    ASSERT_EQ(inventoryService_.getStock(kProductId), kInitialStock);
}

    InventoryService inventoryService_;
    PaymentService paymentService_{kTransactionLimit};
    NotificationService notificationService_;
    Logger logger_;


    void ExpectNothingChanged() const {
        EXPECT_EQ(inventoryService_.getStock(kProductId), kInitialStock);
        EXPECT_EQ(paymentService_.getProcessedPaymentCount(), 0);
        EXPECT_DOUBLE_EQ(paymentService_.getLastProcessedAmount(), 0.0);
        EXPECT_EQ(notificationService_.getNotificationCount(), 0);
        EXPECT_TRUE(notificationService_.getConfirmedOrderIds().empty());
        EXPECT_EQ(logger_.getMessageCount(), 0);
    }

    OrderService orderService_{
        inventoryService_,
        paymentService_,
        notificationService_,
        logger_

    };
};

// ---Uspjesna narudzba----

TEST_F(OrderServiceWithoutMocksTest, SuccessfulOrderReturnsSuccess){
    //Act
    const OrderResult result = orderService_.placeOrder(kProductId, 3, kValidAmount);
    //Assert
    EXPECT_EQ(result, OrderResult::Success); 
}

TEST_F(OrderServiceWithoutMocksTest, SmallPositiveAmountIsAccepted) {
    // Act
    const OrderResult result = orderService_.placeOrder(kProductId, 1, 0.5);

    // Assert
    EXPECT_EQ(result, OrderResult::Success);
}

TEST_F(OrderServiceWithoutMocksTest, SuccessfulOrderReducesStock) {
    // Arrange
    ASSERT_EQ(inventoryService_.getStock(kProductId), kInitialStock);

    // Act
    orderService_.placeOrder(kProductId, 3, kValidAmount);

    // Assert
    EXPECT_EQ(inventoryService_.getStock(kProductId), 7);
}

TEST_F(OrderServiceWithoutMocksTest, SuccessfulOrderProcessesPayment) {
    // Arrange
    ASSERT_EQ(paymentService_.getProcessedPaymentCount(), 0);
    ASSERT_DOUBLE_EQ(paymentService_.getLastProcessedAmount(), 0.0);

    // Act
    orderService_.placeOrder(kProductId, 3, kValidAmount);

    // Assert
    EXPECT_EQ(paymentService_.getProcessedPaymentCount(), 1);
    EXPECT_DOUBLE_EQ(paymentService_.getLastProcessedAmount(), kValidAmount);
}

TEST_F(OrderServiceWithoutMocksTest, SuccessfulOrderSendsConfirmation) {
    // Arrange
    ASSERT_EQ(notificationService_.getNotificationCount(), 0);

    // Act
    orderService_.placeOrder(kProductId, 3, kValidAmount);

    // Assert
    EXPECT_EQ(notificationService_.getNotificationCount(), 1);
}

TEST_F(OrderServiceWithoutMocksTest, SuccessfulOrderWritesLogMessage) {
    // Arrange
    ASSERT_EQ(logger_.getMessageCount(), 0);

    // Act
    orderService_.placeOrder(kProductId, 3, kValidAmount);

    // Assert
    ASSERT_EQ(logger_.getMessageCount(), 1);
    EXPECT_EQ(logger_.getMessages()[0], "Order placed successfully. Order ID: 1000");
}


TEST_F(OrderServiceWithoutMocksTest, GeneratedOrderIdsAreUniquePerCall) {
    // Arrange
    ASSERT_TRUE(notificationService_.getConfirmedOrderIds().empty());

    // Act
    orderService_.placeOrder(kProductId, 1, 10.0);
    orderService_.placeOrder(kProductId, 1, 10.0);

    // Assert
    const auto& confirmedIds = notificationService_.getConfirmedOrderIds();
    ASSERT_EQ(confirmedIds.size(), 2u);
    EXPECT_NE(confirmedIds[0], confirmedIds[1]);
}

// --- Granicne vrijednost/slucajevi ---

TEST_F(OrderServiceWithoutMocksTest, OrderingEntireStockSucceeds) {
    // Arrange
    ASSERT_EQ(inventoryService_.getStock(kProductId), kInitialStock);

    // Act
    const OrderResult result = orderService_.placeOrder(kProductId, kInitialStock, kValidAmount);

    // Assert
    EXPECT_EQ(result, OrderResult::Success);
    EXPECT_EQ(inventoryService_.getStock(kProductId), 0);
}

TEST_F(OrderServiceWithoutMocksTest, OrderingOneMoreThanStockFails) {
    // Act
    const OrderResult result = orderService_.placeOrder(kProductId, kInitialStock + 1, kValidAmount);

    // Assert
    EXPECT_EQ(result, OrderResult::ProductUnavailable);
    ExpectNothingChanged();
}

TEST_F(OrderServiceWithoutMocksTest, AmountEqualToLimitSucceeds) {
    // Act
    const OrderResult result = orderService_.placeOrder(kProductId, 1, kTransactionLimit);

    // Assert
    EXPECT_EQ(result, OrderResult::Success);
}

TEST_F(OrderServiceWithoutMocksTest, AmountJustAboveLimitFails) {
    // Act
    const OrderResult result = orderService_.placeOrder(kProductId, 1, kTransactionLimit + 0.01);

    // Assert
    EXPECT_EQ(result, OrderResult::PaymentFailed);
}

// --- Neispavni ulazi ---
class OrderServiceInvalidInputTest : public OrderServiceWithoutMocksTest,
                                     public ::testing::WithParamInterface<std::tuple<
                                     int,                   //productId
                                     int,                   //ordered quantity
                                     double,                //payment amount
                                     OrderResult>> {};      //expected result of placeOrder()

TEST_P(OrderServiceInvalidInputTest, RejectsInputAndCausesNoSideEffects) {
    // Arrange
    const auto [productId, quantity, amount, expected] = GetParam();

    // Act
    const OrderResult result = orderService_.placeOrder(productId, quantity, amount);

    // Assert
    EXPECT_EQ(result, expected);
    ExpectNothingChanged();
}

INSTANTIATE_TEST_SUITE_P(
    InvalidInputs,
    OrderServiceInvalidInputTest,
    ::testing::Values(
        std::make_tuple(0,  3, 50.0,  OrderResult::InvalidProduct), // id = 0
        std::make_tuple(-1, 3, 50.0,  OrderResult::InvalidProduct), // id < 0
        std::make_tuple(1,  0, 50.0,  OrderResult::InvalidQuantity),// quantity = 0
        std::make_tuple(1, -1, 50.0,  OrderResult::InvalidQuantity),// quantity < 0
        std::make_tuple(1,  3, 0.0,   OrderResult::InvalidAmount),  // amount = 0 
        std::make_tuple(1,  3, -50.0, OrderResult::InvalidAmount)   // amount < 0
    )
);

// --- Neuspjesno, kasnije ---
TEST_F(OrderServiceWithoutMocksTest, UnavailableProductCausesNoSideEffects) {
    // Act
    const OrderResult result = orderService_.placeOrder(kProductId, 100, kValidAmount);

    // Assert
    EXPECT_EQ(result, OrderResult::ProductUnavailable);
    ExpectNothingChanged();
}

TEST_F(OrderServiceWithoutMocksTest, PaymentOverLimitStopsFurtherSteps) {
    // Act
    const OrderResult result = orderService_.placeOrder(kProductId, 3, kAmountOverLimit);

    // Assert
    EXPECT_EQ(result, OrderResult::PaymentFailed);
    ExpectNothingChanged();
}

TEST_F(OrderServiceWithoutMocksTest, UnavailablePaymentServiceStopsFurtherSteps) {
    // Arrange
    paymentService_.setServiceAvailable(false);

    // Act

    const OrderResult result = orderService_.placeOrder(kProductId, 3, kValidAmount);

    // Assert
    EXPECT_EQ(result, OrderResult::PaymentFailed);
    ExpectNothingChanged();
}
