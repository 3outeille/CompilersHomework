%option noyywrap
%{
/*****************声明和选项设置  begin*****************/
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
int files_count = 0;
/*int lines = 1;*/
/*int pos_start = 0;*/
/*int pos_end = 0;*/

enum cminus_token_type{
	ERROR = 258,
	ADD = 259,
	SUB = 260,
	MUL = 261,
	DIV = 262,
	LT = 263,
	LTE = 264,
	GT = 265,
	GTE = 266,
	EQ = 267,
	NEQ = 268,
	ASSIN = 269,
	SEMICOLON = 270,
	COMMA = 271,
	LPARENTHESE = 272,
	RPARENTHESE = 273,
	LBRACKET = 274,
	RBRACKET = 275,
	LBRACE = 276,
	RBRACE = 277,
	ELSE = 278,
	IF = 279,
	INT = 280,
	RETURN = 281,
	VOID = 282,
	WHILE = 283,
	IDENTIFIER = 284,
	NUMBER = 285,
	LETTER = 286,
	ARRAY = 287,
	EOL = 288,
	COMMENT = 289,
	BLANK = 290
};

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
NUM {DIGIT}+
LETTERS [a-zA-Z]
ID {LETTERS}+
 /*ID [a-zA-Z_][a-zA-Z0-9_]**/

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

/****************请按需求补全C代码 start*************/

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

/// \brief get all file paths under 'testcase' directory
///
/// under 'testcase' directory, there could be many *.cminus files.
/// \todo student should fill this function
void getAllTestcase(char filename[][256], char extension[]){
	DIR *d;
	struct dirent *dir;
	d = opendir("./testcase");
	int extlen = strlen(extension);
	int dnamelen;
	if (d) {
		while((dir = readdir(d)) != NULL) {
			dnamelen = strlen(dir->d_name);
			// check whether the extension matches
			if ((dnamelen > extlen) && (strcmp(dir->d_name + dnamelen - extlen, extension) == 0)) {
				strncpy(filename[++files_count - 1], dir->d_name, 255);
				printf("[INFO]: Found file %s to analyze.\n", dir->d_name);
			}
		}
		closedir(d);
	}
}

/// \brief process all *.cminus file
///
/// note that: use relative path for all i/o operations
///	process all *.cminus files under 'testcase' directory,
/// then create *.tokens files under 'tokens' directory
/// \todo student should fill this function
int main(int argc, char **argv){
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
