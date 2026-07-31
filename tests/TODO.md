# Тестни фајлови које треба додати

Студент треба да креира најмање:

```text
ProductTest.cpp
CartTest.cpp
DiscountCalculatorTest.cpp
InventoryServiceTest.cpp
PaymentServiceTest.cpp
NotificationServiceTest.cpp
LoggerTest.cpp
OrderServiceWithoutMocksTest.cpp
OrderServiceMockTest.cpp
```

Сваки нови фајл потребно је додати у
`tests/CMakeLists.txt`.

Посебно треба тестирати:

- исправне улазе;
- неисправне улазе;
- граничне вриједности;
- изузетке;
- све значајне гране;
- промјене стања;
- позиве зависности;
- изостанак позива након грешке;
- редослијед позива тамо гдје је значајан.
