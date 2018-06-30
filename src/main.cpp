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

	/* Create the top level interpreter function to call as entry */
	vector<Type*> argTypes;
	FunctionType *ftype = FunctionType::get(Type::getVoidTy(cog.context), argTypes, false);
	Function *_start = Function::Create(ftype, GlobalValue::ExternalLinkage, "_start", cog.module);
	BasicBlock *_startBody = BasicBlock::Create(cog.context, "entry", _start, 0);
	cog.scopes.push_back(Cog::Scope(_startBody));
	cog.builder.SetInsertPoint(_startBody);

	yyin = fopen(argv[1], "r");
	yyparse();
	fclose(yyin);

	//cog.module->print(llvm::errs(), nullptr, false, true);

	cog.setTarget();
	//cog.emit(TargetMachine::CGFT_AssemblyFile);
	cog.emit();
	
  return 0;
}