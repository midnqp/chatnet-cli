/* 
 * File: lib-cpython-builtins.cpp
 * Repository: http://github.com/midnqp/lib-cpython-builtins
 * This file is modified according to need for TerminalChat.
 * Contains functions to operate on datatypes like Python.
 *
 * Copyright (C) 2021 Muhammad Bin Zafar <midnightquantumprogrammer@gmail.com>
 * Licensed under the MIT License: https://opensource.org/licenses/mit-license.php
 *
 */



#pragma once

// Standard libs have proper include-guards inside them.
#include <stdlib.h> 		// malloc, realloc, exit...
#include <string.h> 		// strcpy...
#include <stdio.h>  		// printf, FILE, perror...
#include <stdarg.h>  		// va_list...
#include <sys/stat.h>  	    // mkdir
#include <ctype.h>          // isalpha()
#ifdef _WIN32
	#include <Windows.h>        // Everything...
	#include "dirent.h"         // dirent.h for Windows
#else
	#include <unistd.h>         // sleep
	#include <dirent.h>         // dir_exists...
#endif




#define TYPE_CHAR    2
#define TYPE_STR     3
#define TYPE_INT     1
#define TYPE_LONG    4
#define TYPE_DOUBLE  5
#define TYPE_FLOAT   6
#define TYPE_UNKNOWN 0


#define type(variable) _Generic(variable, \
	int: TYPE_INT, \
	char: TYPE_CHAR, \
	char*: TYPE_STR, \
	long: TYPE_LONG, \
	double: TYPE_DOUBLE, \
	float: TYPE_FLOAT,\
	default: TYPE_UNKNOWN)


#define new(_type_, _bytes_) (_type_)malloc(_bytes_ + 1)


#define true 1
#define false 0


#define _libcpyb_rterr "lib-cpython-builtins RuntimeError:"



char* file_read(const char* filename) {
	FILE* file = fopen(filename, "r");
	if (file != NULL) {
		fseek(file, 0, SEEK_END);
		long length = ftell(file);
		rewind(file);
		char* buffer = new(char*, length);
		strcpy(buffer, "");
		fseek(file, 0, SEEK_SET);
		if (buffer) { fread(buffer, 1, length, file); }
		buffer[length] = '\0'; //fread doesn't automatically set that
		fclose(file);

		return buffer;
	}
	else { 
		printf(_libcpyb_rterr " Couldn't read file: %s\n", filename); 
		exit(1); 
	}
}




long file_size(const char* filename) {
	FILE* file = fopen(filename, "r");
	if (file) {
		fseek(file, 0, SEEK_END);
		long size = ftell(file);
		fclose(file);
		return size;
	}
	return -1;
}




void file_write(const char* filename, const char* buffer) {
	FILE* f = fopen(filename, "w");
	if (f == NULL) { printf("Couldn't open file: %s\n", filename); exit(1); }
	fprintf(f, "%s", buffer);
	fclose(f);
}




void file_append(const char* filename, const char* buffer) {
	FILE* f = fopen(filename, "a");
	if (f == NULL) { printf("Couldn't open file: %s\n", filename); exit(1); }
	fprintf(f, "%s", buffer);
	fclose(f);
}




int file_exists(const char* fn) {
	FILE* file = fopen(fn, "r");
	if (file) {
		fclose(file);
		return 1;
	}
	else {
		return 0;
	}
}




int file_remove(const char* fn) {
	return remove(fn);
}




int dir_exists(const char* dirName) {
	DIR* dir = opendir(dirName);
	if (dir) {
		closedir(dir);
		return 1;
	}
	else {
		closedir(dir);
		return 0;
	}
}






char* input(const char* any) {
	char* tmp = new(char*, 10000); //10 KB
	strcpy(tmp, "");
	printf("%s", any); 
	fgets(tmp, 10000, stdin);
	
	size_t len = strlen(tmp);
	tmp = (char*)realloc(tmp, len);  // if allocation > input, resizing and saving space!
	tmp[len - 1] = '\0';  //fgets adds a newline, while reading from stdin.
	return tmp;
}




// Only works in the same scope.
#define list_len(arr) (sizeof(arr)/sizeof(*arr))




double list_max(double numbers[], int arrlen) {
	double maxitem = numbers[0];
	for (int i = 1; i < arrlen; i++) {
		if (numbers[i] > maxitem) {
			maxitem = numbers[i];
		}
	}
	return maxitem;
}




double list_min(double numbers[], int arrlen) {
	double minitem = numbers[0];
	for (int i = 1; i < arrlen; i++) {
		if (numbers[i] < minitem) {
			minitem = numbers[i];
		}
	}
	return minitem;
}



// TODO
// double** list_sort(double** list);  qsort() from stdlib
// list_reverse(list)
// list_remove(list, x)


double list_sum(double numbers[], int arrlen) {
	double sum = 0;
	for (int i = 0; i < arrlen; i++) { sum += numbers[i]; }
	return sum;
}






// For unknown reason, inside str_addva(), although I'm doing `free(tmp)`, a memory leak error is shown by gcc: "Direct Memory leak"
const char* __asan_default_options() { return "detect_leaks=0"; }





struct string {
	char* str;
	size_t len;
};
void str_init(struct string* string) {
	string->len = 0;
	string->str = new(char*, 1);
	if (string->str == NULL) { printf("str_init() failed\n"); exit(1); }
	strcpy(string->str, "");
}




// Returns a pointer with *dest and *src concatenated.
char* str_add(const char* dest, const char* src) {
	size_t totalLen = strlen(dest) + strlen(src);
	char* ret = (char*)malloc(totalLen);

	strcpy(ret, dest);
	strcat(ret, src);

	return ret;
}




char* _str_addva(const char* strings, ...) {
	// Using va_args, adds multiple strings (that are passed as arguments) together.
	// And, then returns.
	va_list allstrings;
	va_start(allstrings, strings);

	char* parent = (char*)malloc(9000);
	strcpy(parent, "");
	char* _tmp = (char*)malloc(5000);
	strcpy(_tmp, "");
	strcpy(_tmp, strings);
	strcat(parent, _tmp);
	free(_tmp);

	char* tmp = va_arg(allstrings, char*);

	while (tmp != NULL) {
		strcat(parent, tmp);
		tmp = va_arg(allstrings, char*);
	}
	va_end(allstrings);
	//free(tmp);
	return parent;
}
#define str_addva(...) _str_addva(__VA_ARGS__, NULL)




int str_eq(const char* a, const char* b) {
	if (strcmp(a, b) == 0) return 1;
	else return 0;
}




char* str_reverse(const char* string) {
	int len = strlen(string);
	char* tmp = new(char*, len);
	strcpy(tmp, "");
	for (int i = len; i >= 0; i--) {
		strncat(tmp, &string[i], 1);
	}
	return tmp;
}




// Basically checks if substr is in string.
// Returns up to what is matched.
char* str_substr(char* string, char* substr) {
	char* matched = (char*)malloc(strlen(substr));
	strcpy(matched, "");
	size_t i = 0;    // counter for substr
	size_t j = 0;    // counter for string

	for (j = 0; j < strlen(string); j++) {
		if (substr[i] == string[j]) {
			strncat(matched, &substr[i], 1);    //substr[i] matched!
			i++;
		}
		if (str_eq(matched, substr)) break;
	}
	return matched;
}




//char* str_nsubstr(char* string, char* substr, int start, int end) {}




int str_index(const char* str, const char* substr, int start, int end) {
	int size = strlen(str) + 1;
	char* string = new(char*, size);   // temporary
	strcpy(string, str + start);  //makes perfect sense

	//210720
	//char* p = new(char*, strlen(str) + 1);
	//strcpy(p, "");
	char* p = strstr(string, substr);
	if (!p) {
		free(string);
		return -1;
	}

	int loc = (p - string) + start;
	//if ((size_t)loc < strlen(str)) return loc;
	
		//free(p);
		free(string);
	
	if (loc < end) return loc;
	// the location is absolute in respect to the *str.
	// str_index("abcdefghij", h, 4) --> doesn't output 3, rather 7
	else return -1;
}





int str_count(const char* string, const char* substr, int start, int end) {
	int count = 0;
	int i = str_index(string, substr, start, end);
	while (i != -1) {
		count++;
		i = str_index(string, substr, i + 1, end);
	}
	return count;
}


char* str_slice(const char* string, int start, int step, int end);
int str_isalpha(const char* string) {
	int result = 1; //true
	for (size_t i = 0; i < strlen(string); i++) {
		char* tmpSlice = str_slice(string, i, 1, i + 1);
		char ch = tmpSlice[0];
		free(tmpSlice);
		if (!isalpha(ch)) { result = 0; break; }  //false
	}
	return result;
}


char* str_replace(const char* main, const char* repl, const char* with, int start, int end) {
	int len_main = strlen(main);
	int occ = str_count(main, repl, start, end);   // occurences

	size_t size = (len_main - occ * strlen(repl)) + occ * strlen(with);
	char* Result = new(char*, size);
	strcpy(Result, "");

	int whereRepl = str_index(main, repl, start, end);

	int i = start;

	while (main[i] != '\0') {
		if (i >= whereRepl && whereRepl != -1) {
			for (size_t j = 0; j < strlen(with); j++)
				strncat(Result, &with[j], 1);

			whereRepl = str_index(main, repl, i + strlen(repl), end);
			i = i + strlen(repl);  //else block: main[i]
		}
		else {
			strncat(Result, &main[i], 1);
			i++;
		}
	}
	return Result;
}




char* str_slice(const char* string, int start, int step, int end) {
	char* tmp = new(char*, strlen(string));
	strcpy(tmp, "");
	for (int i = start; i < end; i += step) {
		//tmp[j] = string[i];
		strncat(tmp, &string[i], 1);
		//printf("str_slice --- i: %d --- tmp: %s\n", i, tmp);
	}
	return tmp;
}




char** str_split(char* a_str, const char a_delim) {
	char** result = 0;
	size_t count = 0;
	char* tmp = a_str;
	char* last_comma = 0;
	char delim[2];
	delim[0] = a_delim;
	delim[1] = 0;

	/* Count how many elements will be extracted. */
	while (*tmp)
	{
		if (a_delim == *tmp)
		{
			count++;
			last_comma = tmp;
		}
		tmp++;
	}

	/* Add space for trailing token. */
	count += last_comma < (a_str + strlen(a_str) - 1);

	/* Add space for terminating null string so caller
	   knows where the list of returned strings ends. */
	count++;

	result = (char**)malloc(sizeof(char*) * count);

	if (result)
	{
		size_t idx = 0;
		char* token = strtok(a_str, delim);

		while (token)
		{
			*(result + idx++) = strdup(token);
			token = strtok(0, delim);
		}
		*(result + idx) = 0;
	}

	return result;
}




// The following things will be turned into just macros, in future version of lib-cpython-builtins.
#define _R0  "\033[0;0m"
#define _BLK "\033[30m"
#define _RED "\033[31m"
#define _GRN "\033[32m"
#define _YLW "\033[33m"
#define _BLU "\033[34m"
#define _MGN "\033[35m"
#define _CYN "\033[36m"
#define _GRY "\033[37m"

#define _BLKBG "\033[40m"
#define _REDBG "\033[41m"
#define _GRNBG "\033[42m"
#define _YLWBL "\033[43m"
#define _BLUBG "\033[44m"
#define _MGNBG "\033[45m"
#define _CYNBG "\033[46m"
#define _GRYBG "\033[47m"

#define TBSP "    "

const char* R0 = _R0;
const char* BLK = _BLK;
const char* RED = _RED;
const char* GRN = _GRN;
const char* YLW = _YLW;
const char* BLU = _BLU;
const char* MGN = _MGN;
const char* CYN = _CYN;
const char* GRY = _GRY;

const char* REDBG = _REDBG;
const char* GRNBG = _GRNBG;
const char* BLUBG = _BLUBG;
const char* MGNBG = _MGNBG;
const char* GRYBG = _GRYBG;
