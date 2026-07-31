#pragma once

#include "Product.h"

#include <cstddef>
#include <vector>

class Cart {
public:
    struct CartItem {
        Product product;
        int quantity;
    };

    void addProduct(const Product& product, int quantity);
    bool removeProduct(int productId);

    double calculateTotal() const;
    int getItemCount() const;
    std::size_t getDifferentProductCount() const;

    bool isEmpty() const;
    void clear();

    const std::vector<CartItem>& getItems() const;

private:
    std::vector<CartItem> items_;
};
