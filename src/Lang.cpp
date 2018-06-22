#include "Lang.h"

extern Cog::Compiler compiler;
extern vector<Cog::Symbol> symbols;
extern map<Cog::Typename, Cog::Type> types;

namespace Cog
{

Info *getConstant(char *str, int len)
{
	Info *result = new Info();
	result->constant = ConstantInt::get(compiler.context, APInt(32, atoi(str), true));
	result->Typename.id = "int";
	return result;
}

Info *getTypename(char *str, int len)
{
	Info *result = new Info();
	result->Typename.id = str;
	return result;
}

Info *getIdentifier(char *str, int len)
{
	for (int i = (int)symbols.size()-1; i >= 0; i--) {
		if (strcmp(symbols[i].name.c_str(), str) == 0) {
			Info *result = new Info();
			result->symbol = i;
			return result;
		}
	}

	return NULL;
}

}
