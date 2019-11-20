#include "cminus_builder.hpp"
#include <iostream>

enum var_op {
  LOAD,
  STORE
};

// You can define global variables here
// to store state
using namespace llvm;  
// seems to need a global variable to record whether the last declaration is main
// this is for array to get basicblock ref. correctly
BasicBlock* curr_block;
// the return basicblock, to make code inside function know where to jump to return
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

// show what to do in syntax_var
var_op curr_op;

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

void CminusBuilder::visit(syntax_num &node) {
	add_depth();
	_DEBUG_PRINT_N_(depth);
	std::cout << "num" << std::endl;
	expression = ConstantInt::get(context, APInt(32, node.value));
	remove_depth();
}

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
	return_block = BasicBlock::Create(context, "returnBB", 0, 0);
	auto entrybb = BasicBlock::Create(context, "entry", function);
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
	
	// reset label counter for each new function
	label_cnt = 0;

	node.compound_stmt->accept(*this);

	return_block->insertInto(function);
	builder.SetInsertPoint(return_block);
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
	curr_op = LOAD;
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
	curr_op = LOAD;
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
		curr_op = LOAD;
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
	add_depth();
	_DEBUG_PRINT_N_(depth);
	std::cout << "var: " << node.id << " op: " << curr_op << std::endl;
	switch (curr_op) {
		case LOAD: {
			if (scope.in_global()) {
				// in global
				std::cout << "global" << std::endl;
			} else {
				// not in global
				if (node.expression == nullptr) {
					// variable
					auto alloca = scope.find(node.id);
					expression = builder.CreateLoad(Type::getInt32Ty(context), alloca);
				}
				else{
					// array
					auto alloca = scope.find(node.id);
					auto arr_ptr = builder.CreateLoad(Type::getInt32Ty(context), alloca);
					curr_op = LOAD;
					node.expression->accept(*this);
					auto gep = builder.CreateGEP(Type::getInt32Ty(context), arr_ptr, expression);
					expression = builder.CreateLoad(Type::getInt32Ty(context), gep);
				}
			}
			break;
		}
		case STORE: {
			if (scope.in_global()) {
			// in global
				std::cout << "global" << std::endl;
			} else {
				// not in global
				if (node.expression == nullptr) {
					// variable
					auto alloca = scope.find(node.id);
					builder.CreateStore(expression, alloca);
				}
				else{
					// array
					auto alloca = scope.find(node.id);
					auto arr_ptr = builder.CreateLoad(Type::getInt32Ty(context), alloca);
					curr_op = LOAD;
					node.expression->accept(*this);
					auto gep = builder.CreateGEP(Type::getInt32Ty(context), arr_ptr, expression);
					builder.CreateStore(expression, gep);
				}
			}
			break;
		}
		default: {
			std::cout << "ERROR: wrong var op!" << std::endl;
		}
	}
	remove_depth();
}

void CminusBuilder::visit(syntax_assign_expression &node) {
	 //std::cout << "*generate dummy expression" << std::endl;
	 ////dummy for my testing
	//expression = ConstantInt::get(Type::getInt32Ty(context), APInt(32, 10));
	 //expression = builder.CreateICmpNE(ConstantInt::get(Type::getInt32Ty(context), APInt(32, 10)), ConstantInt::get(Type::getInt32Ty(context), APInt(32, 10)));
	 //return;

	add_depth();
	_DEBUG_PRINT_N_(depth);
	std::cout << "assign_expression" << std::endl;

	curr_op = LOAD;
	node.expression->accept(*this);

	curr_op = STORE;
	node.var->accept(*this);
	remove_depth();
}

void CminusBuilder::visit(syntax_simple_expression &node) {
	std::cout << "*generate dummy expression" << std::endl;
	 //dummy for my testing
	expression = ConstantInt::get(Type::getInt32Ty(context), APInt(32, 10));
	expression = builder.CreateICmpNE(ConstantInt::get(Type::getInt32Ty(context), APInt(32, 10)), ConstantInt::get(Type::getInt32Ty(context), APInt(32, 10)));
	return;

	add_depth();
	_DEBUG_PRINT_N_(depth);
	std::cout << "simple_expression" << std::endl;

	if (node.additive_expression_r == nullptr) {
		curr_op = LOAD;
		node.additive_expression_l->accept(*this);
	}
	else {
		curr_op = LOAD;
		node.additive_expression_l->accept(*this);
		Value* lhs = expression;
		curr_op = LOAD;
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
	remove_depth();
	//auto lhsAlloca = builder.CreateAlloca(Type::getInt32Ty(context));
	//auto rhsAlloca = builder.CreateAlloca(Type::getInt32Ty(context));
}

void CminusBuilder::visit(syntax_additive_expression &node) {
	add_depth();
	_DEBUG_PRINT_N_(depth);
	std::cout << "additive_expression" << std::endl;

	if (node.additive_expression == nullptr) {
		curr_op = LOAD;
		node.term->accept(*this);
	}
	else {
		curr_op = LOAD;
		node.additive_expression->accept(*this);
		Value* lhs = expression;
		curr_op = LOAD;
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
	remove_depth();
	//std::cout << "*generate dummy expression" << std::endl;
	// dummy for my testing
	//expression = ConstantInt::get(Type::getInt32Ty(context), APInt(32, 10));
	//expression = builder.CreateICmpNE(ConstantInt::get(Type::getInt32Ty(context), APInt(32, 10)), ConstantInt::get(Type::getInt32Ty(context), APInt(32, 10)));
}

void CminusBuilder::visit(syntax_term &node) {
	add_depth();
	_DEBUG_PRINT_N_(depth);
	std::cout << "term" << std::endl;

	if (node.term == nullptr) {
		curr_op = LOAD;
		node.factor->accept(*this);
	}
	else {
		curr_op = LOAD;
		node.term->accept(*this);
		Value* lhs = expression;
		curr_op = LOAD;
		node.factor->accept(*this);
		Value* rhs = expression;
		switch (node.op) {
			case OP_MUL:
				expression = builder.CreateMul(lhs, rhs);
				break;
			case OP_DIV:
				expression = builder.CreateUDiv(lhs, rhs);
				break;
		}
	}
	remove_depth();
}

void CminusBuilder::visit(syntax_call &node) {}
