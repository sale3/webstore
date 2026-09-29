#include <mocks/LoggerMock.h>
#include <mocks/MockNotificationService.h>
#include <mocks/MockPaymentService.h>
#include <mocks/MockInventoryService.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <OrderService.h>

using ::testing::_;
using ::testing::HasSubstr;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::StrictMock;

class OrderServiceMockTest : public ::testing::Test
{
protected:
    StrictMock<MockInventoryService> inventory_;
    StrictMock<MockPaymentService> payment_;
    StrictMock<MockNotificationService> notification_;
    StrictMock<MockLogger> logger_;

    static constexpr double kAmount = 50.0;
    static constexpr int kProductId = 1;
    static constexpr int kQuantity = 3;
    static constexpr int kFirstOrderId = 1000;

    OrderService orderService_{inventory_, payment_, notification_, logger_};
};

//----Uspjesno---
TEST_F(OrderServiceMockTest, SuccessfulOrderCallsDependenciesInOrder)
{
    // Arrange
    InSequence sequence;
    EXPECT_CALL(inventory_, isAvailable(kProductId, kQuantity)).WillOnce(Return(true)); //prvi put vrati true
    EXPECT_CALL(payment_, processPayment(kAmount)).WillOnce(Return(true));
    EXPECT_CALL(inventory_, reduceStock(kAmount, kQuantity)).WillOnce(Return(true));
    EXPECT_CALL(notification_, sendOrderConfirmation(kFirstOrderId));
    EXPECT_CALL(logger_, log(HasSubstr(std::to_string(kFirstOrderId))));

    // Act
    const OrderResult result = orderService_.placeOrder(kProductId, kQuantity, kAmount);

    //Assert
    EXPECT_EQ(result, OrderResult::Success);
}

TEST_F(OrderServiceMockTest, EachOrderGetsNextOrderId){
    //Arrange
    EXPECT_CALL(inventory_, isAvailable(_, _)).Times(2).WillRepeatedly(Return(true)); //svaki put vrati true
    EXPECT_CALL(payment_, processPayment(_)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(inventory_, reduceStock(_,_)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(logger_, log(_)).Times(2);

    {
        InSequence sequence;
        EXPECT_CALL(notification_, sendOrderConfirmation(kFirstOrderId));
        EXPECT_CALL(notification_, sendOrderConfirmation(kFirstOrderId + 1));
        
    }

    //Act
    orderService_.placeOrder(kProductId, kQuantity, kAmount);
    orderService_.placeOrder(kProductId, kQuantity, kAmount);
}

//----Neuspjesno----

class OrderServiceMockInvalidInputTest
    : public OrderServiceMockTest,
      public ::testing::WithParamInterface<std::tuple<int, int, double, OrderResult>> {}; //id, quantity, amount, results

      //posto je StrictMock on treba da obori test na bilo koji poziv 
TEST_P(OrderServiceMockInvalidInputTest, RejectsInputWithoutCallingDependencies) {
    // Arrange
    const auto [productId, quantity, amount, expected] = GetParam();

    // Act
    const OrderResult result = orderService_.placeOrder(productId, quantity, amount);

    // Assert
    EXPECT_EQ(result, expected);
}

INSTANTIATE_TEST_SUITE_P(
    InvalidInputs,
    OrderServiceMockInvalidInputTest,
    ::testing::Values(
        std::make_tuple(0,  3, 50.0,  OrderResult::InvalidProduct),
        std::make_tuple(-1, 3, 50.0,  OrderResult::InvalidProduct),
        std::make_tuple(1,  0, 50.0,  OrderResult::InvalidQuantity),
        std::make_tuple(1, -1, 50.0,  OrderResult::InvalidQuantity),
        std::make_tuple(1,  3, 0.0,   OrderResult::InvalidAmount),
        std::make_tuple(1,  3, -50.0, OrderResult::InvalidAmount)
    )
);


//----Neuspjeh u servisima----

TEST_F(OrderServiceMockTest, UnavailableProductStopsBeforePayment){
    //Arrange
    EXPECT_CALL(inventory_, isAvailable(kProductId, kQuantity)).WillOnce(Return(false));
    //Act
    const OrderResult results = orderService_.placeOrder(kProductId, kQuantity, kAmount);

    //Assert
    EXPECT_EQ(results, OrderResult::InvalidProduct);
}

TEST_F(OrderServiceMockTest, FailedPaymentStopsBeforeReducingStock){
    //Arrange
    EXPECT_CALL(inventory_, isAvailable(kProductId, kQuantity)).WillOnce(Return(true));
    EXPECT_CALL(payment_, processPayment(kAmount)).WillOnce(Return(false));
    //Act
    const OrderResult results = orderService_.placeOrder(kProductId, kQuantity, kAmount);
    //Assert
    EXPECT_EQ(results, OrderResult::PaymentFailed);
}

TEST_F(OrderServiceMockTest, FailedStockUpdateStopsBeforeConfirmation){
    //Arrange
    EXPECT_CALL(inventory_, isAvailable(kProductId, kQuantity)).WillOnce(Return(true));
    EXPECT_CALL(payment_, processPayment(kAmount)).WillOnce(Return(true));
    EXPECT_CALL(inventory_, reduceStock(kProductId, kQuantity)).WillOnce(Return(false));
    //Act
    const OrderResult results = orderService_.placeOrder(kProductId, kQuantity, kAmount);
    //Assert
    EXPECT_EQ(results, OrderResult::StockUpdateFailed);
}

