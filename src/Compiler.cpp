#include "Compiler.h"
#include <llvm/IR/IRPrintingPasses.h>

using namespace std;

namespace Cog
{

Compiler::Compiler() : builder(context)
{
	targetTriple = "";
}

Compiler::~Compiler()
{
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

}
