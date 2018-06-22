%token TYPENAME IDENTIFIER CONSTANT
%token IF ELSE WHILE
%left '+' '-'
%left '*' '/'
%union
{
	Info *info;
}

%{
#include <stdio.h>

void yyerror(char *);
int yylex(void);

extern FILE* yyin;
extern int line;
extern int column;

%}
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
	: type_specifier IDENTIFIER ';' {
									core::string name(core::wrapstr($<strValue>2));
									if (!symbol_index.contains(name)) {
										symbol_index.insert(name, symbol_table.size());
										$<intValue>$ = symbol_table.size();
										symbol_table.push_back(0);
									} else {
										$<intValue>$ = -1;
										fprintf(stderr, "%d:%d: variable already defined '%s'\n", line, column, $<strValue>1);
									}
								}

	;

assignment
	: inst_specifier '=' expression ';' {
																				int index = $<intValue>1;
																				if (index >= 0) {
																					$<intValue>$ = (symbol_table[index] = $<intValue>3);
																				} else {
																					fprintf(stderr, "%d:%d: lhs of assignment must be lval\n", line, column);
																				}
																			}
	;

expression
	: add_expression { $<intValue>$ = $<intValue>1; }
	;

add_expression
	: add_expression add_operator mult_expression {
																				if ($<intValue>2 == '+')
																					$<intValue>$ = $<intValue>1 + $<intValue>3;
																				else if ($<intValue>2 == '-')
																					$<intValue>$ = $<intValue>1 - $<intValue>3;
																				else
																					$<intValue>$ = 0;
																			}

	| mult_expression { $<intValue>$ = $<intValue>1; }
	;

mult_expression
	: mult_expression mult_operator unary_expression {
																				if ($<intValue>2 == '*')
																					$<intValue>$ = $<intValue>1 * $<intValue>3;
																				else if ($<intValue>2 == '/')
																					$<intValue>$ = $<intValue>1 / $<intValue>3;
																				else
																					$<intValue>$ = 0;
																			}

	| unary_expression { $<intValue>$ = $<intValue>1; }
	;

unary_expression
	: unary_operator primary_expression {
																				if ($<intValue>1 == '-')
																					$<intValue>$ = -$<intValue>2;
																				else
																					$<intValue>$ = $<intValue>2;
																			}
	| primary_expression { $<intValue>$ = $<intValue>1; }
	;

primary_expression
	: CONSTANT						{ $<intValue>$ = $<intValue>1; }
	| inst_specifier			{
													int index = $<intValue>1;
													if (index >= 0) {
														$<intValue>$ = symbol_table[index];
													} else {
														$<intValue>$ = 0;
													}
												}
	| '(' expression ')'	{ $<intValue>$ = $<intValue>2; }
	;

inst_specifier
	: IDENTIFIER	{
									core::string name(core::wrapstr($<strValue>1));
									symbol_key key = symbol_index.find(name);
									if (key) {
										$<intValue>$ = key->value;
										printf("Identifier %s found at %d with value %d\n", key->key.c_str(), key->value, symbol_table[key->value]);
									} else {
										$<intValue>$ = -1;
										fprintf(stderr, "%d:%d: undefined variable '%s'\n", line, column, $<strValue>1);
									}
								}
	;

type_specifier
	: INTEGER
	;

unary_operator
	: '+'	{ $<intValue>$ = '+'; }
	| '-'	{ $<intValue>$ = '-'; }
	;

mult_operator
	: '*'	{ $<intValue>$ = '*'; }
	| '/'	{ $<intValue>$ = '/'; }
	;

add_operator
	: '+' { $<intValue>$ = '+'; }
	| '-' { $<intValue>$ = '-'; }
	;

%%

void yyerror(char *s) {
	fprintf(stderr, "%d:%d: %s, found '%c'\n", line, column, s, yychar);
}

