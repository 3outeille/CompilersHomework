#include "cminus_builder.hpp"
#include <iostream>

// You can define global variables here
// to store state
using namespace llvm;  
// seems to need a global variable to record whether the last declaration is main

#define _DEBUG_PRINT_N_(N) {\
  std::cout << std::string(N, '-');\
}
void CminusBuilder::visit(syntax_program &node) {
	_DEBUG_PRINT_N_(depth);
	std::cout << "program" << std::endl;
	add_depth();
	for (auto decl: node.declarations) {
		decl->accept(*this);
	}
	remove_depth();
}

void CminusBuilder::visit(syntax_num &node) {}

void CminusBuilder::visit(syntax_var_declaration &node) {
	add_depth();
	// If declaration is variable
	if (node.num == nullptr) {
		_DEBUG_PRINT_N_(depth);
		std::cout << "var-declaration: " << node.id;
		GlobalVariable* gvar = new GlobalVariable(*module, Type::getInt32Ty(context), false, GlobalValue::CommonLinkage, 0, node.id);

	}
	// declaration is array
	else {
		_DEBUG_PRINT_N_(depth);
		std::cout << "var-declaration: " << node.id << "[" << node.num->value << "]";
		ArrayType* arrType = ArrayType::get(IntegerType::get(context, 32), node.num->value);
		ConstantAggregateZero* constarr = ConstantAggregateZero::get(arrType);
		GlobalVariable* gvar = new GlobalVariable(*module, arrType, false, GlobalValue::CommonLinkage, constarr, node.id);
	}
	// need to check multiple-defination here!
	std::cout << std::endl;
	remove_depth();
}

void CminusBuilder::visit(syntax_fun_declaration &node) {
	_DEBUG_PRINT_N_(depth);
	std::cout << "fun-declaration: " << node.id << std::endl;
	add_depth();
	std::vector<Type *> Ints(2, Type::getInt32Ty(context));
	auto function = Function::Create(FunctionType::get(Type::getInt32Ty(context), Ints, false), GlobalValue::LinkageTypes::ExternalLinkage, node.id, *module);
	auto bb = BasicBlock::Create(context, "entry", function);
	builder.SetInsertPoint(bb);
	remove_depth();
}

void CminusBuilder::visit(syntax_param &node) {}

void CminusBuilder::visit(syntax_compound_stmt &node) {}

void CminusBuilder::visit(syntax_expresion_stmt &node) {}

void CminusBuilder::visit(syntax_selection_stmt &node) {}

void CminusBuilder::visit(syntax_iteration_stmt &node) {}

void CminusBuilder::visit(syntax_return_stmt &node) {}

void CminusBuilder::visit(syntax_var &node) {}

void CminusBuilder::visit(syntax_assign_expression &node) {}

void CminusBuilder::visit(syntax_simple_expression &node) {}

void CminusBuilder::visit(syntax_additive_expression &node) {}

void CminusBuilder::visit(syntax_term &node) {}

void CminusBuilder::visit(syntax_call &node) {}
