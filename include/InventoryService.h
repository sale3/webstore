#pragma once

#include "IInventoryService.h"

#include <unordered_map>

class InventoryService : public IInventoryService {
public:
    void setStock(int productId, int quantity);
    int getStock(int productId) const;

    bool isAvailable(
        int productId,
        int quantity
    ) const override;

    bool reduceStock(
        int productId,
        int quantity
    ) override;

private:
    std::unordered_map<int, int> stock_;
};
