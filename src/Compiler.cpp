#include "Compiler.h"
#include <llvm/IR/IRPrintingPasses.h>

using namespace std;

namespace Cog
{

Compiler::Compiler() : builder(context)
{
	targetTriple = "";
	types.push_back(Typename());
	types.back().name = "int";
	types.back().type = Type::getInt32Ty(context);
	func = NULL;
}

Compiler::~Compiler()
{
}

void Compiler::printScope()
{
	printf("Scopes:");
	for (int i = 0; i < (int)scopes.size(); i++)
		printf(" %d/%d", (int)std::distance(scopes[i].blocks.begin(), scopes[i].curr), (int)scopes[i].blocks.size());
	printf("\n");
}

Scope* Compiler::getScope()
{
	return &scopes.back();
}

void Compiler::pushScope()
{
	printf("pushScope()\n");
	printScope();
	scopes.push_back(Scope(getScope()->getBlock()));
	printScope();
}

void Compiler::popScope()
{
	printf("popScope()\n");
	printScope();
	BasicBlock *block = getScope()->getBlock();
	scopes.pop_back();
	getScope()->setBlock(block);
	printScope();
}

Symbol* Compiler::findSymbol(string name)
{
	for (auto scope = scopes.rbegin(); scope != scopes.rend(); ++scope) {
		for (auto symbol = scope->symbols.rbegin(); symbol != scope->symbols.rend(); ++symbol) {
			if (symbol->name == name) {
				return &(*symbol);
			}
		}
	}

	return NULL;
}

Symbol *Compiler::createSymbol(string name, llvm::Type *type)
{
	getScope()->symbols.push_back(Cog::Symbol(name, type));
	return &getScope()->symbols.back();
}

void Compiler::loadFile(string filename)
{
	
	module = new Module(filename, context);
	source = filename;
}

bool Compiler::setTarget(string targetTriple)
{
	if (!module) {
		errs() << "Module not yet initialized";
		return false;
	} else if (targetTriple != this->targetTriple) {
		if (this->targetTriple == "") {
			InitializeAllTargetInfos();
			InitializeAllTargets();
			InitializeAllTargetMCs();
			InitializeAllAsmParsers();
			InitializeAllAsmPrinters();
		}

		this->targetTriple = targetTriple;
		module->setTargetTriple(targetTriple);

		string error;
		const Target *targetEntry = TargetRegistry::lookupTarget(targetTriple, error);
		if (!targetEntry) {
			errs() << "error: " << error;
			return false;
		}

		TargetOptions options;
		Optional<Reloc::Model> relocModel;
		target = targetEntry->createTargetMachine(targetTriple, "generic", "", options, relocModel);

		module->setDataLayout(target->createDataLayout());
	}
	return true;
}


void Compiler::createExit(Value *ret)
{
	vector<Type*> argTypes;
	vector<Value*> argValues;

	if (ret != NULL) {
		argTypes.push_back(Type::getInt32Ty(context));
		argValues.push_back(ret);
	}

	FunctionType *returnType = FunctionType::get(Type::getVoidTy(context), argTypes, false);	
	InlineAsm *asmIns = InlineAsm::get(returnType, "movl $$1,%eax; int $$0x80", "", true);
	builder.CreateCall(asmIns, argValues);
	builder.CreateUnreachable();
}

bool Compiler::emit(TargetMachine::CodeGenFileType fileType)
{
	if (this->targetTriple == "")
		setTarget();

	size_t typeindex = source.find_last_of(".");
	string filename = source.substr(0, typeindex);
	if (fileType == TargetMachine::CGFT_AssemblyFile)
		filename += ".s";
	else
		filename += ".o";

  std::error_code EC;
  raw_fd_ostream dest(filename, EC, sys::fs::F_None);

  if (EC) {
    errs() << "Could not open file: " << EC.message();
    return false;
  }

  legacy::PassManager pass;
	
	pass.add(createPrintModulePass(llvm::outs(), "Hello!"));

	if (target->addPassesToEmitFile(pass, dest, fileType, llvm::CodeGenOpt::Level::None)) {
    errs() << "The target machine can't emit a file of this type";
    return false;
  }

  pass.run(*module);
  dest.flush();

  outs() << "Wrote " << filename << "\n";

  return true;
}

}
