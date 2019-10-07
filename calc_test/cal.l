%{
#include <stdio.h>
enum yytokentype { /*定义token*/
	NUMBER = 258,
	ADD = 259,
	SUB = 260,
	MUL = 261,
	DIV = 262,
	ABS = 263,
	EOL = 264
};
int yylval;
%}

%%
"+"			{ return ADD; }
"-"			{ return SUB; }
"*" 		{ return MUL; }
"/" 		{ return DIV; }
"|" 		{ return ABS; }
[0-9]+		{ yylval = atoi(yytext); return NUMBER; }
\n 			{ return EOL; }
[ \t] 		{ /*空白字符*/ }
.			{ printf("mystery character %c\n", *yytext); }
%%
