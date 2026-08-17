#include "InventoryService.h"


#include <algorithm>
#include <stdexcept>

void InventoryService::setStock(
    int productId,
    int quantity
) {
    // ИМПЛ:
    // Провјерити идентификатор и количину,
    // а затим поставити стање.
    static_cast<void>(productId);
    static_cast<void>(quantity);
    if (productId <= 0) {
        throw std::invalid_argument("Product id must be positive");
    }
    if (quantity < 0) {
        throw std::invalid_argument("Quantity cannot be negative");
    }

    stock_[productId] = quantity;
}

int InventoryService::getStock(int productId) const {
    // ИМПЛ:
    // За непостојећи производ вратити 0.
    auto it = stock_.find(productId);

    if (it != stock_.end())
    {
        return it->second;
    }
    else
        return 0;
   
}

bool InventoryService::isAvailable(
    int productId,
    int quantity
) const {
    // ИМПЛ: Провјерити постојање и довољну количину.
   
    auto it = stock_.find(productId);

    if (it == stock_.end())
        return false;
    else 
        return it->second >= quantity;
}

bool InventoryService::reduceStock(
    int productId,
    int quantity
) {
    // ИМПЛ:
    // Смањити стање само ако је количина доступна.
    if (!isAvailable(productId, quantity))
        return false;
    
    stock_[productId] -= quantity;
    return true;
}
