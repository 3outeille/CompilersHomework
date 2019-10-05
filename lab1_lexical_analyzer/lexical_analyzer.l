%option noyywrap
%{
/*****************声明和选项设置  begin*****************/
#include <stdio.h>
#include <stdlib.h>

#include "lab1_lexical_analyzer/lexical_analyzer.h"
#include "common/common.h"

#ifndef LAB1_ONLY
#include "syntax_analyzer.h"
#endif

/* try to use bison location but failed */
/*#include "parser.tab.h"*/
/*int yycolumn = 1;*/
/*#define YY_USER_ACTION \*/
    /*yylloc->first_line = yylloc->last_line; \*/
    /*yylloc->first_column = yylloc->last_column; \*/
    /*for(int i = 0; yytext[i] != '\0'; i++) { \*/
        /*if(yytext[i] == '\n') { \*/
            /*yylloc->last_line++; \*/
            /*yylloc->last_column = 0; \*/
        /*} \*/
        /*else { \*/
            /*yylloc->last_column++; \*/
        /*} \*/
    /*}*/
typedef struct {
	int first_line;
	int first_column;
	int last_line;
	int last_column;
}myloc;
myloc ml;
myloc* pmyloc = &ml;
#define YY_USER_ACTION \
    pmyloc->first_line = pmyloc->last_line; \
    pmyloc->first_column = pmyloc->last_column; \
    for(int i = 0; yytext[i] != '\0'; i++) { \
	if(yytext[i] == '\n') { \
	    pmyloc->last_line++; \
	    pmyloc->last_column = 1; \
	} \
	else { \
	    pmyloc->last_column++; \
	} \
    }
/*****************end*****************/

%}

DIGIT [0-9]
 /*NUM {DIGIT}+*/
NUM (0|[1-9]{DIGIT}*)
LETTERS [a-zA-Z]
ID {LETTERS}+
 /*ID [a-zA-Z_][a-zA-Z0-9_]**/

 /* TA's patterns */
 /*commentPattern "/*"([^\*]|(\*)*[^\*/])*(\*)*"*/"*/
 /*digitPattern [0-9]*/
 /*numberPattern (0|[1-9]{digitPattern}*)*/
 /*identifierPattern [a-zA-Z][a-zA-Z]**/

%%

 /****请在此补全所有flex的模式与动作  start******/

\n {return EOL;}
 /* comments */
\/\*(\/?([^\*\/]|\n)*\*?([^\*\/]|\n)+\/?)**\*\/ {return COMMENT;}
 /* special symbols */
\+ {return ADD;}
\- {return SUB;}
\* {return MUL;}
\/ {return DIV;}
\< {return LT;}
\<= {return LTE;}
\> {return GT;}
\>= {return GTE;}
== {return EQ;}
!= {return NEQ;}
= {return ASSIN;}
; {return SEMICOLON;}
, {return COMMA;}
\( {return LPARENTHESE;}
\) {return RPARENTHESE;}
\[ {return LBRACKET;}
\] {return RBRACKET;}
\{ {return LBRACE;}
\} {return RBRACE;}
\[\] {return ARRAY;}
 /* keywords */
else {return ELSE;}
if {return IF;}
int {return INT;}
return {return RETURN;}
void {return VOID;}
while {return WHILE;}
 /* identifiers and numbers */
 /*{LETTERS} {return LETTER;}*/
{ID} {return IDENTIFIER;}
{NUM} {return NUMBER;}
 /* blank */
[ \t] {return BLANK;}
. {return ERROR;}


 /****  end******/
%%

/// \brief analysize a *.cminus file
///
///	\param input_file_name
/// \param output_file_name
/// \todo student should fill this function
void analyzer(char* input_file_name, char* output_file_name){
	char input_path[256] = "./testcase/";
	strcat(input_path, input_file_name);
	char output_path[256] = "./tokens/";
	strcat(output_path, output_file_name);
	// yyin is the input stream pointer
	if(!(yyin = fopen(input_path,"r"))){
		printf("[ERR] No input file\n");
		exit(1);
	}
	printf("[START]: Read from: %s\n", input_file_name);
	FILE *fp = fopen(output_path,"w+");
	int token;
	// yylex(): entry point of lex, read yyin, return zero on end 
	while(token = yylex()){
		switch(token){
			case ERROR:
				printf("[ERR]: unable to analysize %s at %d line, from %d to %d\n", yytext, 
				fprintf(fp, "[ERR]: unable to analysize %s at %d line, from %d to %d\n", yytext, 
						pmyloc->first_line, pmyloc->first_column, pmyloc->last_column);
				break;
			case COMMENT:
				/*printf("[INFO]: Comment found.\n");*/
			case BLANK:
				/*printf("[INFO]: Blank found.\n");*/
				break;
			case EOL:
				break;
			default :
				/*fprintf(fp, "%s\t%d\t%d\t%d\t%d\n",yytext, token, lines, pos_start, pos_end);*/
				fprintf(fp, "%s\t%d\t%d\t%d\t%d\n",yytext, token, \
						pmyloc->first_line, pmyloc->first_column, pmyloc->last_column);
		}
	}
	fclose(fp);
	printf("[END]: Analysis completed.\n");
}


/// \brief process all *.cminus file
///
/// note that: use relative path for all i/o operations
///	process all *.cminus files under 'testcase' directory,
/// then create *.tokens files under 'tokens' directory
/// \todo student should fill this function
int lex_main(int argc, char **argv){
	char filename[10][256];
	char output_file_name[256];
	char suffix[] = ".tokens";
	char extension[] = ".cminus";
	getAllTestcase(filename, extension);
	for(int i = 0; i < files_count; i++){
		// reinitialize position counter
		pmyloc->first_line = pmyloc->first_column = pmyloc->last_line = pmyloc->last_column = 1;
		// prepare output filename
		strcpy(output_file_name, filename[i]);
		strcpy(output_file_name + strlen(output_file_name) - strlen(extension), suffix);
		analyzer(filename[i],output_file_name);
	}
	return 0;
}
/**************** end*************/
