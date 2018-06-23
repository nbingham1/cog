#include "Compiler.h"
#include "Parser.y.h"

#include <vector>
using std::vector;

Cog::Compiler cog;
extern FILE* yyin;

int main(int argc, char **argv)
{
	cog.loadFile("main.cog");

	yyin = fopen(argv[1], "r");
	yyparse();
	fclose(yyin);

	Function *_start;
	BasicBlock *_startBody;

	/* Create the top level interpreter function to call as entry */
	{
		vector<Type*> argTypes;
		FunctionType *ftype = FunctionType::get(Type::getVoidTy(cog.context), argTypes, false);
		_start = Function::Create(ftype, GlobalValue::ExternalLinkage, "_start", cog.module);
		_startBody = BasicBlock::Create(cog.context, "entry", _start, 0);
	}

	cog.createExit(_startBody);

	ReturnInst::Create(cog.context, _startBody);
	
	cog.setTarget();
	cog.emit(TargetMachine::CGFT_AssemblyFile);
	cog.emit();
	
  return 0;
}
