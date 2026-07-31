#pragma once

class IPaymentService {
public:
    virtual ~IPaymentService() = default;

    virtual bool processPayment(double amount) = 0;
};
