#include "Compiler.h"
#include <llvm/IR/IRPrintingPasses.h>

using namespace std;

extern int line;
extern int column;
extern const char *str;

namespace Cog
{

Compiler::Compiler() : builder(context)
{
	targetTriple = "";
	scopes.push_back(Scope());
	currFn = NULL;
}

Compiler::~Compiler()
{
	for (auto type = types.begin(); type != types.end(); type++) {
		delete *type;
	}
	types.clear();
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
	scopes.push_back(Scope(getScope()));
}

void Compiler::popScope()
{
	scopes[scopes.size()-2].merge(&scopes.back());
	scopes.pop_back();
}

Type *Compiler::getType(Type *newType)
{
	for (auto type = types.begin(); type != types.end(); type++) {
		if ((*type)->eq(newType)) {
			delete newType;
			return *type;
		}
	}

	types.push_back(newType);
	return newType;
}

void Compiler::loadFile(string filename)
{
	
	module = new llvm::Module(filename, context);
	source = filename;
}

bool Compiler::setTarget(string targetTriple)
{
	if (!module) {
		llvm::errs() << "Module not yet initialized";
		return false;
	} else if (targetTriple != this->targetTriple) {
		if (this->targetTriple == "") {
			llvm::InitializeAllTargetInfos();
			llvm::InitializeAllTargets();
			llvm::InitializeAllTargetMCs();
			llvm::InitializeAllAsmParsers();
			llvm::InitializeAllAsmPrinters();
		}

		this->targetTriple = targetTriple;
		module->setTargetTriple(targetTriple);

		string error;
		const llvm::Target *targetEntry = llvm::TargetRegistry::lookupTarget(targetTriple, error);
		if (!targetEntry) {
			llvm::errs() << "error: " << error;
			return false;
		}

		llvm::TargetOptions options;
		llvm::Optional<llvm::Reloc::Model> relocModel;
		target = targetEntry->createTargetMachine(targetTriple, "generic", "", options, relocModel);

		module->setDataLayout(target->createDataLayout());
	}
	return true;
}

bool Compiler::emit(llvm::TargetMachine::CodeGenFileType fileType)
{
	if (this->targetTriple == "")
		setTarget();

	size_t typeindex = source.find_last_of(".");
	string filename = source.substr(0, typeindex);
	if (fileType == llvm::TargetMachine::CGFT_AssemblyFile)
		filename += ".s";
	else
		filename += ".o";

  std::error_code EC;
  llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::F_None);

  if (EC) {
    llvm::errs() << "Could not open file: " << EC.message();
    return false;
  }

  llvm::legacy::PassManager pass;
	
	pass.add(llvm::createPrintModulePass(llvm::outs(), "Hello!"));

	if (target->addPassesToEmitFile(pass, dest, fileType, llvm::CodeGenOpt::Level::None)) {
    llvm::errs() << "The target machine can't emit a file of this type";
    return false;
  }

  pass.run(*module);
  dest.flush();

  llvm::outs() << "Wrote " << filename << "\n";

  return true;
}

std::ostream &error_(const char *dfile, int dline)
{
	cout << str << endl;
	cout << dfile << ":" << dline << " error " << line << ":" << column << ": ";
	return cout;
}

}
