#include <gmock/gmock.h>
#include <ILogger.h>
#include <string>

class MockLogger : public ILogger {

    public:
    MOCK_METHOD(void, log, (const std::string& message), (override));
   
};