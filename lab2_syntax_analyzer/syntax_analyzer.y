%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/common.h"
#include "syntax_tree/SyntaxTree.h"

#include "lab1_lexical_analyzer/lexical_analyzer.h"

extern int yylex();
extern int yyparse();
extern FILE * yyin;

extern int lines;
extern char * yytext;

SyntaxTree * gt;

void yyerror(const char * s);
%}

%union {
/********** TODO: Fill in this union structure *********/
}

/********** TODO: Your token definition here ***********/

/* compulsory starting symbol */
%start program

%%
/*************** TODO: Your rules here *****************/
program :
 | ;

%%

void yyerror(const char * s)
{
	fprintf(stderr, "%s:%d syntax error for %s\n", s, lines, yytext);
}

void syntax(const char * input, const char * output)
{
	gt = newSyntaxTree();

	char inputpath[256] = "./testcase/";
	char outputpath[256] = "./syntree/";
	strcat(inputpath, input);
	strcat(outputpath, output);

	if (!(yyin = fopen(inputpath, "r"))) {
		fprintf(stderr, "[ERR] Open input file %s failed.", inputpath);
		exit(1);
	}
	printf("[START]: Syntax analysis start for %s\n", input);
	FILE * fp = fopen(outputpath, "w+");

	while (!feof(fp)) {
		yyparse();
	}

	printf("[OUTPUT] Printing tree to output file %s\n", outputpath);
	printSyntaxTree(stdout, gt);
	deleteSyntaxTree(gt);
	gt = 0;

	fclose(fp);
	printf("[END] Syntax analysis end for %s\n", input);
}

int syntax_main(int argc, char ** argv)
{
	char filename[10][256];
	char output_file_name[256];
	const char * suffix = ".syntax_tree";
	int fn = getAllTestcase(filename);
	for (int i = 0; i < fn; i++) {
			int name_len = strstr(filename[i], ".cminus") - filename[i];
			strncpy(output_file_name, filename[i], name_len);
			strcpy(output_file_name+name_len, suffix);
			syntax(filename[i], output_file_name);
	}
	return 0;
}
