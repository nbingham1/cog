%code requires {
#include "Lang.h"
}
%{
#include "Parser.y.h"

void yyerror(const char *s);
int yylex(void);

extern FILE* yyin;
extern int line;
extern int column;
extern const char* str;

%}

%token VOID_PRIMITIVE BOOL_PRIMITIVE INT_PRIMITIVE FLOAT_PRIMITIVE FIXED_PRIMITIVE
%token IDENTIFIER HEX_CONSTANT DEC_CONSTANT BIN_CONSTANT BOOL_CONSTANT STRING_CONSTANT
%token ASM STRUCT IF ELSE WHILE RETURN AND XOR OR NOT
%token LE GE NE EQ SHL ASHR LSHR ROL ROR
%token ASM_REGISTER ASM_CONSTANT ASM_DIRECTIVE ASM_LABEL
%left '+' '-'
%left '*' '/' '%'
%union {
	int token;
	char *syntax;
	Cog::Info *info;
}

%type<syntax> VOID_PRIMITIVE BOOL_PRIMITIVE INT_PRIMITIVE FLOAT_PRIMITIVE FIXED_PRIMITIVE IDENTIFIER HEX_CONSTANT DEC_CONSTANT BIN_CONSTANT BOOL_CONSTANT STRING_CONSTANT ASM_REGISTER ASM_CONSTANT ASM_DIRECTIVE ASM_LABEL

%%

program
	: program block
	| block
	;

block
	: structure_declaration
	| function_prototype
	| function_declaration
	;

structure_declaration
	: STRUCT IDENTIFIER '{' declaration_block '}' { Cog::structureDefinition($<syntax>2); }
	| STRUCT IDENTIFIER '{' '}' { Cog::structureDefinition($<syntax>2); }
	;

function_declaration
	: function_declaration '{' statement_list '}' { Cog::functionDefinition(); }
	| function_declaration '{' '}' { Cog::functionDefinition(); }
	;

function_prototype
	: type_specifier IDENTIFIER '(' type_list ')' ';' { Cog::functionPrototype($<info>1, $<syntax>2, $<info>4); }
	| type_specifier IDENTIFIER '(' ')' ';' { Cog::functionPrototype($<info>1, $<syntax>2, NULL); }
	;

function_declaration
	: type_specifier IDENTIFIER '(' declaration_list ')' { Cog::functionDeclaration($<info>1, $<syntax>2); }
	| type_specifier IDENTIFIER '(' ')' { Cog::functionDeclaration($<info>1, $<syntax>2); }
	;

statement_list
	: statement_list primary_statement
	| primary_statement
	;

primary_statement
	: variable_declaration ';'
	| assignment ';'
	| asm_block
	| call ';'
	| ret ';'
	| if_statement
	| while_statement
	;

statement_block
	: '{' statement_list '}'
	| primary_statement
	;

while_statement
	: while_condition statement_block { Cog::whileStatement(); }
	;

while_condition
	: while_keyword '(' expression ')' { Cog::whileCondition($<info>3); }
	;

while_keyword
	: WHILE { Cog::whileKeyword(); }
	;

if_statement
	: if_block else_condition statement_block { Cog::ifStatement(); }
	| if_block { Cog::ifStatement(); }
	;

if_block
	: if_block elseif_condition if_condition statement_block
	| if_condition statement_block
	;

if_condition
	: IF '(' expression ')' { Cog::ifCondition($<info>3); }
	;

elseif_condition
	: ELSE { Cog::elseifCondition(); }
	;

else_condition
	: ELSE { Cog::elseCondition(); }
	;

declaration_block
	: declaration_block variable_declaration ';'
	| variable_declaration ';'
	;

declaration_list
	: declaration_list ',' variable_declaration
	| variable_declaration
	;

variable_declaration
	: type_specifier IDENTIFIER { Cog::declareSymbol($<info>1, $<syntax>2); }
	;

type_list
	: type_list ',' type_specifier { Cog::infoList($<info>1, $<info>3); }
	| type_specifier { $<info>$ = $<info>1; }
	;

assignment
	: instance '=' expression { Cog::assignSymbol($<info>1, $<info>3); }
	;

asm_block
	: ASM '{' asm_statement_list '}'
	;

asm_statement_list
	: asm_statement_list asm_statement
	| asm_statement_list ';' asm_statement
	| asm_statement
	;

asm_statement
	: ASM_LABEL asm_instruction { printf("%s\n", $<syntax>1); }
	| asm_instruction
	;

asm_instruction
	: ASM_DIRECTIVE STRING_CONSTANT
	| ASM_DIRECTIVE IDENTIFIER
	| ASM_DIRECTIVE DEC_CONSTANT
	| ASM_DIRECTIVE HEX_CONSTANT
	| IDENTIFIER asm_argument_list	{ printf("%s\n", $<syntax>1); }
	| IDENTIFIER	{ printf("%s\n", $<syntax>1); }
	;

asm_argument_list
	: asm_argument_list ',' asm_argument
	| asm_argument
	;

asm_argument
	: ASM_REGISTER	{ printf("%s\n", $<syntax>1); }
	| ASM_CONSTANT	{ printf("%s\n", $<syntax>1); }
	| IDENTIFIER	{ printf("%s\n", $<syntax>1); }
	| '-' DEC_CONSTANT '(' asm_argument_list ')'	{ printf("%s\n", $<syntax>2); }
	| DEC_CONSTANT '(' asm_argument_list ')'	{ printf("%s\n", $<syntax>1); }
	| '(' asm_argument_list ')'
	;

call
	: IDENTIFIER '(' argument_list ')' { Cog::callFunction($<syntax>1, $<info>3); }
	| IDENTIFIER '(' ')' { Cog::callFunction($<syntax>1, NULL); }
	;

ret
	: RETURN expression { Cog::returnValue($<info>2); }
	| RETURN { Cog::returnVoid(); }
	;

expression
	: lOR_expression { $<info>$ = $<info>1; }
	;

lOR_expression
	: lOR_expression OR lXOR_expression { $<info>$ = Cog::binaryOperator($<info>1, $<token>2, $<info>3); }
	| lXOR_expression { $<info>$ = $<info>1; }
	;

lXOR_expression
	: lXOR_expression XOR lAND_expression { $<info>$ = Cog::binaryOperator($<info>1, $<token>2, $<info>3); }
	| lAND_expression { $<info>$ = $<info>1; }
	;

lAND_expression
	: lAND_expression AND rel_expression { $<info>$ = Cog::binaryOperator($<info>1, $<token>2, $<info>3); }
	| rel_expression { $<info>$ = $<info>1; }
	;

rel_expression
	: rel_expression rel_operator bOR_expression { $<info>$ = Cog::binaryOperator($<info>1, $<token>2, $<info>3); }
	| bOR_expression { $<info>$ = $<info>1; }
	;

rel_operator
	: '<' { $<token>$ = '<'; }
	| '>' { $<token>$ = '>'; }
	| LE { $<token>$ = LE; }
	| GE { $<token>$ = GE; }
	| EQ { $<token>$ = EQ; }
	| NE { $<token>$ = NE; }
	;

bOR_expression
	: bOR_expression '|' bXOR_expression { $<info>$ = Cog::binaryOperator($<info>1, $<token>2, $<info>3); }
	| bXOR_expression { $<info>$ = $<info>1; }
	;

bXOR_expression
	: bXOR_expression '^' bAND_expression { $<info>$ = Cog::binaryOperator($<info>1, $<token>2, $<info>3); }
	| bAND_expression { $<info>$ = $<info>1; }
	;

bAND_expression
	: bAND_expression '&' shift_expression { $<info>$ = Cog::binaryOperator($<info>1, $<token>2, $<info>3); }
	| shift_expression { $<info>$ = $<info>1; }
	;

shift_expression
	: shift_expression shift_operator add_expression { $<info>$ = Cog::binaryOperator($<info>1, $<token>2, $<info>3); }
	| add_expression { $<info>$ = $<info>1; }
	;

shift_operator
	: SHL  { $<token>$ = SHL; }
	| ASHR { $<token>$ = ASHR; }
	| LSHR { $<token>$ = LSHR; }
	| ROR { $<token>$ = ROR; }
	| ROL { $<token>$ = ROL; }
	;

add_expression
	: add_expression add_operator mult_expression { $<info>$ = Cog::binaryOperator($<info>1, $<token>2, $<info>3); }
	| mult_expression { $<info>$ = $<info>1; }
	;

add_operator
	: '+' { $<token>$ = '+'; }
	| '-' { $<token>$ = '-'; }
	;

mult_expression
	: mult_expression mult_operator unary_expression { $<info>$ = Cog::binaryOperator($<info>1, $<token>2, $<info>3); }
	| unary_expression { $<info>$ = $<info>1; }
	;

mult_operator
	: '*'	{ $<token>$ = '*'; }
	| '/'	{ $<token>$ = '/'; }
	| '%'	{ $<token>$ = '%'; }
	;

unary_expression
	: unary_operator primary_expression { $<info>$ = Cog::unaryOperator($<token>1, $<info>2); }
	| primary_expression { $<info>$ = $<info>1; }
	;

unary_operator
	: '-' { $<token>$ = '-'; }
	| NOT { $<token>$ = NOT; }
	| '~' { $<token>$ = '~'; }
	;

primary_expression
	: constant						{ $<info>$ = $<info>1; }
	| instance						{ $<info>$ = $<info>1; }
	| '(' expression ')'	{ $<info>$ = $<info>2; }
	;

argument_list
	: argument_list ',' expression { $<info>$ = Cog::infoList($<info>1, $<info>3); }
	| expression { $<info>$ = $<info>1; }
	;

constant
	: BOOL_CONSTANT	{ $<info>$ = Cog::getConstant(BOOL_CONSTANT, $<syntax>1); }
	| HEX_CONSTANT	{ $<info>$ = Cog::getConstant(HEX_CONSTANT, $<syntax>1); }
	| DEC_CONSTANT	{ $<info>$ = Cog::getConstant(DEC_CONSTANT, $<syntax>1); }
	| BIN_CONSTANT	{ $<info>$ = Cog::getConstant(BIN_CONSTANT, $<syntax>1); }
	| STRING_CONSTANT	{ $<info>$ = Cog::getConstant(STRING_CONSTANT, $<syntax>1); }
	;

instance
	: IDENTIFIER	{ $<info>$ = Cog::getIdentifier($<syntax>1); }
	;

type_specifier
	: VOID_PRIMITIVE { $<info>$ = Cog::getTypename(VOID_PRIMITIVE, $<syntax>1); }
	| BOOL_PRIMITIVE { $<info>$ = Cog::getTypename(BOOL_PRIMITIVE, $<syntax>1); }
	| INT_PRIMITIVE { $<info>$ = Cog::getTypename(INT_PRIMITIVE, $<syntax>1); }
	| FLOAT_PRIMITIVE { $<info>$ = Cog::getTypename(FLOAT_PRIMITIVE, $<syntax>1); }
	| FIXED_PRIMITIVE { $<info>$ = Cog::getTypename(FIXED_PRIMITIVE, $<syntax>1); }
	| IDENTIFIER { $<info>$ = Cog::getTypename(IDENTIFIER, $<syntax>1); }
	;

%%

void yyerror(const char *s) {
	fprintf(stderr, "%d:%d: %s\n", line, column, s);
	fprintf(stderr, "%s\n", str);
	for (int i = 0; i < column-1; i++) {
		if (str[i] <= ' ' || str[i] == 127)
			fputc(str[i], stderr);
		else
			fputc(' ', stderr);
	}
	fputc('^', stderr);
	for (const char *c = str+column+1; c; c++)
		fputc('~', stderr);
	fputc('\n', stderr);
}

