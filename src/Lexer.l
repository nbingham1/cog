%{
#include "Parser.y.h"

void yyerror(char*);

int column = 0;
int line = 0;
%}

l			[a-zA-Z_]
d			[0-9]
	/* x			[a-fA-F0-9] */
	/* e			[Ee][-+]?{d}+ */
	/* is		(u|i)({d}*) */
	/* fs		f(32|64) */

%%

	/* keywords */
"if"									{ column += yyleng; return IF; }
"else"								{ column += yyleng; return ELSE; }
"while"								{ column += yyleng; return WHILE; }

	/* primitive types */
int										{ column += yyleng; yylval.syntax = strndup(yytext, yyleng); return TYPENAME; }
	/* u?int({d}*)						{ return INTEGER; } */
	/* float(16|32|64|80|128)	{ return FLOAT; } */
	/* u?fixed({d}*)					{ return FIXED; } */

	/* constants */
{l}({l}|{d})*					{ column += yyleng; yylval.syntax = strndup(yytext, yyleng); return IDENTIFIER; }
	/* 0[xX]{x}+{is}?					{ return CONSTANT; } */
	/* {d}+{is}?							{ return CONSTANT; } */
{d}+									{ column += yyleng; yylval.syntax = strndup(yytext, yyleng); return CONSTANT; }

	/* {d}+{e}{fs}?						{ return CONSTANT; }
{d}*"."{d}+{e}?{fs}?	{ return CONSTANT; }
{d}+"."{d}*{e}?{fs}?	{ return CONSTANT; } */

"{"										{ column += yyleng; return '{'; }
"}"										{ column += yyleng; return '}'; }
"("										{ column += yyleng; return '('; }
")"										{ column += yyleng; return ')'; }

	/* operators */
"="										{ column += yyleng; return '='; }
"+"										{ column += yyleng; return '+'; }
"-"										{ column += yyleng; return '-'; }
"*"										{ column += yyleng; return '*'; }
"/"										{ column += yyleng; return '/'; }

";"										{ column += yyleng; return ';'; }

	/* misc */
[\n]									{ ++line; column = 0; }
[ \t\v\f]							{ column += yyleng; }
.											{ column += yyleng; /* ignore unmatched characters */ }

%%

int yywrap(void) {
	return 1;
}

