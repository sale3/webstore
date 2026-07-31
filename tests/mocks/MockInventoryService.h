#pragma once

#include "IInventoryService.h"

#include <gmock/gmock.h>

// Примјер једне mock класе.
// Студент треба да креира mock класе за остале интерфејсе.

class MockInventoryService
    : public IInventoryService {
public:
    MOCK_METHOD(
        bool,
        isAvailable,
        (int productId, int quantity),
        (const, override)
    );

    MOCK_METHOD(
        bool,
        reduceStock,
        (int productId, int quantity),
        (override)
    );
};
