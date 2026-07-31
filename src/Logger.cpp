#include "Logger.h"

#include <stdexcept>

void Logger::log(const std::string& message) {
    // TODO: Додати поруку у интерну колекцију.
    static_cast<void>(message);

    throw std::logic_error(
        "Logger::log is not implemented"
    );
}

int Logger::getMessageCount() const {
    return static_cast<int>(messages_.size());
}

bool Logger::contains(
    const std::string& message
) const {
    // TODO: Провјерити постојање поруке.
    static_cast<void>(message);

    throw std::logic_error(
        "Logger::contains is not implemented"
    );
}

const std::vector<std::string>&
Logger::getMessages() const {
    return messages_;
}
