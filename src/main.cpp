#include "Product.h"

#include <iostream>

int main() {
    const Product product(
        1,
        "Demonstration product",
        25.50
    );

    std::cout
        << "Initial project is configured.\n"
        << "Product: "
        << product.getName()
        << ", price: "
        << product.getPrice()
        << '\n';

    std::cout
        << "Student must implement the TODO methods.\n";

    return 0;
}
