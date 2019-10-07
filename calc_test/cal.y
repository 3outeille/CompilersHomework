%{
//在%{和%}中的代码会被原样照抄到生成的name.tab.c文件的开头，可以在这里书写声明与定义
#include <stdio.h>
%}

/*声明tokens，告诉bison在语法分析程序中记号的名称*/
%token NUMBER ADD SUB MUL DIV ABS EOL
/*%token ADD SUB MUL DIV ABS*/
/*%token EOL*/

%%
/*该部分定义规则，C的动作代码在每条规则后用花括号括起*/
calclist: /*空规则*/
	| calclist exp EOL { printf("output = %d\n", $2); }
	;
exp:factor { $$ = $1; }
	| exp ADD factor { $$ = $1 + $3; }
	| exp SUB factor { $$ = $1 - $3; }
	;
factor:term { $$ = $1; }
	| factor MUL term { $$ = $1 * $3; }
	| factor DIV term { $$ = $1 / $3; }
	;
term:NUMBER { $$ = $1; }
	| ABS term { $$ = $2 >= 0 ? $2 : -$2; }
	;
%%

int main(int argc, char **argv){
extern int yydebug;
yydebug = 1;
    yyparse();
    return 0;
}
int yyerror(char *s) {
    fprintf(stderr, "error: %s\n", s);
}
