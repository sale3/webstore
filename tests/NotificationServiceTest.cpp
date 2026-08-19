#include "NotificationService.h"

#include <gtest/gtest.h>

class NotificationServiceTest : public ::testing::Test {
protected:
    NotificationService service_;
};

//  invalid orderId tests

class NotificationServiceInvalidOrderIdTest
    : public NotificationServiceTest,
      public ::testing::WithParamInterface<int> {
};

TEST_P(NotificationServiceInvalidOrderIdTest, InvalidValuesExpectFailure) {
    // Arrange
    const int invalidOrderId = GetParam();

    // Act & Assert
    EXPECT_THROW(
        service_.sendOrderConfirmation(invalidOrderId),
        std::invalid_argument
    );
}

INSTANTIATE_TEST_SUITE_P(
    InvalidOrderIds,
    NotificationServiceInvalidOrderIdTest,
    ::testing::Values(0, -5, -999)
);

// valid orderId tests

class NotificationServiceValidOrderIdTest
    : public NotificationServiceTest,
      public ::testing::WithParamInterface<int> {
};

TEST_P(NotificationServiceValidOrderIdTest, ValidValuesExpectSuccess) {
    // Arrange
    const int validOrderId = GetParam();

    // Act
    service_.sendOrderConfirmation(validOrderId);

    // Assert
    EXPECT_EQ(service_.getNotificationCount(), 1);
    EXPECT_TRUE(service_.wasConfirmationSent(validOrderId));
}

INSTANTIATE_TEST_SUITE_P(
    ValidOrderIds,
    NotificationServiceValidOrderIdTest,
    ::testing::Values(1, 1000, 999999)
);

// confirmation sent tests
class NotificationServiceWithConfirmationTest : public NotificationServiceTest {
    protected:
        void SetUp() override {
            service_.sendOrderConfirmation(1000);
        }

};

TEST_F(NotificationServiceWithConfirmationTest,SendOrderConfirmationRecordsOrderId ){
    // Assert
    EXPECT_EQ(service_.getNotificationCount(), 1);
    EXPECT_EQ(service_.getConfirmedOrderIds()[0], 1000);
}

TEST_F(NotificationServiceWithConfirmationTest, SendOrderConfirmationRecordsMultipleOrders) {
    // Act
    service_.sendOrderConfirmation(1001);

    // Assert
    EXPECT_EQ(service_.getNotificationCount(), 2);
}
TEST_F(NotificationServiceWithConfirmationTest, WasConfirmationSentReturnsTrueForSentOrder) {
    // Act & Assert
    EXPECT_TRUE(service_.wasConfirmationSent(1000));
}

TEST_F(NotificationServiceWithConfirmationTest, WasConfirmationSentReturnsFalseForUnknownOrder) {
    // Act & Assert
    EXPECT_FALSE(service_.wasConfirmationSent(1001));
}


TEST_F(NotificationServiceTest, NewServiceHasNoConfirmations) {
    // Act & Assert
    EXPECT_EQ(service_.getNotificationCount(), 0);
    EXPECT_TRUE(service_.getConfirmedOrderIds().empty());
}

TEST_F(NotificationServiceTest, WasConfirmationSentReturnsFalseWhenEmpty) {
    // Act & Assert
    EXPECT_FALSE(service_.wasConfirmationSent(1000));
}