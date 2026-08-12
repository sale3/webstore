#include "OrderService.h"

#include <stdexcept>
#include <string>
#include <chrono>

OrderService::OrderService(
    IInventoryService& inventoryService,
    IPaymentService& paymentService,
    INotificationService& notificationService,
    ILogger& logger
)
    : inventoryService_(inventoryService),
      paymentService_(paymentService),
      notificationService_(notificationService),
      logger_(logger),
      nextOrderId_(1000) {
}

OrderResult OrderService::placeOrder(
    int productId,
    int quantity,
    double amount
) {
    // ИМПЛ:
    // 1. Провјерити productId.
    // 2. Провјерити quantity.
    // 3. Провјерити amount.
    // 4. Провјерити доступност производа.
    // 5. Обрадити плаћање.
    // 6. Смањити стање.
    // 7. Генерисати идентификатор наруџбе.
    // 8. Послати потврду.
    // 9. Уписати лог поруку.
    // 10. Вратити одговарајући OrderResult.
    //
    // Послије неуспјешног корака наредни сервиси
    // не смију бити позвани.
    if (productId <= 0) {
        return OrderResult::InvalidProduct;
    }
    if (quantity <= 0) {
        return OrderResult::InvalidQuantity;
    }
    if (amount <= 0) {
        return OrderResult::InvalidAmount;
    }
    if (inventoryService_.isAvailable(productId, quantity) == false) {
        return OrderResult::ProductUnavailable;
    }

    OrderResult result;
    bool paymentSuccessful = paymentService_.processPayment(amount);

    if (paymentSuccessful) {
        bool stockReduced = inventoryService_.reduceStock(productId, quantity);
        if (stockReduced) {
            int id = generateOrderId();
            notificationService_.sendOrderConfirmation(id);
            logger_.log("Order placed successfully. Order ID: " + std::to_string(id));
            result = OrderResult::Success;
        }
        else {
            result = OrderResult::StockUpdateFailed;
        }
    }
    else {
        result = OrderResult::PaymentFailed;
    }

    return result;


}

int OrderService::generateOrderId() {
    // ИМПЛ:
    // Генерисати позитиван и јединствен идентификатор
    // у оквиру једне инстанце сервиса.
    
     using namespace std::chrono;

     auto now = system_clock::now();
     //вријеме протекло од 1.1.1970.
     auto millis_since_epoch = duration_cast<milliseconds>(
         now.time_since_epoch()
     ).count();

     // узимамо остатак при дијељењу како би стало у инт
     int time_base = static_cast<int>(millis_since_epoch % 1000000);

     return time_base + (nextOrderId_++);
    

}
