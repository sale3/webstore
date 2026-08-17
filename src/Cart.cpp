#include "Cart.h"

#include <algorithm>
#include <stdexcept>

void Cart::addProduct(
    const Product& product,
    int quantity
) {
    //ИМПЛ
    if (quantity <= 0) {
        throw std::invalid_argument(
            "Quantity must be positive"
        );
    }

    auto it = std::find_if(items_.begin(), items_.end(), [&](const auto& item) {
        return item.product.getId() == product.getId();
        });

    if (it != items_.end()) {
        it->quantity += quantity;
    }
    else {
        items_.push_back({ product, quantity });
    }
}

bool Cart::removeProduct(int productId) {
    auto it = std::find_if(items_.begin(), items_.end(), [productId](const CartItem& item) {
        return item.product.getId() == productId;
    });

    if (it != items_.end()) {
        items_.erase(it);
        return true;
    }

    return false;
}

double Cart::calculateTotal() const {
    // ИМПЛ
    double result = 0;
    for (const CartItem& item : items_) {
        result += item.quantity * item.product.getPrice();
    }

    return result;
}

int Cart::getItemCount() const {
    // ИМПЛ: Вратити укупан број појединачних артикала.
    
    int result = 0;
    for (const CartItem& item : items_) {
        result += item.quantity;
    }
    return result;

}

std::size_t Cart::getDifferentProductCount() const {
    // Ова метода је остављена као једноставан примјер.
    return items_.size();
}

bool Cart::isEmpty() const {
    // Ова метода је остављена као једноставан примјер.
    return items_.empty();
}

//ИМПЛ
void Cart::clear() {
    items_.clear();
}

const std::vector<Cart::CartItem>&
Cart::getItems() const {
    return items_;
}
