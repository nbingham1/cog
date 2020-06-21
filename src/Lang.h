#pragma once

#include <parse/grammar.h>
#include <parse/lexer.h>
#include "Type.h"
#include "Symbol.h"
#include "cog.h"

namespace Cog
{

extern ::parse::cog_t cog;

Type *cogTypename(parse::token_t token, parse::lexer_t &lexer);
void cogLang(parse::token_t token, parse::lexer_t &lexer);
void cogFunction(parse::token_t token, parse::lexer_t &lexer);
void cogConstant(parse::token_t token, parse::lexer_t &lexer);

}

