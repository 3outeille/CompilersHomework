#include "cminus_builder.hpp"
#include <iostream>

// You can define global variables here
// to store state
using namespace llvm;  
// seems to need a global variable to record whether the last declaration is main
// this is for array to get basicblock ref. correctly
BasicBlock* curr_block;
// may be this?
Function* curr_func;

#define _DEBUG_PRINT_N_(N) {\
  std::cout << std::string(N, '-');\
}
void CminusBuilder::visit(syntax_program &node) {
	add_depth();
	_DEBUG_PRINT_N_(depth);
	std::cout << "program" << std::endl;
	for (auto decl: node.declarations) {
		decl->accept(*this);
	}
	remove_depth();
}

void CminusBuilder::visit(syntax_num &node) {}

void CminusBuilder::visit(syntax_var_declaration &node) {
	add_depth();
	// declaration is variable
	if (node.num == nullptr) {
		_DEBUG_PRINT_N_(depth);
		std::cout << "var-declaration: " << node.id;
		ConstantInt* const_int = ConstantInt::get(context, APInt(32, 0));
		GlobalVariable* gvar = new GlobalVariable(*module, 
				PointerType::getInt32Ty(context), 
				false, 
				GlobalValue::CommonLinkage, 
				const_int, 
				node.id);
		gvar->setAlignment(4);
	}
	// declaration is array
	else {
		_DEBUG_PRINT_N_(depth);
		std::cout << "var-declaration: " << node.id << "[" << node.num->value << "]";
		ArrayType* arrType = ArrayType::get(IntegerType::get(context, 32), node.num->value);
		ConstantAggregateZero* constarr = ConstantAggregateZero::get(arrType);
		GlobalVariable* gvar = new GlobalVariable(*module, 
				arrType, 
				false, 
				GlobalValue::CommonLinkage, 
				constarr, 
				node.id);
	}
	std::cout << std::endl;
	remove_depth();
}

void CminusBuilder::visit(syntax_fun_declaration &node) {
	add_depth();
	_DEBUG_PRINT_N_(depth);
	scope.enter();
	std::cout << "fun-declaration: " << node.id << std::endl;
	std::vector<Type *> Vars;
	for (auto param: node.params) {
		if (param->isarray) {
			Vars.push_back(PointerType::getInt32Ty(context));
		}
		else {
			Vars.push_back(Type::getInt32Ty(context));
		}
	}
	auto functype = node.type == TYPE_INT ? Type::getInt32Ty(context) : Type::getVoidTy(context);
	auto function = Function::Create(FunctionType::get(functype, Vars, false), 
			GlobalValue::LinkageTypes::ExternalLinkage, 
			node.id, 
			*module);
	curr_func = function;
	auto bb = BasicBlock::Create(context, "entry", function);
	builder.SetInsertPoint(bb);
	curr_block = bb;
	// allocate space for function params, and add to symbol table(scope)
	auto arg = function->arg_begin();
	for (auto param: node.params) {
		if (arg == function->arg_end()) {
			std::cout << "Fatal error: parameter number different!!" << std::endl;
		}
		if (param->isarray) {
			//auto arrType = ArrayType::get()
			//auto param_arr = builder.Create
			auto param_var = builder.CreateAlloca(PointerType::getInt32Ty(context));
			//builder.CreatePtrToInt
			scope.push(param->id, param_var);
			builder.CreateStore(arg, param_var);
		}
		else {
			auto param_var = builder.CreateAlloca(Type::getInt32Ty(context)); 
			scope.push(param->id, param_var);
			builder.CreateStore(arg, param_var);
		}
		arg++;
	}


	node.compound_stmt->accept(*this);

	//// allocate the return register
	//if(node.type != TYPE_VOID) {
		//auto retAlloca = builder.CreateAlloca(Type::getInt32Ty(context));
	//}

	// maybe need this? in case no explicit return?
	if(node.type == TYPE_VOID) {
		builder.CreateRetVoid();
	}
	scope.exit();
	remove_depth();
}

void CminusBuilder::visit(syntax_param &node) {
	//node.id
	//node.isarray
	//node.type
}

void CminusBuilder::visit(syntax_compound_stmt &node) {
	add_depth();
	_DEBUG_PRINT_N_(depth);
	std::cout << "compound_stmt" << std::endl;
	// process var-declaration
	for (auto var_decl: node.local_declarations) {
		if (var_decl->type == TYPE_VOID) {
			std::cout << "Error: no void type variable or array is allowed!" << std::endl;
		}
		// array declaration
		if (var_decl->num != nullptr) {
			auto arrType = ArrayType::get(IntegerType::get(context, 32), var_decl->num->value);
			auto arrptr = new AllocaInst(arrType, 0, "", curr_block);
		}
		// normal variable declaration
		else {
			auto var = builder.CreateAlloca(Type::getInt32Ty(context));
			scope.push(var_decl->id, var);
		}
		//var_decl.
	}
	for (auto stmt: node.statement_list) {
		stmt->accept(*this);
	}
	remove_depth();
}

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
