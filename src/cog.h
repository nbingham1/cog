#pragma once

#include <parse/grammar.h>

namespace parse
{

struct cog_t
{
	int32_t COG;
	int32_t STRUCTDEF;
	int32_t FUNCTIONDEF;
	int32_t OUTDECL;
	int32_t TYPENAME;
	int32_t INDECL;
	int32_t BLOCK;
	int32_t STATEMENT;
	int32_t EXPRESSION;
	int32_t IF_STMT;
	int32_t WHILE_LOOP;
	int32_t ASSIGN;
	int32_t EXP13;
	int32_t EXP12;
	int32_t EXP11;
	int32_t EXP10;
	int32_t EXP9;
	int32_t EXP8;
	int32_t EXP7;
	int32_t EXP6;
	int32_t EXP5;
	int32_t EXP4;
	int32_t EXP2;
	int32_t CONSTANT;
	int32_t EXP1;
	int32_t BOOL;
	int32_t INT;
	int32_t STRING;
	int32_t REAL;

	void load(grammar_t &grammar);
};

}

