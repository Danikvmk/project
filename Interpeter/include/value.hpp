#pragma once

#include <memory>
#include <string>
#include <variant>


struct Value {
    virtual ~Value() noexcept = default;
};

struct Rvalue : public Value {
    virtual ~Rvalue() noexcept = default;
};

struct IntValue : public Rvalue {
    int value;
    IntValue(int value = 0) : value(value) {}
};

struct DoubleValue : public Rvalue {
    double value;
    DoubleValue(double value = 0) : value(value) {}
};

struct CharValue : public Rvalue {
    char value;
    CharValue(char value = 0) : value(value) {}
};

struct BoolValue : public Rvalue {
    bool value;
    BoolValue(bool value = 0) : value(value) {}
};

struct StringValue : public Rvalue {
    std::string value;
    StringValue(const std::string& value = "") : value(value) {} 
};

struct Lvalue : public Value {
    std::shared_ptr<Rvalue> value;
    Lvalue(const std::shared_ptr<Value>& value = nullptr)
    	: value(std::dynamic_pointer_cast<Rvalue>(value)) {}
};
