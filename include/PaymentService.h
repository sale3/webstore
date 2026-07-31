#pragma once

#include "IPaymentService.h"

class PaymentService : public IPaymentService {
public:
    explicit PaymentService(
        double transactionLimit = 10000.0
    );

    bool processPayment(double amount) override;

    void setServiceAvailable(bool available);
    bool isServiceAvailable() const;

    double getLastProcessedAmount() const;
    int getProcessedPaymentCount() const;

private:
    double transactionLimit_;
    bool serviceAvailable_;
    double lastProcessedAmount_;
    int processedPaymentCount_;
};
