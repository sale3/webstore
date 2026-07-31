#pragma once

#include "ILogger.h"

#include <string>
#include <vector>

class Logger : public ILogger {
public:
    void log(const std::string& message) override;

    int getMessageCount() const;
    bool contains(const std::string& message) const;

    const std::vector<std::string>&
    getMessages() const;

private:
    std::vector<std::string> messages_;
};
