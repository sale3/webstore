#include "Logger.h"

#include <stdexcept>
#include <algorithm>

void Logger::log(const std::string& message) {
    // ИМПЛ: Додати поруку у интерну колекцију.
	messages_.push_back(message);
}

int Logger::getMessageCount() const {
    return static_cast<int>(messages_.size());
}

bool Logger::contains(
    const std::string& message
) const {
    // ИМПЛ: Провјерити постојање поруке.
	auto it = std::find_if(messages_.begin(), messages_.end(),
		[&message](const std::string& msg) {
			return msg == message;
		});
	return it != messages_.end();
}

const std::vector<std::string>&
Logger::getMessages() const {
    return messages_;
}
