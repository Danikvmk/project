#include "visitor.hpp"

void Namespace_decl::accept(Visitor& visitor) {
    visitor.visit(*this);
}
void Variables_decl::accept(Visitor& visitor) {
    visitor.visit(*this);
}
void ConstVariable::accept(Visitor& visitor) {
    visitor.visit(*this);
}
void Functions_decl::accept(Visitor& visitor) {
    visitor.visit(*this);
}

/////////////////////////////////////////////////////

void Expression_statement::accept(Visitor& visitor) {
    visitor.visit(*this);
}
void Block_statement::accept(Visitor& visitor) {
    visitor.visit(*this);
}
void Decl_statement::accept(Visitor& visitor) {
    visitor.visit(*this);
}
void While_statement::accept(Visitor& visitor) {
    visitor.visit(*this);
}
void For_statement::accept(Visitor& visitor) {
    visitor.visit(*this);
}
void ConditionalBlock::accept(Visitor& visitor) {
    visitor.visit(*this);
}
void ConditionalBranches::accept(Visitor& visitor) {
    visitor.visit(*this);
}
void Continue_statement::accept(Visitor& visitor) {
    visitor.visit(*this);
}
void Break_statement::accept(Visitor& visitor) {
    visitor.visit(*this);
}
void Return_statement::accept(Visitor& visitor) {
    visitor.visit(*this);
}

////////////////////////////////////////////////////////////////////////

void BinaryNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}
void TernaryNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}
void PrefixNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}
void PostfixNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}
void FunctionNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}
void IdentifierNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}
void ParenthesizedNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}
void IntNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}
void CharNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}
void BoolNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}
void StringNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}
void DoubleNode::accept(Visitor& visitor) {
    visitor.visit(*this);
}
