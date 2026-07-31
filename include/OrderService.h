#pragma once

#include "IInventoryService.h"
#include "ILogger.h"
#include "INotificationService.h"
#include "IPaymentService.h"

enum class OrderResult {
    Success,
    InvalidProduct,
    InvalidQuantity,
    InvalidAmount,
    ProductUnavailable,
    PaymentFailed,
    StockUpdateFailed
};

class OrderService {
public:
    OrderService(
        IInventoryService& inventoryService,
        IPaymentService& paymentService,
        INotificationService& notificationService,
        ILogger& logger
    );

    OrderResult placeOrder(
        int productId,
        int quantity,
        double amount
    );

private:
    int generateOrderId();

    IInventoryService& inventoryService_;
    IPaymentService& paymentService_;
    INotificationService& notificationService_;
    ILogger& logger_;

    int nextOrderId_;
};
