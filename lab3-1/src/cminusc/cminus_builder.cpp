#include "cminus_builder.hpp"
#include <iostream>

// You can define global variables here
// to store state
using namespace llvm;  
// seems to need a global variable to record whether the last declaration is main
// this is for array to get basicblock ref. correctly
BasicBlock* curr_block;
// the return label 
BasicBlock* return_block;
// the return value Alloca
Value* return_alloca;
// for BasicBlock::Create to get function ref. 
Function* curr_func;
// record the expression, to be used in while, if and return statements
Value* expression;
// record whether a return statement is encountered. If return is found, following code should be ignored to avoid IR problem.
bool is_returned = false;
bool is_returned_record = false;
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
	auto nullentrybb = BasicBlock::Create(context, "nullentry", function);
	return_block = BasicBlock::Create(context, "returnBB", function);
	auto entrybb = BasicBlock::Create(context, "entry", function);
	builder.SetInsertPoint(nullentrybb);
	builder.CreateBr(entrybb);
	builder.SetInsertPoint(entrybb);
	curr_block = entrybb;
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
	// allocate the return register
	if(node.type != TYPE_VOID) {
		return_alloca = builder.CreateAlloca(Type::getInt32Ty(context));
	}
	//builder.SetInsertPoint(return_block);
	
	// reset label counter for each new function
	label_cnt = 0;

	//builder.SetInsertPoint(entrybb);
	node.compound_stmt->accept(*this);

	auto truereturn = BasicBlock::Create(context, "trueReturnBB", function);
	//builder.CreateBr(truereturn);
	builder.SetInsertPoint(return_block);
	builder.CreateBr(truereturn);
	builder.SetInsertPoint(truereturn);
	if(node.type != TYPE_VOID) {
		auto retLoad = builder.CreateLoad(Type::getInt32Ty(context), return_alloca);
		builder.CreateRet(retLoad);
	}
	else {
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
	// a compound_stmt means a new scope
	scope.enter();
	// process var-declaration
	for (auto var_decl: node.local_declarations) {
		if (var_decl->type == TYPE_VOID) {
			std::cout << "Error: no void type variable or array is allowed!" << std::endl;
		}
		// array declaration
		if (var_decl->num != nullptr) {
			auto arrType = ArrayType::get(IntegerType::get(context, 32), var_decl->num->value);
			auto arrptr = new AllocaInst(arrType, 0, "", curr_block);
			scope.push(var_decl->id, arrptr);
		}
		// normal variable declaration
		else {
			auto var = builder.CreateAlloca(Type::getInt32Ty(context));
			scope.push(var_decl->id, var);
		}
	}
	is_returned = false;
	for (auto stmt: node.statement_list) {
		stmt->accept(*this);
		if (is_returned)
			break;
	}
	is_returned_record = is_returned;
	is_returned = false;
	scope.exit();
	remove_depth();
}

void CminusBuilder::visit(syntax_expresion_stmt &node) {
	add_depth();
	_DEBUG_PRINT_N_(depth);
	std::cout << "expression_stmt" << std::endl;
	node.expression->accept(*this);
	remove_depth();
}

void CminusBuilder::visit(syntax_selection_stmt &node) {
	add_depth();
	_DEBUG_PRINT_N_(depth);
	std::cout << "selection_stmt" << std::endl;
	node.expression->accept(*this);
	// actually this is not needed, as llvm will automatically generate different label name
	char labelname[100];
	BasicBlock* trueBB;
	BasicBlock* falseBB;
	auto expri1 = builder.CreateICmpNE(expression, ConstantInt::get(Type::getInt32Ty(context), 0, true));
	// record the current block, for add the br later
	BasicBlock* orig_block = curr_block;
	// create the conditional jump CondBr
	//auto exprAlloca = builder.CreateAlloca(Type::getInt1Ty(context));
	//auto exprStore = builder.CreateStore(expri1, exprAlloca);
	//auto exprLoad = builder.CreateLoad(Type::getInt1Ty(context), exprAlloca);
	//builder.CreateCondBr(exprLoad, trueBB, falseBB);
	//builder.CreateCondBr(exprLoad, trueBB, falseBB);
	//
	// if-statement
	label_cnt++;
	int label_now = label_cnt;
	sprintf(labelname, "selTrueBB_%d", label_now);
	trueBB = BasicBlock::Create(context, labelname, curr_func);
	builder.SetInsertPoint(trueBB);
	curr_block = trueBB;
	BasicBlock* trueBB_location;
	BasicBlock* falseBB_location;
	bool trueBB_returned;
	bool falseBB_returned;
	node.if_statement->accept(*this);
	trueBB_location = curr_block;
	trueBB_returned = is_returned_record;
	// optional else-statement
	if (node.else_statement != nullptr) {
		sprintf(labelname, "selFalseBB_%d", label_now);
		falseBB = BasicBlock::Create(context, labelname, curr_func);
	}
	if (node.else_statement != nullptr) {
		builder.SetInsertPoint(falseBB);
		curr_block = falseBB;
		node.else_statement->accept(*this);
		falseBB_location = curr_block;
		falseBB_returned = is_returned_record;
	}
	sprintf(labelname, "selEndBB_%d", label_now);
	auto endBB = BasicBlock::Create(context, labelname, curr_func);
	builder.SetInsertPoint(orig_block);
	if (node.else_statement != nullptr) {
		builder.CreateCondBr(expri1, trueBB, falseBB);
	}
	else {
		builder.CreateCondBr(expri1, trueBB, endBB);
	}
	// unconditional jump to make ends meet
	if (!trueBB_returned) {
		builder.SetInsertPoint(trueBB_location);
		builder.CreateBr(endBB);
	}
	if (node.else_statement != nullptr && !falseBB_returned) {
		builder.SetInsertPoint(falseBB_location);
		builder.CreateBr(endBB);
	}
	builder.SetInsertPoint(endBB);
	curr_block = endBB;
	remove_depth();
}

void CminusBuilder::visit(syntax_iteration_stmt &node) {
	add_depth();
	_DEBUG_PRINT_N_(depth);
	std::cout << "iteration_stmt" << std::endl;
	// iteration_stmt => 
	//
	// 	goto start
	// start:
	// 	if expression goto body else goto end
	// body:
	// 	statement
	// 	goto start
	// end:
	label_cnt++;
	int label_now = label_cnt;
	char labelname[100];
	sprintf(labelname, "loopStartBB_%d", label_now);
	auto startBB = BasicBlock::Create(context, labelname, curr_func);
	// goto start, end former block
	builder.CreateBr(startBB);
	builder.SetInsertPoint(startBB);
	curr_block = startBB;
	node.expression->accept(*this);
	auto expri1 = builder.CreateICmpNE(expression, ConstantInt::get(Type::getInt32Ty(context), 0, true));
	sprintf(labelname, "loopBodyBB_%d", label_now);
	auto bodyBB = BasicBlock::Create(context, labelname, curr_func);
	builder.SetInsertPoint(bodyBB);
	curr_block = bodyBB;
	node.statement->accept(*this);
	if (!is_returned_record) {
		builder.CreateBr(startBB);
	}
	sprintf(labelname, "loopEndBB_%d", label_now);
	auto endBB = BasicBlock::Create(context, labelname, curr_func);
	// go back to create the CondBr in it's right location
	builder.SetInsertPoint(startBB);
	builder.CreateCondBr(expri1, bodyBB, endBB);
	builder.SetInsertPoint(endBB);
	curr_block = endBB;
	remove_depth();
}

void CminusBuilder::visit(syntax_return_stmt &node) {
	add_depth();
	_DEBUG_PRINT_N_(depth);
	std::cout << "return_stmt" << std::endl;
	if (node.expression != nullptr) {
		node.expression->accept(*this);
		Value* retVal;
		if (expression->getType() == Type::getInt1Ty(context)) {
			// cast i1 boolean true or false result to i32 0 or 1
			auto retCast = builder.CreateIntCast(expression, Type::getInt32Ty(context), false);
			retVal = retCast;
			//builder.CreateRet(retCast);
		}
		else if (expression->getType() == Type::getInt32Ty(context)) {
			retVal = expression;
			//builder.CreateRet(expression);
		}
		else {
			std::cout << "Error: unknown expression return type!" << std::endl;
		}
		//builder.CreateRet(retVal);
		builder.CreateStore(retVal, return_alloca);
	}
	else {
		//builder.CreateRetVoid();
	}
	builder.CreateBr(return_block);
	is_returned = true;
	remove_depth();
}

void CminusBuilder::visit(syntax_var &node) {
	//node.id
}

void CminusBuilder::visit(syntax_assign_expression &node) {
	std::cout << "*generate dummy expression" << std::endl;
	// dummy for my testing
	//expression = ConstantInt::get(Type::getInt32Ty(context), APInt(32, 10));
	expression = builder.CreateICmpNE(ConstantInt::get(Type::getInt32Ty(context), APInt(32, 10)), ConstantInt::get(Type::getInt32Ty(context), APInt(32, 10)));
}

void CminusBuilder::visit(syntax_simple_expression &node) {
	if (node.additive_expression_r == nullptr) {
		node.additive_expression_l->accept(*this);
	}
	else {
		node.additive_expression_l->accept(*this);
		Value* lhs = expression;
		node.additive_expression_r->accept(*this);
		Value* rhs = expression;
		switch (node.op) {
			case OP_LE:
				expression = builder.CreateICmpSLE(lhs, rhs);
				break;
			case OP_LT:
				expression = builder.CreateICmpSLT(lhs, rhs);
				break;
			case OP_GT:
				expression = builder.CreateICmpSGT(lhs, rhs);
				break;
			case OP_GE:
				expression = builder.CreateICmpSGE(lhs, rhs);
				break;
			case OP_EQ:
				expression = builder.CreateICmpEQ(lhs, rhs);
				break;
			case OP_NEQ:
				expression = builder.CreateICmpNE(lhs, rhs);
				break;
		}
	}
	//auto lhsAlloca = builder.CreateAlloca(Type::getInt32Ty(context));
	//auto rhsAlloca = builder.CreateAlloca(Type::getInt32Ty(context));
	//std::cout << "*generate dummy expression" << std::endl;
	// dummy for my testing
	//expression = ConstantInt::get(Type::getInt32Ty(context), APInt(32, 10));
	//expression = builder.CreateICmpNE(ConstantInt::get(Type::getInt32Ty(context), APInt(32, 10)), ConstantInt::get(Type::getInt32Ty(context), APInt(32, 10)));
}

void CminusBuilder::visit(syntax_additive_expression &node) {
	if (node.term == nullptr) {
		node.additive_expression->accept(*this);
	}
	else {
		node.additive_expression->accept(*this);
		Value* lhs = expression;
		node.term->accept(*this);
		Value* rhs = expression;
		switch (node.op) {
			case OP_PLUS:
				expression = builder.CreateAdd(lhs, rhs);
				break;
			case OP_MINUS:
				expression = builder.CreateSub(lhs, rhs);
				break;
		}
	}
	//std::cout << "*generate dummy expression" << std::endl;
	// dummy for my testing
	//expression = ConstantInt::get(Type::getInt32Ty(context), APInt(32, 10));
	//expression = builder.CreateICmpNE(ConstantInt::get(Type::getInt32Ty(context), APInt(32, 10)), ConstantInt::get(Type::getInt32Ty(context), APInt(32, 10)));
}

void CminusBuilder::visit(syntax_term &node) {}

void CminusBuilder::visit(syntax_call &node) {}
