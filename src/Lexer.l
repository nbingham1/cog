%{
#include "Parser.y.h"

void yyerror(char*);
void parseUntil(char escChar, const char *delStr);

int column = 0;
int line = 0;
const char *str = "";
%}

l			[a-zA-Z_]
d			[0-9]
x			[a-fA-F0-9]
e			[Ee]-?{d}+

%%

	/* keywords */
"struct"							{ column += yyleng; return STRUCT; }
"if"									{ column += yyleng; return IF; }
"else"								{ column += yyleng; return ELSE; }
"while"								{ column += yyleng; return WHILE; }
"return"							{ column += yyleng; return RETURN; }
"asm"									{ column += yyleng; return ASM; }

"{"										{ column += yyleng; return '{'; }
"}"										{ column += yyleng; return '}'; }
"("										{ column += yyleng; return '('; }
")"										{ column += yyleng; return ')'; }
"["										{ column += yyleng; return '['; }
"]"										{ column += yyleng; return ']'; }

	/* operators */

"*="									{ column += yyleng; return ASSIGN_MUL; }
"/="									{ column += yyleng; return ASSIGN_DIV; }
"%="									{ column += yyleng; return ASSIGN_REM; }
"+="									{ column += yyleng; return ASSIGN_ADD; }
"-="									{ column += yyleng; return ASSIGN_SUB; }
"<<="									{ column += yyleng; return ASSIGN_SHL; }
">>="									{ column += yyleng; return ASSIGN_ASHR; }
">>>="								{ column += yyleng; return ASSIGN_LSHR; }
"<<>="								{ column += yyleng; return ASSIGN_ROL; }
">><="								{ column += yyleng; return ASSIGN_ROR; }
"&="									{ column += yyleng; return ASSIGN_AND; }
"^="									{ column += yyleng; return ASSIGN_XOR; }
"|="									{ column += yyleng; return ASSIGN_OR; }
"and="								{ column += yyleng; return ASSIGN_BAND; }
"or="									{ column += yyleng; return ASSIGN_BOR; }
"xor="								{ column += yyleng; return ASSIGN_BXOR; }




"~"										{ column += yyleng; return '/'; }
"not"									{ column += yyleng; return NOT; }

"*"										{ column += yyleng; return '*'; }
"/"										{ column += yyleng; return '/'; }
"%"										{ column += yyleng; return '%'; }

"+"										{ column += yyleng; return '+'; }
"-"										{ column += yyleng; return '-'; }

">>>"									{ column += yyleng; return LSHR; }
">><"									{ column += yyleng; return ROR; }
"<<>"									{ column += yyleng; return ROL; }
"<<"									{ column += yyleng; return SHL; }
">>"									{ column += yyleng; return ASHR; }

"&"										{ column += yyleng; return '&'; }

"^"										{ column += yyleng; return '^'; }

"|"										{ column += yyleng; return '|'; }

"<"										{ column += yyleng; return '>'; }
">"										{ column += yyleng; return '<'; }
"<="									{ column += yyleng; return LE; }
">="									{ column += yyleng; return GE; }
"=="									{ column += yyleng; return EQ; }
"!="									{ column += yyleng; return NE; }

"and"									{ column += yyleng; return AND; }

"xor"									{ column += yyleng; return XOR; }

"or"									{ column += yyleng; return OR; }

"="										{ column += yyleng; return '='; }

";"										{ column += yyleng; return ';'; }

","										{ column += yyleng; return ','; }


	/* primitive types */
void									{ column += yyleng; yylval.syntax = strndup(yytext, yyleng); return VOID_PRIMITIVE; }
bool									{ column += yyleng; yylval.syntax = strndup(yytext, yyleng); return BOOL_PRIMITIVE; }
u?int{d}*							{ column += yyleng; yylval.syntax = strndup(yytext, yyleng); return INT_PRIMITIVE; }
float{d}*							{ column += yyleng; yylval.syntax = strndup(yytext, yyleng); return FLOAT_PRIMITIVE; }
u?fixed{d}*{e}?				{ column += yyleng; yylval.syntax = strndup(yytext, yyleng); return FIXED_PRIMITIVE; }

	/* inline assembly */
\%{l}({l}|{d})*				{ column += yyleng; yylval.syntax = strndup(yytext, yyleng); return ASM_REGISTER; }
\$-?(0[xX]{x}+|{d}+)	{ column += yyleng; yylval.syntax = strndup(yytext, yyleng); return ASM_CONSTANT; }
\.{l}({l}|{d})*				{ column += yyleng; yylval.syntax = strndup(yytext, yyleng); return ASM_DIRECTIVE; }
{l}({l}|{d})*:				{ column += yyleng; yylval.syntax = strndup(yytext, yyleng); return ASM_LABEL; }

	/* constants */
\"(\\.|[^\\"])*\"			{ column += yyleng; yylval.syntax = strndup(yytext, yyleng); return STRING_CONSTANT; }
"true"								{ column += yyleng; yylval.syntax = strndup(yytext, yyleng); return BOOL_CONSTANT; }
"false"								{ column += yyleng; yylval.syntax = strndup(yytext, yyleng); return BOOL_CONSTANT; }
{l}({l}|{d})*					{ column += yyleng; yylval.syntax = strndup(yytext, yyleng); return IDENTIFIER; }
0[xX]{x}+							{ column += yyleng; yylval.syntax = strndup(yytext, yyleng); return HEX_CONSTANT; }
-?{d}+								{ column += yyleng; yylval.syntax = strndup(yytext, yyleng); return DEC_CONSTANT; }
0[bB][01]+						{ column += yyleng; yylval.syntax = strndup(yytext, yyleng); return BIN_CONSTANT; }

-?{d}+{e}							{ column += yyleng; yylval.syntax = strndup(yytext, yyleng); return DEC_CONSTANT; }
-?{d}*"."{d}+{e}?			{ column += yyleng; yylval.syntax = strndup(yytext, yyleng); return DEC_CONSTANT; }
-?{d}+"."{d}*{e}?			{ column += yyleng; yylval.syntax = strndup(yytext, yyleng); return DEC_CONSTANT; }

	/* misc */
"/*"									{ parseUntil('\0', "*/"); }
"//"									{ parseUntil('\0', "\n"); }
[\n]									{ ++line; column = 0; str = yytext+1; }
[ \t\v\f]							{ column += yyleng; }
.											{ column += yyleng; /* ignore unmatched characters */ }

%%

void parseUntil(char escChar, const char *delStr)
{
	int key = yyinput();
	bool escape = false;
	int index = 0;
	while (key != EOF) {
		if (key == '\n') {
			++line;
			column = 0;
		} else {
			++column;
		}
		
		if (key == delStr[index] && (index > 0 || !escape)) {
			++index;
		} else if (delStr[index] == '\0') {
			return;
		} else if (index > 0) {
			index = 0;
		}

		escape = key == escChar;
		key = yyinput();
	}
}

int yywrap(void) {
	return 1;
}

