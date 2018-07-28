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

BaseType *Compiler::getFunction(Typename retType, Typename thisType, std::string name, const std::vector<Declaration> &args)
{
	for (auto type = types.begin(); type != types.end(); type++) {
		if (type->name == name
		 && type->retType == retType
		 && type->thisType == thisType
		 && type->args == args) {
			return &(*type);
		}
	}

	types.push_back(BaseType());
	BaseType *result = &types.back();
	result->retType = retType;
	result->thisType = thisType;
	result->name = name;
	result->args = args;

	std::vector<llvm::Type*> llvmArgs;
	llvmArgs.reserve(args.size()+1);
	if (thisType.isSet() && (thisType.prim == NULL || thisType.prim->kind != PrimType::Void))
		llvmArgs.push_back(thisType.getLlvm());

	for (auto arg = args.begin(); arg != args.end(); arg++) {
		llvmArgs.push_back(arg->type.getLlvm());
	}

	result->llvmType = FunctionType::get(retType.getLlvm(), llvmArgs, false);
	return result;
}

BaseType *Compiler::getFunction(Typename retType, Typename thisType, std::string name, const std::vector<Symbol> &symbols)
{
	std::vector<Declaration> args;
	args.reserve(symbols.size());
	for (auto symbol = symbols.begin(); symbol != symbols.end(); symbol++)
		args.push_back(Declaration(&(*symbol)));

	return getFunction(retType, thisType, name, args);
}

BaseType *Compiler::getStructure(std::string name, const std::vector<Declaration> &args)
{
	for (auto type = types.begin(); type != types.end(); type++) {
		if (type->name == name
		 && !type->retType.isSet()
		 && !type->thisType.isSet()
		 && type->args == args) {
			return &(*type);
		}
	}

	types.push_back(BaseType());
	BaseType *result = &types.back();
	result->name = name;
	result->args = args;

	std::vector<llvm::Type*> llvmArgs;
	for (auto arg = args.begin(); arg != args.end(); arg++) {
		llvmArgs.push_back(arg->type.getLlvm());
	}

	result->llvmType = StructType::create(context, llvmArgs, name, false);

	return result;
}

BaseType *Compiler::getStructure(std::string name, const std::vector<Symbol> &symbols)
{
	std::vector<Declaration> args;
	args.reserve(symbols.size());
	for (auto symbol = symbols.begin(); symbol != symbols.end(); symbol++)
		args.push_back(Declaration(&(*symbol)));

	return getStructure(name, args);
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

std::ostream &error_(const char *dfile, int dline)
{
	cout << str << endl;
	cout << dfile << ":" << dline << " error " << line << ":" << column << ": ";
	return cout;
}

}
