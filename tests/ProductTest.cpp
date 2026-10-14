#include "Product.h"

#include <gtest/gtest.h>

#include <string>
#include <tuple>

class ProductTest : public ::testing::Test {
    protected:
};


//invalid argument tests
class ProductInvalidArgumentTest
    : public ProductTest,                                    
      public ::testing::WithParamInterface<std::tuple<int, std::string, double>> {
};

TEST_P(ProductInvalidArgumentTest, InvalidValuesExpectFail) {
    const auto& [id, name, price] = GetParam();
    EXPECT_THROW(Product(id, name, price), std::invalid_argument);
};

INSTANTIATE_TEST_SUITE_P(
    InvalidConstructorArguments,          
    ProductInvalidArgumentTest,          
    ::testing::Values(                    
        std::make_tuple(-1, "Test product", 25.50), //ThrowsOnNegativeId
        std::make_tuple(0, "Test product", 25.50),  //ThrowsOnZeroId
        std::make_tuple(1, "", 25.50),              //ThrowsOnEmptyName
        std::make_tuple(1, "Test product", -5.0)   //ThrowsOnNegativePrice
    )
);

//valid argument tests

class ProductValidArgumentTest
    : public ProductTest,                                    
      public ::testing::WithParamInterface<std::tuple<int, std::string, double>> {
};  

TEST_P(ProductValidArgumentTest, ValidValuesExpectSuccess) {
    const auto& [id, name, price] = GetParam();

    const Product product(id, name, price);

    EXPECT_EQ(product.getId(), id);
    EXPECT_EQ(product.getName(), name);
    EXPECT_DOUBLE_EQ(product.getPrice(), price);
}

INSTANTIATE_TEST_SUITE_P(ValidConstructorArguments, ProductValidArgumentTest, ::testing::Values(
    std::make_tuple(1, "Test product", 25.50),  //AllowsPositiveId
    std::make_tuple(2, "Free item", 0.0),       //AllowsZeroPrice ???
    std::make_tuple(999, "Test product", 100.0), //AllowsLargeId
    std::make_tuple(3, "Test product", 9999.99)  //AllowsLargePrice
));
