#pragma once

#include <NotificationService.h>

#include <gmock/gmock.h>

class MockNotificationService: public NotificationService{

    public:
    MOCK_METHOD(void, sendOrderConfirmation, (int orderId),(override));

};