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
	| function_prototype { delete $<info>1; }
	| function_definition
	;

structure_declaration
	: STRUCT IDENTIFIER '{' declaration_block '}' { Cog::structureDefinition($<syntax>2); }
	| STRUCT IDENTIFIER '{' '}' { Cog::structureDefinition($<syntax>2); }
	;

function_definition
	: function_declaration '{' statement_list '}' { Cog::functionDefinition(); }
	| function_declaration asm_block { Cog::asmFunctionDefinition(); }
	| function_declaration '{' '}' { Cog::functionDefinition(); }
	;

function_prototype
	: type_specifier IDENTIFIER '(' declaration_list ')' ';' { $<info>$ = Cog::functionPrototype($<info>1, $<syntax>2); }
	| type_specifier IDENTIFIER '(' ')' ';' { $<info>$ = Cog::functionPrototype($<info>1, $<syntax>2); }
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
	: ASM '{' asm_statement_list '}' { Cog::asmBlock($<info>3); }
	;

asm_statement_list
	: asm_statement_list asm_statement { $<info>$ = Cog::infoList($<info>1, $<info>2); }
	| asm_statement_list ';' asm_statement { $<info>$ = Cog::infoList($<info>1, $<info>3); }
	| asm_statement { $<info>$ = $<info>1; }
	;

asm_statement
	: ASM_LABEL asm_instruction { $<info>$ = asmStatement($<syntax>1, $<info>2); }
	| asm_instruction { $<info>$ = $<info>1; }
	;

asm_instruction
	: IDENTIFIER asm_argument_list	{ $<info>$ = Cog::asmInstruction($<syntax>1, $<info>2); }
	| IDENTIFIER	{ $<info>$ = Cog::asmInstruction($<syntax>1, NULL); }
	;

asm_argument_list
	: asm_argument_list ',' asm_argument { $<info>$ = Cog::infoList($<info>1, $<info>3); }
	| asm_argument_list ',' asm_memory_arg { $<info>$ = Cog::infoList($<info>1, $<info>3); }
	| asm_argument { $<info>$ = $<info>1; }
	| asm_memory_arg { $<info>$ = $<info>1; }
	;

asm_memory_arg
	: ASM_REGISTER ':' DEC_CONSTANT asm_address	{ $<info>$ = Cog::asmMemoryArg($<syntax>1, $<syntax>3, $<info>4); }
	| DEC_CONSTANT asm_address									{ $<info>$ = Cog::asmMemoryArg(NULL, $<syntax>1, $<info>2); }
	;

asm_address
	: '(' asm_argument ',' asm_argument ',' DEC_CONSTANT ')'	{ $<info>$ = Cog::asmAddress($<info>2, $<info>4, $<syntax>6); }
	| '(' asm_argument ',' asm_argument ')'										{ $<info>$ = Cog::asmAddress($<info>2, $<info>4, NULL); }
	| '(' asm_argument ')'																		{ $<info>$ = Cog::asmAddress($<info>2, NULL, NULL); }
	| '(' ')'																									{ $<info>$ = Cog::asmAddress(NULL, NULL, NULL); }
	| '(' asm_argument ',' ',' DEC_CONSTANT ')'								{ $<info>$ = Cog::asmAddress($<info>2, NULL, $<syntax>5); }
	| '(' ',' asm_argument ',' DEC_CONSTANT ')'								{ $<info>$ = Cog::asmAddress(NULL, $<info>3, $<syntax>5); }
	| '(' ',' DEC_CONSTANT ')'																{ $<info>$ = Cog::asmAddress(NULL, $<info>3, NULL); }
	;

asm_argument
	: ASM_REGISTER	{ $<info>$ = Cog::asmRegister($<syntax>1); }
	| ASM_CONSTANT	{ $<info>$ = Cog::asmConstant($<syntax>1); }
	| IDENTIFIER	{ $<info>$ = Cog::getIdentifier($<syntax>1); }
	| IDENTIFIER '(' type_list ')' { $<info>$ = Cog::asmFunction($<syntax>1, $<info>3); }
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

