#include "NotificationService.h"

#include <stdexcept>
#include <algorithm>


void NotificationService::sendOrderConfirmation(
    int orderId
) {
    // ИМПЛ:
    // Одбити неисправан идентификатор и евидентирати
    // идентификатор успјешно послате потврде.

    if(orderId<=0)
    {
        throw std::invalid_argument("OrderId must be positive number.");
    }

    confirmedOrderIds_.push_back(orderId);
}

int NotificationService::getNotificationCount() const {
    return static_cast<int>(confirmedOrderIds_.size());
}

bool NotificationService::wasConfirmationSent(
    int orderId
) const {
    // ИМПЛ: Провјерити да ли је потврда послата.
    return std::find(
        confirmedOrderIds_.begin(),
        confirmedOrderIds_.end(),
        orderId
    ) != confirmedOrderIds_.end();

}

const std::vector<int>&
NotificationService::getConfirmedOrderIds() const {
    return confirmedOrderIds_;
}
