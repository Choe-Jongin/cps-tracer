/****************************************************************************/
/*                                                                          */
/*                     This code is tracer for lightnvm                     */
/*         Adds a code to the target file that helps with tracking.         */
/*       This may slow down the program or require additional memory.       */
/*               For more information, please visit our github              */
/*              https://github.com/Choe-Jongin?tab=repositories             */
/*              copyrightⓒ 2022 All rights reserved by CPSLAB              */
/*                                                                          */
/****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

#define INSERT 0
#define REMOVE 1

#define TRUE 1
#define FALSE 0

#define FUNC 0
#define HEAD 1
#define BRAK 2

#define MAX_FILE_NAME 256
#define MAX_READ_BUFF 1024
#define MAX_ARGS_BUFF 512

int cps_fileopen(char *filename, int (*modify)(char *), int type, int *errcnt);
int cps_insert(char *filename);
int cps_remove(char *filename);
int add_filelist(char filename[MAX_FILE_NAME], int *index, char *name);
int have_header(char *filename);
void set_valid_char(char *dest, char c, int *index, const char *invalid);
int parse_args(char *args_str, char *parsered_str);
void replacefile(char * oldname, char * newname);

char * str_modi_mode[2] = {" \033[0;32mINSERT\033[0m ", " \033[0;31mREMOVE\033[0m "};
FILE * targetsfile;

int main(int argc, char *argv[])
{
	int (*modify)(char *);
	char filenames[100][MAX_FILE_NAME];
	int fileindex = 0;
	int type = INSERT;
	int errcnt = 0;

	targetsfile = fopen("cps_target.txt", "w");
	fprintf(targetsfile,"LEVEL 0\n");

	// arg parsing
	for (int i = 1; i < argc; i++)
	{
		char str[1024];
		char *field;
		char *value = NULL;

		strcpy(str, argv[i]);
		field = strtok(str, ":");
		if (str != NULL)
			value = strtok(NULL, ":");

		// if no field:value form, field is considered a file name.
		if (value == NULL)
		{
			//if field is rm, it mean type:rm
			if( strcmp(field, "rm") == 0)
				type = REMOVE;
			else
				add_filelist(filenames[fileindex], &fileindex, field);
			continue;
		}

		// assign a value to that field
		if (strcmp(field, "file") == 0)
		{
			add_filelist(filenames[fileindex], &fileindex, value);
			continue;
		}

		if (strcmp(field, "type") == 0)
		{
			if (strcmp(value, "remove") == 0 || strcmp(value, "rm") == 0)
				type = REMOVE;
			else if (strcmp(value, "insert") == 0 || strcmp(value, "in") == 0)
				type = INSERT;
			else
			{
				printf("%s is not supported\n", value);
				return 1;
			}
			continue;
		}
	}

	if (fileindex == 0)
	{
		printf("At least one file is required.\n");
		return 1;
	}

	// set modify function
	if (type == INSERT)
		modify = cps_insert;
	else if (type == REMOVE)
		modify = cps_remove;

	for (int i = 0; i < fileindex; i++)
	{
		// dir
		if (filenames[i][strlen(filenames[i]) - 1] == '/')
		{
			DIR *dir;
			struct dirent *ent;
			char dirpath[256];

			// check target directory
			strcpy(dirpath, filenames[i]);

			dir = opendir(dirpath);
			if (dir == NULL)
			{
				printf("no dir\n");
				return 1;
			}

			while ((ent = readdir(dir)) == NULL)
			{
				char fullpath[256] = "";
				strcat(fullpath, dirpath);
				strcat(fullpath, ent->d_name);
				
				cps_fileopen(fullpath, modify, type, &errcnt);
			}

			closedir(dir);
		}
		else // specific file
		{
			cps_fileopen(filenames[i], modify, type, &errcnt);
		}
	}

	if (errcnt > 0)
	{
		printf("%d error\n", errcnt);
	}


	fprintf(targetsfile,"LEVEL 1\n");
	fprintf(targetsfile,"LEVEL 2\n");
	printf("finish\n");
	return 0;
}

int cps_fileopen(char *filename, int (*modify)(char *), int type, int * errcnt)
{
	printf("[%s]%-20s", str_modi_mode[type], filename);

	if (modify(filename) != 0)
	{
		++(*errcnt);
		return 1;
	}

	int filestart_i = strlen(filename);
	while (filestart_i > 0 && filename[filestart_i-1] != '/')
		filestart_i--;
	
	fprintf(targetsfile,"%s *\n", &filename[filestart_i]);
	printf("Done.\n");
	return 0;
}

// add trace code
int cps_insert(char *filename)
{
	//remove first
	if(cps_remove(filename) != 0)
		return 1;		// remove fail!!

	FILE *fin = fopen(filename, "r");
	FILE *fout = fopen(".cps-temp.c", "w");
	char read_str[MAX_READ_BUFF];
	char args_str[MAX_READ_BUFF];
	int args_i = 0;
	int line = 1;
	int col = 0;
	char c = '\0';
	char c_prev = '\0';

	int args_bracket = 0; // # of '()' opened
	int func_bracket = 0; // # of '{}' opened
	int comment = 0;	  // is comment?
	int func_read = 0;	  // function block start

	if (fin == NULL || fout == NULL)
	{
		fprintf(stderr, "can't open the %s\n", filename);
		return 1;
	}

	if (have_header(filename) == FALSE)
	{
		printf("no header\n");
		return 1;
	}

	/* success file open  */

	// for call stack
	// fputs("#pragma GCC push_options //CPS_FUNCTION Save current options\n", fout);
	// fputs("#pragma GCC optimize (\"no-optimize-sibling-calls\") //CPS_FUNCTION\n", fout);

	//read one line
	while (fgets(read_str, MAX_READ_BUFF, fin) != NULL)
	{
		//append to temp file
		fputs(read_str, fout);
		//check char one by one
		int line_len = strlen(read_str);
		for (int i = 0; i < line_len; i++)
		{
			c = read_str[i];
			switch (c)
			{
			// start or end comment
			case '*':
				if (c_prev == '/')
					comment = 1;
				break;

			case '/':
				if (c_prev == '*' && comment == 1)
					comment = 0;

				if (c_prev == '/')
					comment = 2;
				break;
			}

			if (c != '\n')
				col++;

			// ignore comment
			if (comment != 0)
			{
				c_prev = c;
				continue;
			}

			// check bracket
			if( c == '(')
				args_bracket++;
			if( c == ')')
				args_bracket--;
			if( c == '{')
				func_bracket++;
			if( c == '}')
				func_bracket--;

			//exclude declare
			if (c == ';')
				func_read = 0;

			// if read args
			if (args_bracket != 0)
			{
				if (args_i < 1024)
					args_str[args_i++] = c;
			}
			// end args read
			else if (args_bracket == 0 && args_i != 0)
			{
				args_str[args_i++] = c;
				args_str[args_i] = '\0';
				args_i = 0;
				func_read++;
			}
			else if (c == '{' && func_bracket == 1 && func_read == 1)
			{
				char parsed[MAX_ARGS_BUFF] = "";
				char CPS_TEXT[MAX_READ_BUFF];
				int argc = parse_args(args_str, parsed);
				if (argc > 0)
					sprintf(CPS_TEXT, "CPS_FUNCTION(CPS_CALLER, CPS_CALLEE, __FILE__, __func__, %d, %s);", argc, parsed);
				else
					sprintf(CPS_TEXT, "CPS_FUNCTION(CPS_CALLER, CPS_CALLEE, __FILE__, __func__, %d);", argc);

				// printf("[%4d] %s\n", line - 1, args_str);
				// printf("->%s\n",CPS_TEXT);

				fprintf(fout, "\t%s\n", CPS_TEXT);
			}

			// printf("%c", c);
			c_prev = c;
		}

		line++;
		col = 0;
		if (comment == 2)
			comment = 0;
	}

	// for call stack end
	// fputs("#pragma GCC pop_options  //CPS_FUNCTION Restore saved options\n", fout);

	fclose(fin);
	fclose(fout);

	replacefile(".cps-temp.c", filename);
	
	// success insert
	return 0;
}

// remove trace code
int cps_remove(char *filename)
{
	FILE *fin = fopen(filename, "r");
	FILE *fout = fopen(".cps-temp.c", "w");
	char read_str[MAX_READ_BUFF];

	if (fin == NULL || fout == NULL)
	{
		fprintf(stderr, "can't open the %s\n", filename);
		return 1;
	}
	
	while (fgets(read_str, MAX_READ_BUFF, fin) != NULL)
	{
		if( strstr(read_str, "CPS_FUNCTION") == NULL )
			fputs(read_str, fout);
	}

	fclose(fin);
	fclose(fout);

	replacefile(".cps-temp.c", filename);
	return 0;
}

// args -> CPS(~~)
int parse_args(char *args_str, char *parsered_str)
{
	char copy_args[MAX_ARGS_BUFF] = "";
	int index = 0;
	int len = 0;
	int argc = 0;

	// remove '(' and ')'
	strncpy(copy_args, args_str + 1, strlen(args_str) - 2);
	char *args = strtok(copy_args, ",");
	while (args != NULL)
	{
		// trim
		while (args[0] == ' ' || args[0] == '\t' || args[0] == '\n')
			args++;
		if(strstr(args, "struct ") != NULL && strstr(args, "struct ") == args )
			args+=7;

		if (strcmp(args, "void") == 0)
		{
			args = strtok(NULL, ",");
			continue;
		}

		int args_len = strlen(args);

		// find sep pos
		int sep_i = args_len;
		while (sep_i > 0 && (args[--sep_i] != ' ' && args[sep_i] != '*'))
			;

		// function pointer check
		int brk_i = 0;
		for (brk_i = 0 ; brk_i < args_len ; brk_i++)
		{
			if(args[brk_i] == '(')
			{
				// function pointer!!!!
				sep_i = brk_i -1;
				break;
			}
		}
		// printf("[%s]\n", args);

		// type name
		parsered_str[index++] = '"';
		for (int i = 0; i < sep_i; i++)
			set_valid_char(parsered_str, args[i], &index, "");
		if( args[sep_i] == '*')
			parsered_str[index++] = '*';
		parsered_str[index++] = '"';

		// param name
		parsered_str[index++] = ',';
		parsered_str[index++] = ' ';
		parsered_str[index++] = '"';
		for (int i = sep_i + 1; i < args_len; i++)
			set_valid_char(parsered_str, args[i], &index, "");
		parsered_str[index++] = '"';

		// param value
		parsered_str[index++] = ',';
		parsered_str[index++] = ' ';
		if(brk_i != args_len)
		{
			int openbrk_i;
			int closebrk_i;

			for( openbrk_i = 0 ; openbrk_i < args_len ; openbrk_i++ )
				if( args[openbrk_i] == '(')
					break;
			for( closebrk_i = openbrk_i+1 ; closebrk_i < args_len ; closebrk_i++ )
				if( args[closebrk_i] == ')')
					break;
			for (int i = openbrk_i+1; i <= closebrk_i-1; i++)
				set_valid_char(parsered_str, args[i], &index, "*&");
		}
		else
		{
			for (int i = sep_i + 1; i < args_len; i++)
				set_valid_char(parsered_str, args[i], &index, "*&");
		}

		args = strtok(NULL, ",");
		if (args != NULL)
		{
			parsered_str[index++] = ',';
			parsered_str[index++] = ' ';
		}
		argc++;
	}

	return argc;
}

// only vaild char
void set_valid_char(char *dest, char c, int *index, const char *invalid)
{
	int invalidlen = strlen(invalid);
	for (int i = 0; i < invalidlen; i++)
		if (c == invalid[i])
			return;
	if (c < 32)
		return;

	dest[(*index)++] = c;
}

void replacefile(char * tempname, char * origin)
{
	char bakname[256] = ".bak-";
	strcat(bakname,origin);

	// if you want back up
	// rename(origin, bakname);
	rename(tempname, origin);
}

// have pblk.h?
int have_header(char *filename)
{
	char str[256];
	FILE *file = fopen(filename, "r+");
	while (fgets(str, 256, file) != NULL)
	{
		if (strstr(str, "#include") != NULL && strstr(str, "cps-tracer.h") != NULL)
			return TRUE;
		if (strstr(str, "#include") != NULL && strstr(str, "pblk.h") != NULL)
			return TRUE;
	}
	fclose(file);
	return FALSE;
}

// add file to file list(Add only files that meet the conditions)
int add_filelist(char filename[MAX_FILE_NAME], int *index, char *name)
{
	//find this file name without directory
	int filestart_i = strlen(__FILE__);
	while (filestart_i > 0 && __FILE__[filestart_i-1] != '/')
		filestart_i--;

	// if (strstr(&(__FILE__[filestart_i]), name) != NULL)
	// 	return 0;

	if (strstr(name, &(__FILE__[filestart_i])) != NULL)
		return 0;

	if (name[strlen(name) - 2] != '.' || name[strlen(name) - 1] != 'c')
		return 0;

	strcpy(filename, name);
	(*index)++;
}