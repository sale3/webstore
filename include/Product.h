#pragma once

#include <string>

class Product {
public:
    Product(int id, std::string name, double price);

    int getId() const;
    const std::string& getName() const;
    double getPrice() const;

private:
    int id_;
    std::string name_;
    double price_;
};
