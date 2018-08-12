#include "Intrinsic.h"
#include "Compiler.h"

extern Cog::Compiler cog;
extern int line;
extern int column;
extern const char *str;



namespace Cog
{

llvm::Value *fn_abs(llvm::Value *v0)
{
	llvm::Value *lt0 = cog.builder.CreateICmpSLT(v0, ConstantInt::get(v0->getType(), 0));
	return cog.builder.CreateSelect(lt0, cog.builder.CreateNeg(v0), v0);
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

	llvm::Value *lt0 = cog.builder.CreateICmpSLT(v0, ConstantInt::get(v0->getType(), 0));
	llvm::Value *add = cog.builder.CreateAdd(v0, ConstantInt::get(v0->getType(), mask));
	llvm::Value *sel = cog.builder.CreateSelect(lt0, add, v0);
	return cog.builder.CreateAShr(sel, shift);
}

void fn_exit(llvm::Value *exitCode)
{
	vector<llvm::Type*> argTypes;
	vector<llvm::Value*> argValues;

	if (exitCode != NULL) {
		argTypes.push_back(Type::getInt32Ty(cog.context));
		argValues.push_back(exitCode);
	}

	llvm::FunctionType *fnType = llvm::FunctionType::get(llvm::Type::getVoidTy(cog.context), argTypes, false);	
	llvm::InlineAsm *asmIns = llvm::InlineAsm::get(fnType, "movl $$1,%eax; int $$0x80", "", true);
	cog.builder.CreateCall(asmIns, argValues);
	cog.builder.CreateUnreachable();
}

}

