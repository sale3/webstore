#pragma once

class INotificationService {
public:
    virtual ~INotificationService() = default;

    virtual void sendOrderConfirmation(int orderId) = 0;
};
