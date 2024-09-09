#pragma once

#include <memory>

struct Type {
    virtual ~Type() noexcept = default;
};

struct FundamentalType : public Type {
    virtual ~FundamentalType() noexcept = default;
};

struct VoidType : public FundamentalType {

};

struct ArithmeticType : public FundamentalType {
    virtual ~ArithmeticType() noexcept = default;
};

struct IntegralType : public ArithmeticType {
    virtual ~IntegralType() noexcept = default;
};

struct BoolType : public IntegralType {

};

struct CharType : public IntegralType {

};

struct IntType : public IntegralType {

};

struct FloatingPointType : public ArithmeticType {
    virtual ~FloatingPointType() noexcept = default; 
};

struct DoubleType : public FloatingPointType {
};

struct CompoundType : public Type {
    virtual ~CompoundType() noexcept = default;
};

struct StringType : public CompoundType {
};
