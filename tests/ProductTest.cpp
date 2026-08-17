#include "Product.h"

#include <gtest/gtest.h>

TEST(ProductTest, ThrowsOnNegativeId) {
    //Arrange
    const int invalidId = -1;
    const std::string name = "Test product";
    const double price = 25.50;

    //Act & Assert
    EXPECT_THROW(
        Product(invalidId, name, price),
        std::invalid_argument
    );
}

TEST(ProductTest, ThrowsOnZeroId) {
    //Arrange
    const int invalidId = 0;
    const std::string name = "Test product";
    const double price = 25.50;

    //Act & Assert
    EXPECT_THROW(
        Product(invalidId, name, price),
        std::invalid_argument
    );
}

TEST(ProductTest, ThrowsOnEmptyName) {
    //Arrange
    const int id = 1;
    const std::string emptyName = "";
    const double price = 25.50;

    //Act & Assert
    EXPECT_THROW(
        Product(id, emptyName, price),
        std::invalid_argument
    );
}

TEST(ProductTest, ThrowsOnNegativePrice) {
    //Arrange
    const int id = 1;
    const std::string name = "Test product";
    const double invalidPrice = -5.0;

    //Act & Assert
    EXPECT_THROW(
        Product(id, name, invalidPrice),
        std::invalid_argument
    );
}

TEST(ProductTest, AllowsZeroPrice) {
    //Arrange
    const int id = 1;
    const std::string name = "Free sample";
    const double zeroPrice = 0.0;

    //Act
    const Product product(id, name, zeroPrice);

    //Assert
    EXPECT_DOUBLE_EQ(product.getPrice(), 0.0);
}

TEST(ProductTest, AllowsPositiveId) {
    //Arrange
    const int id = 1;
    const std::string name = "Boundary product";
    const double price = 10.0;

    //Act
    const Product product(id, name, price);

    //Assert 
    EXPECT_EQ(product.getId(), 1);
}