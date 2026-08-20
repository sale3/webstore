#include "InventoryService.h"
#include <gtest/gtest.h>
#include <tuple>
#include <stdexcept>

class InventoryServiceTest : public ::testing::Test {
protected:
    InventoryService service_;
};

//invalid arg
class InventoryServiceInvalidArgumentTest : public InventoryServiceTest, public ::testing::WithParamInterface<std::tuple<int, int>> {};
 
TEST_P(InventoryServiceInvalidArgumentTest, InvalidValuesExpectFail) {
    //Arrange
    const auto& [id, quantity] = GetParam();
    //Act & Assert
    EXPECT_THROW(
        service_.setStock(id, quantity),
        std::invalid_argument
    );
}
 
INSTANTIATE_TEST_SUITE_P(
    InvalidSetStockArguments,
    InventoryServiceInvalidArgumentTest,
    ::testing::Values(
        std::make_tuple(0, 10),
        std::make_tuple(-1, 10),
        std::make_tuple(1, -1)
    )
);

//valid arg
class InventoryServiceValidArgumentTest: public InventoryServiceTest,public ::testing::WithParamInterface<std::tuple<int, int>> {};
 
TEST_P(InventoryServiceValidArgumentTest, ValidValuesExpectSuccess){
    //Arrange
    const auto &[id, stockToSet] = GetParam();
    //Act
    service_.setStock(id, stockToSet);
    //Assert
    EXPECT_EQ(service_.getStock(id), stockToSet);

}

INSTANTIATE_TEST_SUITE_P(ValidSetStockArguments,
    InventoryServiceValidArgumentTest,
    ::testing::Values(
    std::make_tuple(1,0),
    std::make_tuple(1,10)
));

//stock - isAvialible

class InventoryServiceStockTest
    : public InventoryServiceTest, public ::testing::WithParamInterface<std::tuple<bool, int, int, int>> {
    protected:
    void SetUp() override {
        const auto& [expectedResult, id, stockToSet, requestedQuantity] = GetParam();
        service_.setStock(id, stockToSet);
    }
};
 
TEST_P(InventoryServiceStockTest, AvailabilityMatchesExpectedResult) {
    //Arrange
    const auto& [expectedResult, id, stockToSet, requestedQuantity] = GetParam();
    //Act & Assert
    EXPECT_EQ(service_.isAvailable(id, requestedQuantity), expectedResult);
}
 
INSTANTIATE_TEST_SUITE_P(
    AvailabilityScenarios,
    InventoryServiceStockTest,
    ::testing::Values(
        std::make_tuple(true, 1, 5, 5),    
        std::make_tuple(true, 1, 10, 5),   
        std::make_tuple(false, 1, 5, 6)   //stanje<trazeno
    )
);

//reduce stock -- valid
 
class InventoryServiceReduceStockValidQuantityTest : public InventoryServiceTest, public ::testing::WithParamInterface<std::tuple<bool, int, int, int,int>>{
    protected:
    void SetUp() override{
        const auto& [expectedReduceResult, id, stockToSet, reduceValue, expectedStockAfter] = GetParam();
        service_.setStock(id, stockToSet);
    }
};

TEST_P(InventoryServiceReduceStockValidQuantityTest, ReduceStockTest){
    //Arrange
    const auto& [expectedReduceResult, id, stockToSet, reduceValue, expectedStockAfter] = GetParam();
    //Act
    const bool result = service_.reduceStock(id, reduceValue);
    //Assert
    EXPECT_EQ(result, expectedReduceResult);
    EXPECT_EQ(service_.getStock(id), expectedStockAfter);

}

INSTANTIATE_TEST_SUITE_P(ReduceStockTest,InventoryServiceReduceStockValidQuantityTest, ::testing::Values(
    std::make_tuple(true, 1, 10, 3, 7),
    std::make_tuple(false, 1, 2, 5, 2)
));


//reduce stock -- invalid
class InventoryServiceReduceStockInvalidQuantityTest : public InventoryServiceTest, public ::testing::WithParamInterface<int> {
protected:
    void SetUp() override {
        service_.setStock(1, 10);
    }
};

TEST_P(InventoryServiceReduceStockInvalidQuantityTest, ThrowsOnNonPositiveQuantity) {
    // Arrange
    const int invalidQuantity = GetParam();
    // Act & Assert
    EXPECT_THROW(
        service_.reduceStock(1, invalidQuantity),
        std::invalid_argument
    );
}

INSTANTIATE_TEST_SUITE_P(
    InvalidReduceQuantities,
    InventoryServiceReduceStockInvalidQuantityTest,
    ::testing::Values(0, -1, -100)
);

//fixtures
TEST_F(InventoryServiceTest, SetStockOverwritesPreviousValue) {
    //Act
    service_.setStock(1, 10);
    service_.setStock(1, 3);
    //Assert
    EXPECT_EQ(service_.getStock(1), 3);
}

TEST_F(InventoryServiceTest, GetStockReturnsZeroForUnknownProduct) {
    //Assert
    EXPECT_EQ(service_.getStock(999), 0);
}

TEST_F(InventoryServiceTest, IsAvailableReturnsFalseForUnknownProduct) {
    //Assert
    EXPECT_FALSE(service_.isAvailable(999, 1));
}

TEST_F(InventoryServiceTest, ReduceStockFailsForUnknownProduct) {
    //Act
    const bool result = service_.reduceStock(999, 1);
    //Assert
    EXPECT_FALSE(result);
}

TEST_F(InventoryServiceTest, ReduceStockInvalidQuantityDoesNotChangeStock) {
    // Arrange
    service_.setStock(1, 10);
    // Act
    try {
        service_.reduceStock(1, -5);
    } catch (const std::invalid_argument&) {}
    // Assert
    EXPECT_EQ(service_.getStock(1), 10);
}