#include "OrderService.h"

#include <stdexcept>

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
    // TODO:
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

    static_cast<void>(productId);
    static_cast<void>(quantity);
    static_cast<void>(amount);

    throw std::logic_error(
        "OrderService::placeOrder is not implemented"
    );
}

int OrderService::generateOrderId() {
    // TODO:
    // Генерисати позитиван и јединствен идентификатор
    // у оквиру једне инстанце сервиса.
    throw std::logic_error(
        "Order ID generation is not implemented"
    );
}
