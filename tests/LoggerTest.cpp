#include "Logger.h"

#include <gtest/gtest.h>
#include <string>
#include <tuple>

class LoggerTest : public ::testing::Test
{
protected:
    Logger logger_{};
};

TEST_F(LoggerTest, NewLoggerHasNoMessages)
{
    // Assert
    EXPECT_EQ(logger_.getMessageCount(), 0);
    EXPECT_TRUE(logger_.getMessages().empty());
}

TEST_F(LoggerTest, LogIncreasesMessageCount)
{
    // Act
    logger_.log("First message");
    logger_.log("Second message");

    // Assert
    EXPECT_EQ(logger_.getMessageCount(), 2);
}

TEST_F(LoggerTest, GetMessagesReturnsMessagesInLoggedOrder)
{
    // Arrange
    logger_.log("A");
    logger_.log("B");
    logger_.log("C");

    // Act
    const auto &messages = logger_.getMessages();

    // Assert
    ASSERT_EQ(messages.size(), 3u);
    EXPECT_EQ(messages[0], "A");
    EXPECT_EQ(messages[1], "B");
    EXPECT_EQ(messages[2], "C");
}

TEST_F(LoggerTest, LogKeepsDuplicateMessages)
{
    // Act
    logger_.log("SameMessage");
    logger_.log("SameMessage");

    // Assert
    EXPECT_EQ(logger_.getMessageCount(), 2);
}

TEST_F(LoggerTest, ContainsReturnsFalseWhenEmpty)
{
    // Act & Assert
    EXPECT_FALSE(logger_.contains("Anything"));
}

class LoggerContainsTest : public LoggerTest,
                           public ::testing::WithParamInterface<std::tuple<std::string, bool>>
{
protected:
    void SetUp() override
    {
        logger_.log("Order placed successfully");
    }
};

TEST_P(LoggerContainsTest, CorrectReturnTest)
{
    // Arrange
    const auto &[message, expected] = GetParam();

    // Act
    const bool result = logger_.contains(message);

    // Assert
    EXPECT_EQ(result, expected);
}

INSTANTIATE_TEST_SUITE_P(ContainsReturnsValues, LoggerContainsTest,
                         ::testing::Values(
                             std::make_tuple("Order placed successfully", true),
                             std::make_tuple("Anything else", false),
                             std::make_tuple("Order placed", false),
                             std::make_tuple("order placed successfully", false),
                             std::make_tuple("", false)));
