#include "Cart.h"
#include "Product.h"
#include <gtest/gtest.h>


TEST(CartTest, NewCartIsEmpty) {
      
    Cart cart;

    EXPECT_TRUE(cart.isEmpty());
    EXPECT_EQ(cart.getItemCount(), 0);
    EXPECT_EQ(cart.getDifferentProductCount(), 0u);
    EXPECT_DOUBLE_EQ(cart.calculateTotal(), 0.0);
}

TEST(CartTest, AddingNewProductStoresIt) {
      
    Cart cart;
    const Product product(1, "Mouse", 25.0);

    cart.addProduct(product, 2);

    EXPECT_FALSE(cart.isEmpty());
    EXPECT_EQ(cart.getItemCount(), 2);
    EXPECT_EQ(cart.getDifferentProductCount(), 1u);
}

TEST(CartTest, AddingSameProductIncreasesQuantity) {
      
    Cart cart;
    const Product product(1, "Mouse", 25.0);
    cart.addProduct(product, 2);

    cart.addProduct(product, 3);

    EXPECT_EQ(cart.getItemCount(), 5);
    EXPECT_EQ(cart.getDifferentProductCount(), 1u);
}

TEST(CartTest, AddingDifferentProductsCountsSeparately) {
      
    Cart cart;
    const Product mouse(1, "Mouse", 25.0);
    const Product keyboard(2, "Keyboard", 45.0);

    cart.addProduct(mouse, 1);
    cart.addProduct(keyboard, 1);

    EXPECT_EQ(cart.getItemCount(), 2);
    EXPECT_EQ(cart.getDifferentProductCount(), 2u);
}

TEST(CartTest, ThrowsOnZeroQuantity) {
      
    Cart cart;
    const Product product(1, "Mouse", 25.0);

    EXPECT_THROW(
        cart.addProduct(product, 0),
        std::invalid_argument
    );
}

TEST(CartTest, ThrowsOnNegativeQuantity) {
      
    Cart cart;
    const Product product(1, "Mouse", 25.0);

    EXPECT_THROW(
        cart.addProduct(product, -1),
        std::invalid_argument
    );
}

TEST(CartTest, CalculatesTotalAcrossMultipleProducts) {
      
    Cart cart;
    const Product mouse(1, "Mouse", 25.0);
    const Product keyboard(2, "Keyboard", 45.0);
    cart.addProduct(mouse, 2);
    cart.addProduct(keyboard, 1);
  
    const double total = cart.calculateTotal();

    EXPECT_DOUBLE_EQ(total, 95.0);
}

TEST(CartTest, RemovingExistingProductReturnsTrue) {
      
    Cart cart;
    const Product product(1, "Mouse", 25.0);
    cart.addProduct(product, 2);
  
    const bool removed = cart.removeProduct(1);
    
    EXPECT_TRUE(removed);
    EXPECT_TRUE(cart.isEmpty());
}

TEST(CartTest, RemovingNonExistingProductReturnsFalse) {
      
    Cart cart;
    const Product product(1, "Mouse", 25.0);
    cart.addProduct(product, 2);
 
    const bool removed = cart.removeProduct(999);
   
    EXPECT_FALSE(removed);
    EXPECT_EQ(cart.getItemCount(), 2);
}

TEST(CartTest, RemovingOneProductKeepsOthers) {
      
    Cart cart;
    const Product mouse(1, "Mouse", 25.0);
    const Product keyboard(2, "Keyboard", 45.0);
    cart.addProduct(mouse, 1);
    cart.addProduct(keyboard, 1);
  
    cart.removeProduct(1);
 
    EXPECT_EQ(cart.getDifferentProductCount(), 1u);
    EXPECT_DOUBLE_EQ(cart.calculateTotal(), 45.0);
}

TEST(CartTest, ClearEmptiesTheCart) {
      
    Cart cart;
    const Product mouse(1, "Mouse", 25.0);
    const Product keyboard(2, "Keyboard", 45.0);
    cart.addProduct(mouse, 2);
    cart.addProduct(keyboard, 1);
    
    cart.clear();
  
    EXPECT_TRUE(cart.isEmpty());
    EXPECT_EQ(cart.getItemCount(), 0);
    EXPECT_EQ(cart.getDifferentProductCount(), 0u);
}