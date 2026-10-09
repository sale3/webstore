#include "Cart.h"
#include "Product.h"
#include <gtest/gtest.h>

class CartTest: public::testing::Test{
    protected:
        Cart cart;
        const Product product_{1, "Test product", 100.0};
        const Product otherProduct_{2, "Test product 2", 45.0};
};

// invalid quantity
class AddingInvalidQuantityTest: public CartTest, public::testing::WithParamInterface<int> {};

TEST_P(AddingInvalidQuantityTest, InvalidQuantityThrows) {
   //Arrange
    const int quantity = GetParam();
    
    //Act & Assert
    EXPECT_THROW(cart.addProduct(product_, quantity), std::invalid_argument);
};

INSTANTIATE_TEST_SUITE_P(
    InvalidQuantities,
    AddingInvalidQuantityTest,
    ::testing::Values(0,-5)
);

//valid quantity
class AddingValidQuantityTest: public CartTest, public::testing::WithParamInterface<int> {};

TEST_P(AddingValidQuantityTest, ValidQuantityAddsProduct) {
    //Arrange
    const int quantity = GetParam();
    //Act
    cart.addProduct(product_, quantity);
    //Assert
    EXPECT_EQ(cart.getItemCount(), quantity);
    EXPECT_EQ(cart.getDifferentProductCount(), 1u);
}

INSTANTIATE_TEST_SUITE_P(
    ValidQuantities, 
    AddingValidQuantityTest, 
    ::testing::Values(1, 5, 10)
);

// remove product tests

class RemovingProductTest: public CartTest, public::testing::WithParamInterface<std::tuple<int, bool, int>> {};

TEST_P(RemovingProductTest, RemoveBehavesAsExpected) {
    //Arrange
    const auto& [paramProductId, expectedRemoved, expectedItemCountAfter] = GetParam();
    cart.addProduct(product_, 2);
    //Act
    const bool removed = cart.removeProduct(paramProductId);
    //Assert
    EXPECT_EQ(removed, expectedRemoved);
    EXPECT_EQ(cart.getItemCount(), expectedItemCountAfter);
}

INSTANTIATE_TEST_SUITE_P(
    RemoveScenarios,
    RemovingProductTest,
    ::testing::Values(
        std::make_tuple(1, true, 0),      
        std::make_tuple(999, false, 2)   
    )
);


TEST_F(CartTest, NewCartIsEmpty) {
    //Arrange & Act & Assert
    EXPECT_TRUE(cart.isEmpty());
    EXPECT_EQ(cart.getItemCount(), 0);
    EXPECT_EQ(cart.getDifferentProductCount(), 0u);
    EXPECT_DOUBLE_EQ(cart.calculateTotal(), 0.0);
}


TEST_F(CartTest, AddingSameProductIncreasesQuantity) {
    //Arrange & Act
    cart.addProduct(product_, 2);
    cart.addProduct(product_, 3);

    //Assert
    EXPECT_EQ(cart.getItemCount(), 5);
    EXPECT_EQ(cart.getDifferentProductCount(), 1u);
}

TEST_F(CartTest, AddingDifferentProductsCountsSeparately) {
    //Arrange &Act
    cart.addProduct(product_, 1);
    cart.addProduct(otherProduct_, 1);
    //Assert
    EXPECT_EQ(cart.getItemCount(), 2);
    EXPECT_EQ(cart.getDifferentProductCount(), 2u);
}


TEST_F(CartTest, CalculatesTotalAcrossMultipleProducts) {
    //Arrange 
    cart.addProduct(product_, 2);
    cart.addProduct(otherProduct_, 1);
    //Act
    const double total = cart.calculateTotal();

    //Assert
    EXPECT_DOUBLE_EQ(total, 245.0);
}


TEST_F(CartTest, RemovingOneProductKeepsOthers) {
    //Arrange
    cart.addProduct(product_, 1);
    cart.addProduct(otherProduct_, 1);
    //Act
    cart.removeProduct(product_.getId());
    //Assert
    EXPECT_EQ(cart.getDifferentProductCount(), 1u);
    EXPECT_DOUBLE_EQ(cart.calculateTotal(), 45.0);
}

TEST_F(CartTest, ClearEmptiesTheCart) {
    //Arrange 
    cart.addProduct(product_, 2);
    cart.addProduct(otherProduct_, 1);
    //Act
    cart.clear();
    //Assert
    EXPECT_TRUE(cart.isEmpty());
    EXPECT_EQ(cart.getItemCount(), 0);
    EXPECT_EQ(cart.getDifferentProductCount(), 0u);
}

TEST_F(CartTest, GetItemsReturnsCorrectContents) {
    // Arrange & Act
    cart.addProduct(product_, 2);
    cart.addProduct(otherProduct_, 1);

    // Assert
    const auto& items = cart.getItems();
    ASSERT_EQ(items.size(), 2u);
    EXPECT_EQ(items[0].product.getId(), product_.getId());
    EXPECT_EQ(items[0].quantity, 2);
    EXPECT_EQ(items[1].product.getId(), otherProduct_.getId());
    EXPECT_EQ(items[1].quantity, 1);
}