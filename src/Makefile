all:
	yacc -d *.y
	lex *.l
	g++ lex.yy.c y.tab.c -lstdcore -Istdcore/ -Lstdcore/ -o bas

