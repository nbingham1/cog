#pragma once

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instructions.h>

#include <vector>
#include <list>
#include <string>

namespace Cog
{

llvm::Value *fn_abs(llvm::Value *v0);
llvm::Value *fn_div2(llvm::Value *v0, int shift);
void fn_exit(llvm::Value *exitCode);

}

