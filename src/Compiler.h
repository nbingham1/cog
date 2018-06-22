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

#include <llvm/Support/CodeGen.h>

#include <string>

using namespace std;
using namespace llvm;
using namespace llvm::sys;

struct CogCompiler
{
	CogCompiler();
	~CogCompiler();

	LLVMContext context;
	IRBuilder<> builder;
	string targetTriple;
	TargetMachine *target;
	Module *module;
	string source;

	void loadFile(string filename);

	bool setTarget(string targetTriple = sys::getDefaultTargetTriple());	
	void createExit(BasicBlock *body, Value *ret = NULL);
	
	bool emit(TargetMachine::CodeGenFileType fileType = TargetMachine::CGFT_ObjectFile);
};

