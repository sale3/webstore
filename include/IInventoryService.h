#pragma once

class IInventoryService {
public:
    virtual ~IInventoryService() = default;

    virtual bool isAvailable(
        int productId,
        int quantity
    ) const = 0;

    virtual bool reduceStock(
        int productId,
        int quantity
    ) = 0;
};
