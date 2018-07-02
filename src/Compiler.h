#pragma once

#include <llvm/IR/LLVMContext.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/IR/Module.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/Host.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/TargetRegistry.h>

#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/NoFolder.h>

#include <llvm/Support/CodeGen.h>

#include <vector>
#include <string>

#include "Info.h"

using namespace llvm;
using namespace llvm::sys;

namespace Cog
{

struct Compiler
{
	Compiler();
	~Compiler();

	LLVMContext context;
	IRBuilder<llvm::NoFolder> builder;
	std::string targetTriple;
	TargetMachine *target;
	Module *module;
	std::string source;

	Function *func;

	std::vector<Typename> types;
	std::vector<Scope> scopes;

	void printScope();
	Scope* getScope();
	void pushScope();
	void popScope();


	void loadFile(std::string filename);
	bool setTarget(std::string targetTriple = sys::getDefaultTargetTriple());	
	void createExit(Value *ret = NULL);
	
	bool emit(TargetMachine::CodeGenFileType fileType = TargetMachine::CGFT_ObjectFile);
};

}
