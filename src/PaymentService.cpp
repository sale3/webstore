#include "PaymentService.h"

#include <stdexcept>

PaymentService::PaymentService(double transactionLimit)
    : transactionLimit_(transactionLimit),
      serviceAvailable_(true),
      lastProcessedAmount_(0.0),
      processedPaymentCount_(0) {
    if (transactionLimit_ <= 0.0) {
        throw std::invalid_argument(
            "Transaction limit must be positive"
        );
    }
}

bool PaymentService::processPayment(double amount) {
    // TODO:
    // Плаћање прихватити само ако је:
    // - amount > 0,
    // - сервис доступан,
    // - amount <= transactionLimit_.
    //
    // Код успјеха ажурирати lastProcessedAmount_
    // и processedPaymentCount_.
    static_cast<void>(amount);

    throw std::logic_error(
        "PaymentService::processPayment is not implemented"
    );
}

void PaymentService::setServiceAvailable(bool available) {
    serviceAvailable_ = available;
}

bool PaymentService::isServiceAvailable() const {
    return serviceAvailable_;
}

double PaymentService::getLastProcessedAmount() const {
    return lastProcessedAmount_;
}

int PaymentService::getProcessedPaymentCount() const {
    return processedPaymentCount_;
}
