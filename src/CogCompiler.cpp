#include "CogCompiler.h"

CogCompiler::CogCompiler()
{
	targetTriple = "";
}

CogCompiler::~CogCompiler()
{
}

void CogCompiler::loadFile(string filename)
{
	module = new Module(filename, context);
	source = filename;
}

bool CogCompiler::setTarget(string targetTriple)
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


void CogCompiler::createExit(BasicBlock *body, Value *ret)
{
	vector<Type*> argTypes;
	vector<Value*> argValues;

	if (ret != NULL) {
		argTypes.push_back(Type::getInt32Ty(context));
		argValues.push_back(ret);
	}

	FunctionType *returnType = FunctionType::get(Type::getVoidTy(context), argTypes, false);
	InlineAsm *asmIns = InlineAsm::get(returnType, "movl $$1,%eax; int $$0x80", "", true);
	CallInst::Create(asmIns, argValues, "", body);
}

bool CogCompiler::emit(TargetMachine::CodeGenFileType fileType)
{
	if (this->targetTriple == "")
		setTarget();

  auto Filename = "output.o";
  std::error_code EC;
  raw_fd_ostream dest(Filename, EC, sys::fs::F_None);

  if (EC) {
    errs() << "Could not open file: " << EC.message();
    return false;
  }

  legacy::PassManager pass;

  if (target->addPassesToEmitFile(pass, dest, fileType)) {
    errs() << "The target machine can't emit a file of this type";
    return false;
  }

  pass.run(*module);
  dest.flush();

  outs() << "Wrote " << Filename << "\n";

  return true;
}

