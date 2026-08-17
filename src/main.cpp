
#include "Cart.h"
#include "DiscountCalculator.h"
#include "InventoryService.h"
#include "Logger.h"
#include "NotificationService.h"
#include "OrderService.h"
#include "PaymentService.h"
#include "Product.h"

#include <iostream>
/*
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

*/


int main() {
    
    const Product product(1, "Wireless Mouse", 25.50);

    std::cout << "Product: " << product.getName()
        << ", price: " << product.getPrice() << '\n';

  
    InventoryService inventoryService;
    inventoryService.setStock(product.getId(), 10);
    std::cout << "Initial stock: "
        << inventoryService.getStock(product.getId()) << '\n';

    
    Cart cart;
    cart.addProduct(product, 3);
    std::cout << "Cart total: " << cart.calculateTotal()
        << ", items: " << cart.getItemCount() << '\n';

   
    DiscountCalculator discountCalculator;
    double finalAmount = discountCalculator.applyDiscount(
        cart.calculateTotal(),
        CustomerType::Gold
    );
    std::cout << "Amount after discount: " << finalAmount << '\n';

    
    PaymentService paymentService(1000.0);
    NotificationService notificationService;
    Logger logger;

    OrderService orderService(
        inventoryService,
        paymentService,
        notificationService,
        logger
    );

    OrderResult result = orderService.placeOrder(
        product.getId(),
        3,
        finalAmount
    );

    std::cout << "Order result: "
        << (result == OrderResult::Success ? "Success" : "Failed")
        << '\n';
    std::cout << "Stock after order: "
        << inventoryService.getStock(product.getId()) << '\n';

 
    std::cout << "Log messages:\n";
    for (const auto& message : logger.getMessages()) {
        std::cout << "  - " << message << '\n';
    }

    return 0;
}