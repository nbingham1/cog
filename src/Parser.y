%{
#pragma once

#include <stdio.h>
#include "Lang.h"

int yylex(void);

extern FILE* yyin;
extern int line;
extern int column;

void yyerror(char *s) {
	fprintf(stderr, "%d:%d: %s, found '%c'\n", line, column, s, yychar);
}

%}

%token TYPENAME IDENTIFIER CONSTANT
%token IF ELSE WHILE
%token LE GE NE EQ
%left '+' '-'
%left '*' '/'
%union {
	int token;
	char *syntax;
	Cog::Info *info;
}

%type<syntax> TYPENAME IDENTIFIER CONSTANT 

%%

statement_list
	: statement_list primary_statement
	| primary_statement
	;

primary_statement
	: declaration
	| assignment
	| if_statement
	| while_statement
	| '{' statement_list '}'
	;

while_statement
	: WHILE '(' expression ')' primary_statement
	;

if_statement
	: if_statement ELSE primary_statement
	| IF '(' expression ')' primary_statement
	;

declaration
	: type_specifier IDENTIFIER ';' { Cog::declareSymbol($<info>1, $<syntax>2); }
	;

assignment
	: inst_specifier '=' expression ';' { Cog::assignSymbol($<info>1, $<info>3); }
	;

expression
	: add_expression { $<info>$ = $<info>1; }
	;

add_expression
	: add_expression add_operator mult_expression { $<info>$ = Cog::binaryOperator($<info>1, $<token>2, $<info>3); }
	| mult_expression { $<info>$ = $<info>1; }
	;

mult_expression
	: mult_expression mult_operator unary_expression { $<info>$ = Cog::binaryOperator($<info>1, $<token>2, $<info>3); }
	| unary_expression { $<info>$ = $<info>1; }
	;

unary_expression
	: unary_operator primary_expression { $<info>$ = Cog::unaryOperator($<token>1, $<info>2); }
	| primary_expression { $<info>$ = $<info>1; }
	;

primary_expression
	: CONSTANT						{ $<info>$ = Cog::getConstant($<syntax>1); }
	| inst_specifier			{ $<info>$ = $<info>1; }
	| '(' expression ')'	{ $<info>$ = $<info>2; }
	;

inst_specifier
	: IDENTIFIER	{ $<info>$ = Cog::getIdentifier($<syntax>1); }
	;

type_specifier
	: TYPENAME { $<info>$ = Cog::getTypename($<syntax>1); }
	;

unary_operator
	: '+' { $<token>$ = '+'; }
	| '-' { $<token>$ = '-'; }
	;

mult_operator
	: '*'	{ $<token>$ = '*'; }
	| '/'	{ $<token>$ = '/'; }
	;

add_operator
	: '+' { $<token>$ = '+'; }
	| '-' { $<token>$ = '-'; }
	;

%%

