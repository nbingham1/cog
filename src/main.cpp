#include "Compiler.h"
#include "cog.h"
#include "Lang.h"
#include "Intrinsic.h"

#include <vector>
using std::vector;

Cog::Compiler compile;

using namespace llvm;

int main(int argc, char **argv)
{
	if (argc != 2)
		return 1;

	compile.loadFile(argv[1]);

	// Initialize the grammar
	parse::grammar_t gram;
	Cog::cog.load(gram);

	// Load the file into the lexer
	parse::lexer_t lexer;
	lexer.open(argv[1]);

	// Parse the file with the grammar
	parse::parsing result = gram.parse(lexer);
	if (result.msgs.size() == 0)
	{
		// no errors, print the parsed abstract syntax tree
		Cog::cogLang(result.tree, lexer);
	}
	else
	{
		// there were parsing errors, print them out
		for (int i = 0; i < (int)result.msgs.size(); i++)
			std::cout << result.msgs[i];
	}

	/* Create the top level interpreter function to call as entry */
	vector<Type*> argTypes;
	FunctionType *ftype = FunctionType::get(Type::getVoidTy(compile.context), argTypes, false);
	Function *_start = Function::Create(ftype, GlobalValue::ExternalLinkage, "_start", compile.module);
	BasicBlock *_startBody = BasicBlock::Create(compile.context, "entry", _start, 0);
	//compile.getScope()->setBlock(_startBody);
	compile.builder.SetInsertPoint(_startBody);
	
	/*for (auto type = compile.types.begin(); type != compile.types.end(); type++)
		printf("%s\n", type->getName().c_str());*/

	vector<Value*> argValues;
	//compile.builder.CreateCall(compile.module->getFunction("(void)main"), argValues);
	Cog::fn_exit(llvm::ConstantInt::get(llvm::Type::getInt32Ty(compile.context), 0));

	compile.setTarget();
	//compile.emit(TargetMachine::CGFT_AssemblyFile);
	compile.emit();
	
  return 0;
}

