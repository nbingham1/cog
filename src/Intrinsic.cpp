#include "Intrinsic.h"
#include "Compiler.h"

extern Cog::Compiler compile;

namespace Cog
{

llvm::Value *fn_abs(llvm::Value *v0)
{
	llvm::Value *lt0 = compile.builder.CreateICmpSLT(v0, llvm::ConstantInt::get(v0->getType(), 0));
	return compile.builder.CreateSelect(lt0, compile.builder.CreateNeg(v0), v0);
}

/**
 * If I want to quickly divide a signed value by some power of 2, then I should use arithmetic shift right. However, If the numerator has a smaller magnitude than the denominator, then the result of the shift will be -1. Since this is supposed to be a divide, it should be 0, so we need to round it after the fact.
 *
 * Assuming constants:
 * width = sizeof(v0)
 * mask = ((1 << shift)-1)
 *
 * Two possible implementations:
 * v0 = (((v0 >> width) & mask) + v0) >> shift
 * v0 = (v0 < 0 ? v0 + mask : v0) >> shift
 *
 * Both are four instructions, but the second is more parallel:
 * ashr -> and -> add -> ashr
 * (cmplz, add) -> cmov -> ashr 
 */
llvm::Value *fn_div2(llvm::Value *v0, int shift)
{
	int mask = ((1 << shift)-1);

	llvm::Value *lt0 = compile.builder.CreateICmpSLT(v0, llvm::ConstantInt::get(v0->getType(), 0));
	llvm::Value *add = compile.builder.CreateAdd(v0, llvm::ConstantInt::get(v0->getType(), mask));
	llvm::Value *sel = compile.builder.CreateSelect(lt0, add, v0);
	return compile.builder.CreateAShr(sel, shift);
}

void fn_exit(llvm::Value *exitCode)
{
	std::vector<llvm::Type*> argTypes;
	std::vector<llvm::Value*> argValues;

	if (exitCode != NULL) {
		argTypes.push_back(llvm::Type::getInt32Ty(compile.context));
		argValues.push_back(exitCode);
	}

	llvm::FunctionType *fnType = llvm::FunctionType::get(llvm::Type::getVoidTy(compile.context), argTypes, false);	
	llvm::InlineAsm *asmIns = llvm::InlineAsm::get(fnType, "movl $$1,%eax; int $$0x80", "", true);
	compile.builder.CreateCall(asmIns, argValues);
	compile.builder.CreateUnreachable();
}

}

