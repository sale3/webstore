#pragma once

#include <gmock/gmock.h>

#include "PaymentService.h"

class MockPaymentService: public PaymentService{

        public: 
        MOCK_METHOD(bool, processPayment,(double amount),(override));

};