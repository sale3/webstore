#include "Product.h"

#include <gtest/gtest.h>

TEST(ProductInitialTest, ReturnsConstructorValues) {
    const Product product(
        1,
        "Test product",
        25.50
    );

    EXPECT_EQ(product.getId(), 1);
    EXPECT_EQ(product.getName(), "Test product");
    EXPECT_DOUBLE_EQ(product.getPrice(), 25.50);
}

// TODO:
// Студент треба да дода тестове неисправних и
// граничних вриједности.
