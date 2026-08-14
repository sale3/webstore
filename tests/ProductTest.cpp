#include "Product.h"

#include <gtest/gtest.h>

TEST(ProductTest, ThrowsOnNegativeId) {
     
    const int invalidId = -1;
    const std::string name = "Test product";
    const double price = 25.50;

     
    EXPECT_THROW(
        Product(invalidId, name, price),
        std::invalid_argument
    );
}

TEST(ProductTest, ThrowsOnZeroId) {
     
    const int invalidId = 0;
    const std::string name = "Test product";
    const double price = 25.50;

     
    EXPECT_THROW(
        Product(invalidId, name, price),
        std::invalid_argument
    );
}

TEST(ProductTest, ThrowsOnEmptyName) {
     
    const int id = 1;
    const std::string emptyName = "";
    const double price = 25.50;

     
    EXPECT_THROW(
        Product(id, emptyName, price),
        std::invalid_argument
    );
}

TEST(ProductTest, ThrowsOnNegativePrice) {
     
    const int id = 1;
    const std::string name = "Test product";
    const double invalidPrice = -5.0;

     
    EXPECT_THROW(
        Product(id, name, invalidPrice),
        std::invalid_argument
    );
}

TEST(ProductTest, AllowsZeroPrice) {
     
    const int id = 1;
    const std::string name = "Free sample";
    const double zeroPrice = 0.0;

     
    const Product product(id, name, zeroPrice);

     
    EXPECT_DOUBLE_EQ(product.getPrice(), 0.0);
}

TEST(ProductTest, AllowsPositiveId) {
     
    const int id = 1;
    const std::string name = "Boundary product";
    const double price = 10.0;

     
    const Product product(id, name, price);

     
    EXPECT_EQ(product.getId(), 1);
}