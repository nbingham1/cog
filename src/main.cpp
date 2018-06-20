#include "CogCompiler.h"

#include <vector>
using std::vector;

int main()
{
	CogCompiler compiler;

	compiler.loadFile("main");

	Function *_start;
	BasicBlock *_startBody;

	/* Create the top level interpreter function to call as entry */
	{
		vector<Type*> argTypes;
		FunctionType *ftype = FunctionType::get(Type::getVoidTy(compiler.context), argTypes, false);
		_start = Function::Create(ftype, GlobalValue::ExternalLinkage, "_start", compiler.module);
		_startBody = BasicBlock::Create(compiler.context, "entry", _start, 0);
	}

	compiler.createExit(_startBody);

	ReturnInst::Create(compiler.context, _startBody);
	
	compiler.setTarget();
	compiler.emit();
	
  return 0;
}
