#include "Logger.h"

#include <gtest/gtest.h>
#include <string>


class LoggerTest: public ::testing::Test {
    protected:
    Logger logger_{};
};

TEST_F(LoggerTest, NewLoggerHasNoMessages) {
    EXPECT_EQ(logger_.getMessageCount(), 0);
    EXPECT_TRUE(logger_.getMessages().empty());
}

TEST_F(LoggerTest, LogAddsMessageInOrder) {
    logger_.log("First message");
    EXPECT_EQ(logger_.getMessageCount(), 1);
    logger_.log("Second message");
    EXPECT_EQ(logger_.getMessageCount(), 2);

    EXPECT_EQ(logger_.getMessages()[0], "First message");
    EXPECT_EQ(logger_.getMessages()[1], "Second message");

}

class LoggerContainsReturnsTest : public LoggerTest, public ::testing::WithParamInterface<std::tuple<bool,std::string>>{
    protected:
    Logger logger_;
    
    void SetUp() override{
        logger_.log("Order placed successfully");
    }
};


TEST_P(LoggerContainsReturnsTest, CorrectReturnTest ){
    //Arrange
    const auto[isCorrect, message] = GetParam();
    //Act & Assertt
    EXPECT_EQ(isCorrect, logger_.contains(message));
}

INSTANTIATE_TEST_SUITE_P(ContainsReturnsValues, LoggerContainsReturnsTest, ::testing::Values(
    std::make_tuple(true, "Order placed successfully"),
    std::make_tuple(false, "Anything else")
));

TEST_F(LoggerTest, ContainsReturnsFalseWhenEmpty) {
    EXPECT_FALSE(logger_.contains("Anything"));
}

TEST_F(LoggerTest, GetMessagesReturnsAllLoggedMessages) {
    logger_.log("A");
    logger_.log("B");
    logger_.log("C");

    const auto& messages = logger_.getMessages();
    EXPECT_EQ(messages.size(), 3u);
}