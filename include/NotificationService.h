#pragma once

#include "INotificationService.h"

#include <vector>

class NotificationService
    : public INotificationService {
public:
    void sendOrderConfirmation(int orderId) override;

    int getNotificationCount() const;
    bool wasConfirmationSent(int orderId) const;

    const std::vector<int>&
    getConfirmedOrderIds() const;

private:
    std::vector<int> confirmedOrderIds_;
};
