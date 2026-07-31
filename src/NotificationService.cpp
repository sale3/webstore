#include "NotificationService.h"

#include <stdexcept>

void NotificationService::sendOrderConfirmation(
    int orderId
) {
    // TODO:
    // Одбити неисправан идентификатор и евидентирати
    // идентификатор успјешно послате потврде.
    static_cast<void>(orderId);

    throw std::logic_error(
        "NotificationService is not implemented"
    );
}

int NotificationService::getNotificationCount() const {
    return static_cast<int>(confirmedOrderIds_.size());
}

bool NotificationService::wasConfirmationSent(
    int orderId
) const {
    // TODO: Провјерити да ли је потврда послата.
    static_cast<void>(orderId);

    throw std::logic_error(
        "Notification lookup is not implemented"
    );
}

const std::vector<int>&
NotificationService::getConfirmedOrderIds() const {
    return confirmedOrderIds_;
}
