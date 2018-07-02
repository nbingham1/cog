#include "Compiler.h"
#include "Parser.y.h"

#include <vector>
using std::vector;

Cog::Compiler cog;
extern FILE* yyin;

int main(int argc, char **argv)
{
	if (argc != 2)
		return 1;

	cog.loadFile(argv[1]);

	yyin = fopen(argv[1], "r");
	yyparse();
	fclose(yyin);

	/* Create the top level interpreter function to call as entry */
	vector<Type*> argTypes;
	FunctionType *ftype = FunctionType::get(Type::getVoidTy(cog.context), argTypes, false);
	Function *_start = Function::Create(ftype, GlobalValue::ExternalLinkage, "_start", cog.module);
	BasicBlock *_startBody = BasicBlock::Create(cog.context, "entry", _start, 0);
	cog.getScope()->setBlock(_startBody);
	cog.builder.SetInsertPoint(_startBody);
	
	vector<Value*> argValues;
	cog.builder.CreateCall(cog.module->getFunction("main"), argValues);
	cog.createExit();

	cog.setTarget();
	//cog.emit(TargetMachine::CGFT_AssemblyFile);
	cog.emit();
	
  return 0;
}
