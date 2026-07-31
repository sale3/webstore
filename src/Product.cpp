#include "Product.h"

#include <stdexcept>
#include <utility>

Product::Product(int id, std::string name, double price)
    : id_(id),
      name_(std::move(name)),
      price_(price) {
    if (id_ <= 0) {
        throw std::invalid_argument(
            "Product ID must be positive"
        );
    }

    if (name_.empty()) {
        throw std::invalid_argument(
            "Product name cannot be empty"
        );
    }

    if (price_ < 0.0) {
        throw std::invalid_argument(
            "Product price cannot be negative"
        );
    }
}

int Product::getId() const {
    return id_;
}

const std::string& Product::getName() const {
    return name_;
}

double Product::getPrice() const {
    return price_;
}
