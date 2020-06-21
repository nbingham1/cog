#include <parse/default.h>
#include <parse/message.h>
#include "Lang.h"
#include "cog.h"

namespace Cog
{

::parse::cog_t cog;

Type *cogTypename(parse::token_t token, parse::lexer_t &lexer)
{
	Type *result = Type::find(lexer.read(token.begin, token.end));
	if (result == NULL)
		std::cout << (parse::error(lexer, token) << "unrecognized typename.");

	return result;
}

std::vector<Symbol*> cogInDecl(parse::token_t token, parse::lexer_t &lexer)
{
	std::vector<Symbol*> result;

	Type *declType = NULL;
	std::vector<parse::token_t>::iterator curr = token.tokens.begin();
	if (curr->type == cog.TYPENAME) {
		declType = cogTypename(*curr, lexer);
		curr++;
	} else {
		std::cout << (parse::fail(lexer, token) << "variable declaration without typename.");
		return result;
	}
	
	do {
		std::string name;
		if (curr->type == parse::INSTANCE) {
			name = lexer.read(curr->begin, curr->end);
			curr++;
		} else {
			std::cout << (parse::fail(lexer, token) << "variable declaration without name.");
			return result;
		}

		/*std::vector<int> arr;
		if (curr != token.tokens.end() && curr->type == KEYWORD) {
			std::string ctrl = lexer.read(curr->begin, curr->end);
			if (ctrl == "[") {
				curr++;
				arr.push_back(atoi(lexer.read(curr->begin, curr->end).c_str()));
			}
		}

		if (curr != token.tokens.end() && curr->type == KEYWORD) {
			if (ctrl == "=") {
			}
		}*/

		Symbol *sym = new Symbol(declType, name);
		if (Symbol::declare(sym) == sym)
			result.push_back(sym);
		else
			std::cout << (parse::error(lexer, token) << "conflicting variable declaration.");
			
	} while (curr != token.tokens.end());
	
	return result;
}

void cogLang(parse::token_t token, parse::lexer_t &lexer)
{
	for (std::vector<parse::token_t>::iterator i = token.tokens.begin(); i != token.tokens.end(); i++)
	{
		if (i->type == cog.FUNCTIONDEF)
			cogFunction(*i, lexer);
	}
}

void cogFunction(parse::token_t token, parse::lexer_t &lexer)
{
	std::vector<parse::token_t>::iterator curr = token.tokens.begin();
	Type *retType = NULL, *recvType = NULL;
	std::string name;
	std::vector<Symbol*> args;

	if (curr->type == cog.TYPENAME)
	{
		retType = cogTypename(*curr, lexer);
		if (retType)
			printf("%s\n", retType->name().c_str());
		curr++;
	} else {
		std::cout << (parse::fail(lexer, token) << "function parsed without return type.");
		return;
	}

	if (curr->type == cog.TYPENAME)
	{
		recvType = cogTypename(*curr, lexer);
		curr++;
	}

	if (recvType == NULL)
		recvType = new Void();

	if (recvType)	
		printf("%s\n", recvType->name().c_str());

	if (curr->type == parse::INSTANCE)
	{
		name = lexer.read(curr->begin, curr->end);
		curr++;
		printf("%s\n", name.c_str());
	}

	Symbol::pushScope();

	while (curr->type == cog.INDECL)
	{
		std::vector<Symbol*> tmp = cogInDecl(*curr, lexer);
		args.insert(args.end(), tmp.begin(), tmp.end());
		curr++;
	}

	Type *funcType = new Function(retType, recvType, args);
	if (funcType)
		printf("%s\n", funcType->name().c_str());

	
	llvm::Function *func = 
	Type *lookup = Type::find(funcType->name());
	if (lookup == NULL)
		funcType = Type::define(funcType);
	else {
		std::cout << (parse::error(lexer, token) << "redefinition of function.");
	}

	Symbol::pushScope();


	if (curr->type == cog.BLOCK)
	{
		printf("Block\n");
		printf("%s\n", lexer.read(curr->begin, curr->end).c_str());
		curr++;
	}

	Symbol::dropScope();
	Symbol::dropScope();

	if (curr != token.tokens.end())
	{
		std::cout << (parse::fail(lexer, token) << "function parsed with dangling tokens.");
	}

	/*Scope *scope = cog.getScope();

	delete retType;

	std::string mangled = fType->getName(false);
	
	Function *func = cog.module->getFunction(mangled.c_str());
	if (func == NULL)
		func = Function::Create((llvm::FunctionType*)fType->llvmType, Function::ExternalLinkage, mangled.c_str(), cog.module);
	else if (!func->empty()) {
		error() << "function redefined.\n";
		int i = -1;
		while (func != NULL) {
			++i;
			func = cog.module->getFunction((mangled + "_" + char('a' + i)).c_str());
		}

		func = Function::Create((llvm::FunctionType*)fType->llvmType, Function::ExternalLinkage, (mangled + "_" + char('a' + i)).c_str(), cog.module);
	}

	int i = 0;
	for (auto &arg : func->args()) {
		arg.setName(scope->symbols[i].name);
		scope->symbols[i].setValue(&arg);
		++i;
	}

	cog.currFn = fType;
	BasicBlock *body = BasicBlock::Create(compile.context, "entry", func, 0);
	scope->blocks.push_back(body);
	scope->curr = scope->blocks.begin();
	cog.builder.SetInsertPoint(body);*/

}

}

