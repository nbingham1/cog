#include "Compiler.h"
#include "y.tab.h"

#include <vector>
using std::vector;

Cog::Compiler compiler;

int main(int argc, char **argv)
{
	compiler.loadFile("main.cog");

	yyin = fopen(argv[1], "r");
	yyparse();
	fclose(yyin);

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
	compiler.emit(TargetMachine::CGFT_AssemblyFile);
	compiler.emit();
	
  return 0;
}
