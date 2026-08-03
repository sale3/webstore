#include "Cart.h"

#include <algorithm>
#include <stdexcept>

void Cart::addProduct(
    const Product& product,
    int quantity
) {
    if (quantity <= 0) {
        throw std::invalid_argument(
            "Quantity must be positive"
        );
    }

    // TODO:
    // Ако производ већ постоји, повећати његову количину.
    // У супротном, додати нову ставку.
    items_.push_back({product, quantity});
}

bool Cart::removeProduct(int productId) {
    // TODO: Имплементирати уклањање производа.
    static_cast<void>(productId);
    throw std::logic_error(
        "Cart::removeProduct is not implemented"
    );
}

double Cart::calculateTotal() const {
    // TODO: Имплементирати израчунавање укупне цијене.
    throw std::logic_error(
        "Cart::calculateTotal is not implemented"
    );
}

int Cart::getItemCount() const {
    // TODO: Вратити укупан број појединачних артикала.
    throw std::logic_error(
        "Cart::getItemCount is not implemented"
    );
}

std::size_t Cart::getDifferentProductCount() const {
    // Ова метода је остављена као једноставан примјер.
    return items_.size();
}

bool Cart::isEmpty() const {
    // Ова метода је остављена као једноставан примјер.
    return items_.empty();
}

void Cart::clear() {
    items_.clear();
}

const std::vector<Cart::CartItem>&
Cart::getItems() const {
    return items_;
}
