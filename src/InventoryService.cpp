#include "InventoryService.h"

#include <stdexcept>

void InventoryService::setStock(
    int productId,
    int quantity
) {
    // TODO:
    // Провјерити идентификатор и количину,
    // а затим поставити стање.
    static_cast<void>(productId);
    static_cast<void>(quantity);

    throw std::logic_error(
        "InventoryService::setStock is not implemented"
    );
}

int InventoryService::getStock(int productId) const {
    // TODO:
    // За непостојећи производ вратити 0.
    static_cast<void>(productId);

    throw std::logic_error(
        "InventoryService::getStock is not implemented"
    );
}

bool InventoryService::isAvailable(
    int productId,
    int quantity
) const {
    // TODO: Провјерити постојање и довољну количину.
    static_cast<void>(productId);
    static_cast<void>(quantity);

    throw std::logic_error(
        "InventoryService::isAvailable is not implemented"
    );
}

bool InventoryService::reduceStock(
    int productId,
    int quantity
) {
    // TODO:
    // Смањити стање само ако је количина доступна.
    static_cast<void>(productId);
    static_cast<void>(quantity);

    throw std::logic_error(
        "InventoryService::reduceStock is not implemented"
    );
}
