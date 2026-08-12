#include "Product.h"

#include <gtest/gtest.h>


TEST(ProductTest, ConstructorStoresValidValuesCorrectly) {
    
    const int id = 1;
    const std::string name = "Tastatura";
    const double price = 49.99;

    const Product product(id, name, price);

    
    EXPECT_EQ(product.getId(), id);
    EXPECT_EQ(product.getName(), name);
    EXPECT_DOUBLE_EQ(product.getPrice(), price);
}

TEST(ProductTest, GetIdReturnsConstructorValue) {
    const Product product(42, "Mis", 15.0);

    EXPECT_EQ(product.getId(), 42);
}

TEST(ProductTest, GetNameReturnsConstructorValue) {
    const Product product(1, "Monitor", 199.99);

    EXPECT_EQ(product.getName(), "Monitor");
}

TEST(ProductTest, GetPriceReturnsConstructorValue) {
    const Product product(1, "Slusalice", 89.5);

    EXPECT_DOUBLE_EQ(product.getPrice(), 89.5);
}

TEST(ProductTest, GetNameReturnsConstReference) {
  
    const Product product(1, "Tastatura", 49.99);

    const std::string& firstCall = product.getName();
    const std::string& secondCall = product.getName();

    EXPECT_EQ(&firstCall, &secondCall);
}


TEST(ProductTest, ThrowsWhenIdIsZero) {
    EXPECT_THROW(
        Product(0, "Proizvod", 10.0),
        std::invalid_argument
    );
}

TEST(ProductTest, ThrowsWhenIdIsNegative) {
    EXPECT_THROW(
        Product(-5, "Proizvod", 10.0),
        std::invalid_argument
    );
}

TEST(ProductTest, ThrowsWithCorrectMessageWhenIdIsInvalid) {
    try {
        Product product(0, "Proizvod", 10.0);
        FAIL() << "Ocekivan je izuzetak std::invalid_argument";
    } catch (const std::invalid_argument& ex) {
        EXPECT_STREQ(ex.what(), "Product ID must be positive");
    }
}


TEST(ProductTest, ThrowsWhenNameIsEmpty) {
    EXPECT_THROW(
        Product(1, "", 10.0),
        std::invalid_argument
    );
}

TEST(ProductTest, ThrowsWithCorrectMessageWhenNameIsEmpty) {
    try {
        Product product(1, "", 10.0);
        FAIL() << "Ocekivan je izuzetak std::invalid_argument";
    } catch (const std::invalid_argument& ex) {
        EXPECT_STREQ(ex.what(), "Product name cannot be empty");
    }
}


TEST(ProductTest, ThrowsWhenPriceIsNegative) {
    EXPECT_THROW(
        Product(1, "Proizvod", -0.01),
        std::invalid_argument
    );
}

TEST(ProductTest, ThrowsWithCorrectMessageWhenPriceIsNegative) {
    try {
        Product product(1, "Proizvod", -100.0);
        FAIL() << "Ocekivan je izuzetak std::invalid_argument";
    } catch (const std::invalid_argument& ex) {
        EXPECT_STREQ(ex.what(), "Product price cannot be negative");
    }
}

// цијена = 0 (дозвољено)

TEST(ProductTest, AllowsZeroPriceAsBoundaryValue) {
    
    const Product product(1, "Besplatan proizvod", 0.0);

   
    EXPECT_DOUBLE_EQ(product.getPrice(), 0.0);
}

// гранична вриједност идентификатора

TEST(ProductTest, AllowsSmallestValidId) {
    const Product product(1, "Proizvod", 10.0);

    EXPECT_EQ(product.getId(), 1);
}



TEST(ProductTest, DoesNotThrowForValidArguments) {
    EXPECT_NO_THROW(
        Product(1, "Ispravan proizvod", 1.0)
    );
}