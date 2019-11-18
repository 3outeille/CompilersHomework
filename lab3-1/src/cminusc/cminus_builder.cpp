#include "cminus_builder.hpp"
#include <iostream>

// You can define global variables here
// to store state
using namespace llvm;  
// seems to need a global variable to record whether the last declaration is main
// this is for array to get basicblock ref. correctly
BasicBlock* curr_block;
// for BasicBlock::Create to get function ref. 
Function* curr_func;
// record the expression, to be used in while, if and return statements
Value* expression;
int label_cnt = 0;

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
			Vars.push_back(PointerType::getInt32PtrTy(context));
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
			auto param_var = builder.CreateAlloca(PointerType::getInt32PtrTy(context));
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

	label_cnt = 0;

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
	}
	for (auto stmt: node.statement_list) {
		stmt->accept(*this);
	}
	remove_depth();
}

void CminusBuilder::visit(syntax_expresion_stmt &node) {
	add_depth();
	_DEBUG_PRINT_N_(depth);
	std::cout << "*generate dummy expression stmt" << std::endl;
	// dummy for my testing
	expression = builder.CreateICmpEQ(ConstantInt::get(Type::getInt32Ty(context), APInt(32, 10)), ConstantInt::get(Type::getInt32Ty(context), APInt(32, 10)));
	return;

	std::cout << "expression_stmt" << std::endl;
	node.expression->accept(*this);
	remove_depth();
}

void CminusBuilder::visit(syntax_selection_stmt &node) {
	add_depth();
	_DEBUG_PRINT_N_(depth);
	std::cout << "selection_stmt" << std::endl;
	// first process the expression(condition)
	node.expression->accept(*this);
	// actually this is not needed, as llvm will automatically generate different label name
	char labelname[100];
	BasicBlock* trueBB;
	BasicBlock* falseBB;
	// if-statement
	sprintf(labelname, "trueBB_%d", ++label_cnt);
	trueBB = BasicBlock::Create(context, labelname, curr_func);
	// optional else-statement
	if (node.else_statement != nullptr) {
		sprintf(labelname, "falseBB_%d", ++label_cnt);
		falseBB = BasicBlock::Create(context, labelname, curr_func);
	}
	// the conditional jump
	node.expression->accept(*this);
	builder.CreateCondBr(expression, trueBB, falseBB);
	// do the inner if and else statement
	builder.SetInsertPoint(trueBB);
	node.if_statement->accept(*this);
	if (node.else_statement != nullptr) {
		builder.SetInsertPoint(falseBB);
		node.else_statement->accept(*this);
	}
	// unconditional jump to make ends meet
	sprintf(labelname, "endBB_%d", ++label_cnt);
	auto endBB = BasicBlock::Create(context, labelname, curr_func);
	builder.SetInsertPoint(trueBB);
	builder.CreateBr(endBB);
	if (node.else_statement != nullptr) {
		builder.SetInsertPoint(falseBB);
		builder.CreateBr(endBB);
	}
	builder.SetInsertPoint(endBB);
	remove_depth();
}

void CminusBuilder::visit(syntax_iteration_stmt &node) {
	add_depth();
	_DEBUG_PRINT_N_(depth);
	std::cout << "iteration_stmt" << std::endl;
	// iteration_stmt: 
	// start:
	// 	if expression goto body else goto end
	// body:
	// 	statement
	// 	goto start
	// end:
	char labelname[100];
	sprintf(labelname, "startBB_%d", ++label_cnt);
	auto startBB = BasicBlock::Create(context, labelname, curr_func);
	sprintf(labelname, "bodyBB_%d", ++label_cnt);
	auto bodyBB = BasicBlock::Create(context, labelname, curr_func);
	sprintf(labelname, "endBB_%d", ++label_cnt);
	auto endBB = BasicBlock::Create(context, labelname, curr_func);
	builder.SetInsertPoint(startBB);
	node.expression->accept(*this);
	builder.CreateCondBr(expression, bodyBB, endBB);
	builder.SetInsertPoint(bodyBB);
	node.statement->accept(*this);
	builder.CreateBr(startBB);
	builder.SetInsertPoint(endBB);
	remove_depth();
}

void CminusBuilder::visit(syntax_return_stmt &node) {
	add_depth();
	_DEBUG_PRINT_N_(depth);
	std::cout << "return_stmt" << std::endl;
	if (node.expression != nullptr) {
		node.expression->accept(*this);
		// TODO: NEED ZEXT from i1 to i32 here!!
		// and in other expression places need to TRUNC i32 to i1 !!
		builder.CreateRet(expression);
	}
	else {
		builder.CreateRetVoid();
	}
	remove_depth();
}

void CminusBuilder::visit(syntax_var &node) {}

void CminusBuilder::visit(syntax_assign_expression &node) {
	std::cout << "*generate dummy expression" << std::endl;
	// dummy for my testing
	expression = builder.CreateICmpEQ(ConstantInt::get(Type::getInt32Ty(context), APInt(32, 10)), ConstantInt::get(Type::getInt32Ty(context), APInt(32, 10)));
}

void CminusBuilder::visit(syntax_simple_expression &node) {
	std::cout << "*generate dummy expression" << std::endl;
	// dummy for my testing
	expression = builder.CreateICmpEQ(ConstantInt::get(Type::getInt32Ty(context), APInt(32, 10)), ConstantInt::get(Type::getInt32Ty(context), APInt(32, 10)));
}

void CminusBuilder::visit(syntax_additive_expression &node) {
	std::cout << "*generate dummy expression" << std::endl;
	// dummy for my testing
	expression = builder.CreateICmpEQ(ConstantInt::get(Type::getInt32Ty(context), APInt(32, 10)), ConstantInt::get(Type::getInt32Ty(context), APInt(32, 10)));
}

void CminusBuilder::visit(syntax_term &node) {}

void CminusBuilder::visit(syntax_call &node) {}
