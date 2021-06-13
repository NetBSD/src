%{
/*	$NetBSD: testlang_parse.y,v 1.53 2021/06/13 11:06:20 rillig Exp $	*/

/*-
 * Copyright 2009 Brett Lymn <blymn@NetBSD.org>
 * Copyright 2021 Roland Illig <rillig@NetBSD.org>
 *
 * All rights reserved.
 *
 * This code has been donated to The NetBSD Foundation by the Author.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <curses.h>
#include <errno.h>
#include <fcntl.h>
#include <err.h>
#include <unistd.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <vis.h>
#include <stdint.h>
#include "returns.h"
#include "director.h"

#define YYDEBUG 1

extern int verbose;
extern int check_file_flag;
extern int master;
extern struct pollfd readfd;
extern char *check_path;
extern char *cur_file;		/* from director.c */

int yylex(void);

size_t line = 1;

static int input_delay;

/* time delay between inputs chars - default to 0.1ms minimum to prevent
 * problems with input tests
 */
#define DELAY_MIN 0.1

/* time delay after a function call - allows the slave time to
 * run the function and output data before we do other actions.
 * Set this to 50ms.
 */
#define POST_CALL_DELAY 50

static struct timespec delay_spec = {0, 1000 * DELAY_MIN};
static struct timespec delay_post_call = {0, 1000 * POST_CALL_DELAY};

static char *input_str;	/* string to feed in as input */
static bool no_input;	/* don't need more input */

static wchar_t *vals = NULL;	/* wchars to attach to a cchar type */
static unsigned nvals;		/* number of wchars */

const char *const enum_names[] = {	/* for data_enum_t */
	"unused", "numeric", "static", "string", "byte", "cchar", "wchar", "ERR",
	"OK", "NULL", "not NULL", "variable", "reference", "return count",
	"slave error"
};

typedef struct {
	data_enum_t	arg_type;
	size_t		arg_len;
	char		*arg_string;
	int		var_index;
} args_t;

typedef struct {
	char		*function;
	int		nrets;		/* number of returns */
	ct_data_t	*returns;	/* array of expected returns */
	int		nargs;		/* number of arguments */
	args_t		*args;		/* arguments for the call */
} cmd_line_t;

static cmd_line_t	command;

typedef struct {
	char *name;
	size_t len;
	data_enum_t type;
	void *value;
	cchar_t cchar;
} var_t;

static size_t nvars; 		/* Number of declared variables */
static var_t *vars; 		/* Variables defined during the test. */

static int	check_function_table(char *, const char *const[], int);
static int	find_var_index(const char *);
static void 	assign_arg(data_enum_t, void *);
static int	assign_var(const char *);
void		init_parse_variables(int);
static void	validate(int, void *);
static void	validate_return(const char *, const char *, int);
static void	validate_variable(int, data_enum_t, const void *, int, int);
static void	validate_byte(ct_data_t *, ct_data_t *, int);
static void	validate_cchar(cchar_t *, cchar_t *, int);
static void	validate_wchar(wchar_t *, wchar_t *, int);
static void	write_cmd_pipe(char *);
static void	write_cmd_pipe_args(data_enum_t, void *);
static void	read_cmd_pipe(ct_data_t *);
static void	write_func_and_args(void);
static void	compare_streams(const char *, bool);
static void	do_function_call(size_t);
static void	check(void);
static void	delay_millis(const char *);
static void	do_input(const char *);
static void	do_noinput(void);
static void	save_slave_output(bool);
static void	validate_type(data_enum_t, ct_data_t *, int);
static void	set_var(data_enum_t, const char *, void *);
static void	validate_reference(int, void *);
static char *	numeric_or(char *, char *);
static char *	get_numeric_var(const char *);
static void	perform_delay(struct timespec *);
static void	set_cchar(char *, void *);
static void	set_wchar(char *);
static wchar_t *add_to_vals(data_enum_t, void *);

#define variants(fn) "" fn, "mv" fn, "w" fn, "mvw" fn
static const char *const input_functions[] = {
	variants("getch"),
	variants("getnstr"),
	variants("getstr"),
	variants("getn_wstr"),
	variants("get_wch"),
	variants("get_wstr"),
	variants("scanw"),
};
#undef variants

static const unsigned ninput_functions =
	sizeof(input_functions) / sizeof(input_functions[0]);

extern saved_data_t saved_output;

%}

%union {
	char *string;
	ct_data_t *retval;
	wchar_t	*vals;
}

%token <string> PATH
%token <string> STRING
%token <retval> BYTE
%token <string> VARNAME
%token <string> FILENAME
%token <string> VARIABLE
%token <string> REFERENCE
%token <string> NULL_RET
%token <string> NON_NULL
%token <string> ERR_RET
%token <string> OK_RET
%token <string> numeric
%token <string> DELAY
%token <string> INPUT
%token <string> COMPARE
%token <string> COMPAREND
%token <string> ASSIGN
%token <string> CCHAR
%token <string> WCHAR
%token EOL CALL CHECK NOINPUT OR MULTIPLIER LPAREN RPAREN LBRACK RBRACK
%token COMMA
%token CALL2 CALL3 CALL4

%type <string> attributes expr
%type <vals> array_elements array_element

%nonassoc OR

%%

statements	: /* empty */
		| statement EOL statements
		;

statement	: assign
		| call
		| call2
		| call3
		| call4
		| check
		| delay
		| input
		| noinput
		| compare
		| comparend
		| cchar
		| wchar
		| /* empty */
		;

assign		: ASSIGN VARNAME numeric {
			set_var(data_number, $2, $3);
		}
		| ASSIGN VARNAME LPAREN expr RPAREN {
			set_var(data_number, $2, $4);
		}
		| ASSIGN VARNAME STRING {
			set_var(data_string, $2, $3);
		}
		| ASSIGN VARNAME BYTE {
			set_var(data_byte, $2, $3);
		}
		;

cchar		: CCHAR VARNAME attributes char_vals {
			set_cchar($2, $3);
		}
		;

wchar		: WCHAR VARNAME char_vals {
			set_wchar($2);
		}
		;

attributes	: numeric
		| LPAREN expr RPAREN {
			$$ = $2;
		}
		| VARIABLE {
			$$ = get_numeric_var($1);
		}
		;

char_vals	: numeric {
			add_to_vals(data_number, $1);
		}
		| LBRACK array_elements RBRACK
		| VARIABLE {
			add_to_vals(data_var, $1);
		}
		| STRING {
			add_to_vals(data_string, $1);
		}
		| BYTE {
			add_to_vals(data_byte, $1);
		}
		;

call		: CALL result fn_name args {
			do_function_call(1);
		}
		;

call2		: CALL2 result result fn_name args {
			do_function_call(2);
		}
		;

call3		: CALL3 result result result fn_name args {
			do_function_call(3);
		}
		;

call4		: CALL4 result result result result fn_name args {
			do_function_call(4);
		}
		;

check		: CHECK var returns {
			check();
		}
		;

delay		: DELAY numeric {
			delay_millis($2);
		}
		;

input		: INPUT STRING {
			do_input($2);
		}
		;

noinput		: NOINPUT {
			do_noinput();
		}
		;

compare		: COMPARE PATH {
			compare_streams($2, true);
		}
		| COMPARE FILENAME {
			compare_streams($2, true);
		}
		;

comparend	: COMPAREND PATH {
			compare_streams($2, false);
		}
		| COMPAREND FILENAME {
			compare_streams($2, false);
		}
		;


result		: returns
		| reference
		;

returns		: numeric {
			assign_rets(data_number, $1);
		}
		| LPAREN expr RPAREN {
			assign_rets(data_number, $2);
		}
		| STRING {
			assign_rets(data_string, $1);
		}
		| BYTE {
			assign_rets(data_byte, (void *) $1);
		}
		| ERR_RET {
			assign_rets(data_err, NULL);
		}
		| OK_RET {
			assign_rets(data_ok, NULL);
		}
		| NULL_RET {
			assign_rets(data_null, NULL);
		}
		| NON_NULL {
			assign_rets(data_nonnull, NULL);
		}
		| var
		;

var		: VARNAME {
			assign_rets(data_var, $1);
		}
		;

reference	: VARIABLE {
			assign_rets(data_ref, $1);
		}
		;

fn_name		: VARNAME {
			if (command.function != NULL)
				free(command.function);

			command.function = malloc(strlen($1) + 1);
			if (command.function == NULL)
				err(1, "Could not allocate memory for function name");
			strcpy(command.function, $1);
		}
		;

array_elements	: array_element
		| array_element COMMA array_elements
		;

array_element	: numeric {
			$$ = add_to_vals(data_number, $1);
		}
		| VARIABLE {
			$$ = add_to_vals(data_number, get_numeric_var($1));
		}
		| BYTE {
			$$ = add_to_vals(data_byte, (void *) $1);
		}
		| STRING {
			$$ = add_to_vals(data_string, (void *) $1);
		}
		| numeric MULTIPLIER numeric {
			unsigned long i;
			unsigned long acount;

			acount = strtoul($3, NULL, 10);
			for (i = 0; i < acount; i++) {
				$$ = add_to_vals(data_number, $1);
			}
		}
		| VARIABLE MULTIPLIER numeric {
			unsigned long i, acount;
			char *val;

			acount = strtoul($3, NULL, 10);
			val = get_numeric_var($1);
			for (i = 0; i < acount; i++) {
				$$ = add_to_vals(data_number, val);
			}
		}
		| BYTE MULTIPLIER numeric {
			unsigned long i, acount;

			acount = strtoul($3, NULL, 10);
			for (i = 0; i < acount; i++) {
				$$ = add_to_vals(data_byte, (void *) $1);
			}
		}
		| STRING MULTIPLIER numeric {
			unsigned long i, acount;

			acount = strtoul($3, NULL, 10);
			for (i = 0; i < acount; i++) {
				$$ = add_to_vals(data_string, (void *) $1);
			}
		}
		;

expr		: numeric
		| VARIABLE {
			$$ = get_numeric_var($1);
		}
		| expr OR expr {
			$$ = numeric_or($1, $3);
		}
		;

args		: /* empty */
		| arg args
		;

arg		: LPAREN expr RPAREN {
			assign_arg(data_static, $2);
		}
		| numeric {
			assign_arg(data_static, $1);
		}
		| STRING {
			assign_arg(data_static, $1);
		}
		| BYTE {
			assign_arg(data_byte, $1);
		}
		| PATH {
			assign_arg(data_static, $1);
		}
		| FILENAME {
			assign_arg(data_static, $1);
		}
		| VARNAME {
			assign_arg(data_static, $1);
		}
		| VARIABLE {
			assign_arg(data_var, $1);
		}
		| NULL_RET {
			assign_arg(data_null, $1);
		}
		;

%%

static void
excess(const char *fname, size_t lineno, const char *func, const char *comment,
    const void *data, size_t datalen)
{
	size_t dstlen = datalen * 4 + 1;
	char *dst = malloc(dstlen);

	if (dst == NULL)
		err(1, "malloc");

	if (strnvisx(dst, dstlen, data, datalen, VIS_WHITE | VIS_OCTAL) == -1)
		err(1, "strnvisx");

	warnx("%s:%zu: [%s] Excess %zu bytes%s [%s]",
	    fname, lineno, func, datalen, comment, dst);
	free(dst);
}

/*
 * Get the value of a variable, error if the variable has not been set or
 * is not a numeric type.
 */
static char *
get_numeric_var(const char *var)
{
	int i;

	if ((i = find_var_index(var)) < 0)
		errx(1, "Variable %s is undefined", var);

	if (vars[i].type != data_number)
		errx(1, "Variable %s is not a numeric type", var);

	return vars[i].value;
}

/*
 * Perform a bitwise OR on two numbers and return the result.
 */
static char *
numeric_or(char *n1, char *n2)
{
	unsigned long i1, i2, result;
	char *ret;

	i1 = strtoul(n1, NULL, 10);
	i2 = strtoul(n2, NULL, 10);

	result = i1 | i2;
	asprintf(&ret, "%lu", result);

	if (verbose) {
		fprintf(stderr, "numeric or of 0x%lx (%s) and 0x%lx (%s)"
		    " results in 0x%lx (%s)\n",
		    i1, n1, i2, n2, result, ret);
	}

	return ret;
}

/*
 * Sleep for the specified time, handle the sleep getting interrupted
 * by a signal.
 */
static void
perform_delay(struct timespec *ts)
{
	struct timespec delay_copy, delay_remainder;

	delay_copy = *ts;
	while (nanosleep(&delay_copy, &delay_remainder) < 0) {
		if (errno != EINTR)
			err(2, "nanosleep returned error");
		delay_copy = delay_remainder;
	}
}

/*
 * Add to temporary vals array
 */
static wchar_t *
add_to_vals(data_enum_t argtype, void *arg)
{
	wchar_t *retval = NULL;
	int have_malloced;
	int i;
	ct_data_t *ret;

	have_malloced = 0;

	if (nvals == 0) {
		have_malloced = 1;
		retval = malloc(sizeof(wchar_t));
	} else {
		retval = realloc(vals, (nvals + 1) * sizeof(wchar_t));
	}

	if (retval == NULL)
		return retval;

	vals = retval;

	switch (argtype) {
	case data_number:
		vals[nvals++] = (wchar_t) strtoul((char *) arg, NULL, 10);
		break;

	case data_string:
		vals[nvals++] = (wchar_t) ((char *)arg)[0];
		break;

	case data_byte:
		ret = (ct_data_t *) arg;
		vals[nvals++] = *((wchar_t *) ret->data_value);
		break;

	case data_var:
		if ((i = find_var_index((char *) arg)) < 0)
			errx(1, "%s:%zu: Variable %s is undefined",
			    cur_file, line, (const char *) arg);

		switch (vars[i].type) {

		case data_number:
		case data_string:
		case data_byte:
			retval = add_to_vals(vars[i].type, vars[i].value);
			break;

		default:
			errx(1,
			    "%s:%zu: Variable %s has invalid type for cchar",
			    cur_file, line, (const char *) arg);
			break;

		}
		break;

	default:
		errx(1, "%s:%zu: Internal error: Unhandled type for vals array",
		    cur_file, line);

		/* if we get here without a value then tidy up */
		if ((nvals == 0) && (have_malloced == 1)) {
			free(retval);
			retval = vals;
		}
		break;

	}

	return retval;
}

/*
 * Assign the value given to the named variable.
 */
static void
set_var(data_enum_t type, const char *name, void *value)
{
	int i;
	char *number;
	ct_data_t *ret;

	i = find_var_index(name);
	if (i < 0)
		i = assign_var(name);

	vars[i].type = type;
	if ((type == data_number) || (type == data_string)) {
		number = value;
		vars[i].len = strlen(number) + 1;
		vars[i].value = malloc(vars[i].len + 1);
		if (vars[i].value == NULL)
			err(1, "Could not malloc memory for assign string");
		strcpy(vars[i].value, number);
	} else {
		/* can only be a byte value */
		ret = value;
		vars[i].len = ret->data_len;
		vars[i].value = malloc(vars[i].len);
		if (vars[i].value == NULL)
			err(1, "Could not malloc memory to assign byte string");
		memcpy(vars[i].value, ret->data_value, vars[i].len);
	}
}

/*
 * Form up a complex character type from the given components.
 */
static void
set_cchar(char *name, void *attributes)
{
	int i;
	unsigned j;
	attr_t attribs;

	if (nvals >= CURSES_CCHAR_MAX)
		errx(1, "%s:%zu: %s: too many characters in complex char type",
		    cur_file, line, __func__);

	i = find_var_index(name);
	if (i < 0)
		i = assign_var(name);

	if (sscanf((char *) attributes, "%d", &attribs) != 1)
		errx(1,
		    "%s:%zu: %s: conversion of attributes to integer failed",
		    cur_file, line, __func__);

	vars[i].type = data_cchar;
	vars[i].cchar.attributes = attribs;
	vars[i].cchar.elements = nvals;
	for (j = 0; j < nvals; j++)
		vars[i].cchar.vals[j] = vals[j];

	nvals = 0;
	vals = NULL;

}

/*
 * Form up a wide character string type from the given components.
 */
static void
set_wchar(char *name)
{
	int i;
	unsigned j;
	wchar_t *wcval;

	i = find_var_index(name);
	if (i < 0)
		i = assign_var(name);

	vars[i].type = data_wchar;
	vars[i].len = (nvals+1) * sizeof(wchar_t);
	vars[i].value = malloc(vars[i].len);
	if (vars[i].value == NULL)
		err(1, "Could not malloc memory to assign wchar string");
	wcval = vars[i].value;
	for(j = 0; j < nvals; j++)
		wcval[j] = vals[j];
	wcval[nvals] = L'\0';
	nvals = 0;
	vals = NULL;

}

/*
 * Add a new variable to the vars array, the value will be assigned later,
 * when a test function call returns.
 */
static int
assign_var(const char *varname)
{
	var_t *temp;
	char *name;

	if ((name = malloc(strlen(varname) + 1)) == NULL)
		err(1, "Alloc of varname failed");

	if ((temp = realloc(vars, sizeof(*temp) * (nvars + 1))) == NULL) {
		free(name);
		err(1, "Realloc of vars array failed");
	}

	strcpy(name, varname);
	vars = temp;
	vars[nvars].name = name;
	vars[nvars].len = 0;
	vars[nvars].value = NULL;
	nvars++;

	return (nvars - 1);
}

/*
 * Allocate and assign a new argument of the given type.
 */
static void
assign_arg(data_enum_t arg_type, void *arg)
{
	args_t *temp, cur;
	char *str = arg;
	ct_data_t *ret;

	if (verbose) {
		fprintf(stderr, "function is >%s<, adding arg >%s< type %s (%d)\n",
		       command.function, str, enum_names[arg_type], arg_type);
	}

	cur.arg_type = arg_type;
	if (cur.arg_type == data_var) {
		cur.var_index = find_var_index(arg);
		if (cur.var_index < 0)
			errx(1, "%s:%zu: Invalid variable %s",
			    cur_file, line, str);
	} else if (cur.arg_type == data_byte) {
		ret = arg;
		cur.arg_len = ret->data_len;
		cur.arg_string = malloc(cur.arg_len);
		if (cur.arg_string == NULL)
			err(1, "Could not malloc memory for arg bytes");
		memcpy(cur.arg_string, ret->data_value, cur.arg_len);
	} else if (cur.arg_type == data_null) {
		cur.arg_len = 0;
		cur.arg_string = NULL;
	} else {
		cur.arg_len = strlen(str);
		cur.arg_string = malloc(cur.arg_len + 1);
		if (cur.arg_string == NULL)
			err(1, "Could not malloc memory for arg string");
		strcpy(cur.arg_string, arg);
	}

	temp = realloc(command.args, sizeof(*temp) * (command.nargs + 1));
	if (temp == NULL)
		err(1, "Failed to reallocate args");
	command.args = temp;
	memcpy(&command.args[command.nargs], &cur, sizeof(args_t));
	command.nargs++;
}

/*
 * Allocate and assign a new return.
 */
static void
assign_rets(data_enum_t ret_type, void *ret)
{
	ct_data_t *temp, cur;
	char *ret_str;
	ct_data_t *ret_ret;

	cur.data_type = ret_type;
	if (ret_type != data_var) {
		if ((ret_type == data_number) || (ret_type == data_string)) {
			ret_str = ret;
			cur.data_len = strlen(ret_str) + 1;
			cur.data_value = malloc(cur.data_len + 1);
			if (cur.data_value == NULL)
				err(1,
				    "Could not malloc memory for arg string");
			strcpy(cur.data_value, ret_str);
		} else if (ret_type == data_byte) {
			ret_ret = ret;
			cur.data_len = ret_ret->data_len;
			cur.data_value = malloc(cur.data_len);
			if (cur.data_value == NULL)
				err(1,
				    "Could not malloc memory for byte string");
			memcpy(cur.data_value, ret_ret->data_value,
			       cur.data_len);
		} else if (ret_type == data_ref) {
			if ((cur.data_index = find_var_index(ret)) < 0)
				errx(1, "Undefined variable reference");
		}
	} else {
		cur.data_index = find_var_index(ret);
		if (cur.data_index < 0)
			cur.data_index = assign_var(ret);
	}

	temp = realloc(command.returns, sizeof(*temp) * (command.nrets + 1));
	if (temp == NULL)
		err(1, "Failed to reallocate returns");
	command.returns = temp;
	memcpy(&command.returns[command.nrets], &cur, sizeof(ct_data_t));
	command.nrets++;
}

/*
 * Find the given variable name in the var array and return the i
 * return -1 if var is not found.
 */
static int
find_var_index(const char *var_name)
{
	int result;
	size_t i;

	result = -1;

	for (i = 0; i < nvars; i++) {
		if (strcmp(var_name, vars[i].name) == 0) {
			result = i;
			break;
		}
	}

	return result;
}

/*
 * Check the given function name in the given table of names, return 1 if
 * there is a match.
 */
static int
check_function_table(char *function, const char *const table[], int nfunctions)
{
	int i;

	for (i = 0; i < nfunctions; i++) {
		if (strcmp(function, table[i]) == 0)
			return 1;
	}

	return 0;
}

/*
 * Compare the output from the slave against the given file and report
 * any differences.
 */
static void
compare_streams(const char *filename, bool discard)
{
	char check_file[PATH_MAX], drain[100], ref, data;
	struct pollfd fds[2];
	int nfd, check_fd;
	ssize_t result;
	size_t offs;

	/*
	 * Don't prepend check path iff check file has an absolute
	 * path.
	 */
	if (filename[0] != '/') {
		if (strlcpy(check_file, check_path, sizeof(check_file))
		    >= sizeof(check_file))
			errx(2, "CHECK_PATH too long");

		if (strlcat(check_file, "/", sizeof(check_file))
		    >= sizeof(check_file))
			errx(2, "Could not append / to check file path");
	} else {
		check_file[0] = '\0';
	}

	if (strlcat(check_file, filename, sizeof(check_file))
	    >= sizeof(check_file))
		errx(2, "Path to check file path overflowed");

	int create_check_file = 0;

	if (check_file_flag == (GEN_CHECK_FILE | FORCE_GEN))
		create_check_file = 1;
	else if ((check_fd = open(check_file, O_RDONLY, 0)) < 0) {
		if (check_file_flag & GEN_CHECK_FILE)
			create_check_file = 1;
		else
			err(2, "%s:%zu: failed to open file %s",
			    cur_file, line, check_file);
	}

	if (create_check_file) {
		check_fd = open(check_file, O_WRONLY | O_CREAT, 0644);
		if (check_fd < 0) {
			err(2, "%s:%zu: failed to create file %s",
			    cur_file, line, check_file);
		}
	}

	fds[0].fd = check_fd;
	fds[0].events = create_check_file ? POLLOUT:POLLIN;
	fds[1].fd = master;
	fds[1].events = POLLIN;

	nfd = 2;
	/*
	 * if we have saved output then only check for data in the
	 * reference file since the slave data may already be drained.
	 */
	if (saved_output.count > 0)
		nfd = 1;

	offs = 0;
	while (poll(fds, nfd, 500) == nfd) {
		/* Read from check file if doing comparison */
		if (!create_check_file) {
			if (fds[0].revents & POLLIN) {
				if ((result = read(check_fd, &ref, 1)) < 1) {
					if (result != 0) {
						err(2, "Bad read on file %s",
						    check_file);
					} else {
						break;
					}
				}
			}
		}

		if (saved_output.count > 0) {
			data = saved_output.data[saved_output.readp];
			saved_output.count--;
			saved_output.readp++;
			/* run out of saved data, switch to file */
			if (saved_output.count == 0)
				nfd = 2;
		} else {
			int revent = (create_check_file == 1) ? POLLOUT:POLLIN;
			if (fds[0].revents & revent) {
				if (read(master, &data, 1) < 1)
					err(2, "Bad read on slave pty");
			} else
				continue;
		}

		if (create_check_file) {
			if ((result = write(check_fd, &data, 1)) < 1)
				err(2, "Bad write on file %s", check_file);
			ref = data;
		}

		if (verbose) {
			if (create_check_file)
				fprintf(stderr, "Saving reference byte 0x%x (%c)"
					" against slave byte 0x%x (%c)\n",
					ref, (ref >= ' ') ? ref : '-',
					data, (data >= ' ' )? data : '-');
			else
				fprintf(stderr, "Comparing reference byte 0x%x (%c)"
					" against slave byte 0x%x (%c)\n",
					ref, (ref >= ' ') ? ref : '-',
					data, (data >= ' ' )? data : '-');
		}

		if (!create_check_file && ref != data) {
			errx(2, "%s:%zu: refresh data from slave does "
			    "not match expected from file %s offset %zu "
			    "[reference 0x%02x (%c) != slave 0x%02x (%c)]",
			    cur_file, line, check_file, offs,
			    ref, (ref >= ' ') ? ref : '-',
			    data, (data >= ' ') ? data : '-');
		}

		offs++;
	}

	/*
	 * if creating a check file, there shouldn't be
	 * anymore saved output
	 */
	if (saved_output.count > 0) {
		if (create_check_file)
			errx(2, "Slave output not flushed correctly");
		else
			excess(cur_file, line, __func__, " from slave",
				&saved_output.data[saved_output.readp], saved_output.count);
	}

	/* discard any excess saved output if required */
	if (discard) {
		saved_output.count = 0;
		saved_output.readp = 0;
	}

	if (!create_check_file && (result = poll(fds, 2, 0)) != 0) {
		if (result == -1)
			err(2, "poll of file descriptors failed");

		if ((fds[1].revents & POLLIN) == POLLIN) {
			save_slave_output(true);
		} else if ((fds[0].revents & POLLIN) == POLLIN) {
			/*
			 * handle excess in file if it exists.  Poll
			 * says there is data until EOF is read.
			 * Check next read is EOF, if it is not then
			 * the file really has more data than the
			 * slave produced so flag this as a warning.
			 */
			result = read(check_fd, drain, sizeof(drain));
			if (result == -1)
				err(1, "read of data file failed");

			if (result > 0) {
				excess(check_file, 0, __func__, "", drain,
				    result);
			}
		}
	}

	close(check_fd);
}

/*
 * Pass a function call and arguments to the slave and wait for the
 * results.  The variable nresults determines how many returns we expect
 * back from the slave.  These results will be validated against the
 * expected returns or assigned to variables.
 */
static void
do_function_call(size_t nresults)
{
#define MAX_RESULTS 4
	char *p;
	int do_input;
	size_t i;
	struct pollfd fds[3];
	ct_data_t response[MAX_RESULTS], returns_count;
	assert(nresults <= MAX_RESULTS);

	do_input = check_function_table(command.function, input_functions,
	    ninput_functions);

	write_func_and_args();

	/*
	 * We should get the number of returns back here, grab it before
	 * doing input otherwise it will confuse the input poll
	 */
	read_cmd_pipe(&returns_count);
	if (returns_count.data_type != data_count)
		errx(2, "expected return type of data_count but received %s",
		    enum_names[returns_count.data_type]);

	perform_delay(&delay_post_call); /* let slave catch up */

	if (verbose) {
		fprintf(stderr, "Expect %zu results from slave, slave "
		    "reported %zu\n", nresults, returns_count.data_len);
	}

	if ((no_input == false) && (do_input == 1)) {
		if (verbose) {
			fprintf(stderr, "doing input with inputstr >%s<\n",
			    input_str);
		}

		if (input_str == NULL)
			errx(2, "%s:%zu: Call to input function "
			    "but no input defined", cur_file, line);

		fds[0].fd = from_slave;
		fds[0].events = POLLIN;
		fds[1].fd = master;
		fds[1].events = POLLOUT;
 		p = input_str;
		save_slave_output(false);
		while (*p != '\0') {
			perform_delay(&delay_spec);

			if (poll(fds, 2, 0) < 0)
				err(2, "poll failed");
			if (fds[0].revents & POLLIN) {
				warnx("%s:%zu: Slave function "
				    "returned before end of input string",
				    cur_file, line);
				break;
			}
			if ((fds[1].revents & POLLOUT) == 0)
				continue;
			if (verbose) {
				fprintf(stderr, "Writing char >%c< to slave\n",
				    *p);
			}
			if (write(master, p, 1) != 1) {
				warn("%s:%zu: Slave function write error",
				    cur_file, line);
				break;
			}
			p++;

		}
		save_slave_output(false);

		if (verbose) {
			fprintf(stderr, "Input done.\n");
		}

		/* done with the input string, free the resources */
		free(input_str);
		input_str = NULL;
	}

	if (verbose) {
		fds[0].fd = to_slave;
		fds[0].events = POLLIN;

		fds[1].fd = from_slave;
		fds[1].events = POLLOUT;

		fds[2].fd = master;
		fds[2].events = POLLIN | POLLOUT;

		i = poll(&fds[0], 3, 1000);
		fprintf(stderr, "Poll returned %zu\n", i);
		for (i = 0; i < 3; i++) {
			fprintf(stderr, "revents for fd[%zu] = 0x%x\n",
				i, fds[i].revents);
		}
	}

	/* drain any trailing output */
	save_slave_output(false);

	for (i = 0; i < returns_count.data_len; i++) {
		read_cmd_pipe(&response[i]);
	}

	/*
	 * Check for a slave error in the first return slot, if the
	 * slave errored then we may not have the number of returns we
	 * expect but in this case we should report the slave error
	 * instead of a return count mismatch.
	 */
	if ((returns_count.data_len > 0) &&
	    (response[0].data_type == data_slave_error))
		errx(2, "Slave returned error: %s",
		    (const char *)response[0].data_value);

	if (returns_count.data_len != nresults)
		errx(2, "Incorrect number of returns from slave, expected %zu "
		    "but received %zu", nresults, returns_count.data_len);

	if (verbose) {
		for (i = 0; i < nresults; i++) {
			if ((response[i].data_type != data_byte) &&
			    (response[i].data_type != data_err) &&
			    (response[i].data_type != data_ok))
				fprintf(stderr,
					"received response >%s< "
					"expected",
					(const char *)response[i].data_value);
			else
				fprintf(stderr, "received");

			fprintf(stderr, " data_type %s\n",
			    enum_names[command.returns[i].data_type]);
		}
	}

	for (i = 0; i < nresults; i++) {
		if (command.returns[i].data_type != data_var) {
			validate(i, &response[i]);
		} else {
			vars[command.returns[i].data_index].len =
				response[i].data_len;

			if (response[i].data_type == data_cchar) {
				vars[command.returns[i].data_index].cchar =
					*((cchar_t *)response[i].data_value);
		} else {
				vars[command.returns[i].data_index].value =
					response[i].data_value;
			}

			vars[command.returns[i].data_index].type =
				response[i].data_type;
		}
	}

	if (verbose && (saved_output.count > 0))
		excess(cur_file, line, __func__, " from slave",
		    &saved_output.data[saved_output.readp], saved_output.count);

	init_parse_variables(0);
}

/*
 * Write the function and command arguments to the command pipe.
 */
static void
write_func_and_args(void)
{
	int i;

	if (verbose) {
		fprintf(stderr, "calling function >%s<\n", command.function);
	}

	write_cmd_pipe(command.function);
	for (i = 0; i < command.nargs; i++) {
		if (command.args[i].arg_type == data_var)
			write_cmd_pipe_args(command.args[i].arg_type,
					    &vars[command.args[i].var_index]);
		else
			write_cmd_pipe_args(command.args[i].arg_type,
					    &command.args[i]);
	}

	write_cmd_pipe(NULL); /* signal end of arguments */
}

static void
check(void)
{
	ct_data_t retvar;
	var_t *vptr;

	if (command.returns[0].data_index == -1)
		errx(1, "%s:%zu: Undefined variable in check statement",
		    cur_file, line);

	if (command.returns[1].data_type == data_var) {
		vptr = &vars[command.returns[1].data_index];
		command.returns[1].data_type = vptr->type;
		command.returns[1].data_len = vptr->len;
		if (vptr->type != data_cchar)
			command.returns[1].data_value = vptr->value;
		else
			command.returns[1].data_value = &vptr->cchar;
	}

	if (verbose) {
		fprintf(stderr, "Checking contents of variable %s for %s\n",
		    vars[command.returns[0].data_index].name,
		    enum_names[command.returns[1].data_type]);
	}

	/*
	 * Check if var and return have same data types
	 */
	if (((command.returns[1].data_type == data_byte) &&
	     (vars[command.returns[0].data_index].type != data_byte)))
		errx(1, "Var type %s (%d) does not match return type %s (%d)",
		    enum_names[vars[command.returns[0].data_index].type],
		    vars[command.returns[0].data_index].type,
		    enum_names[command.returns[1].data_type],
		    command.returns[1].data_type);

	switch (command.returns[1].data_type) {
	case data_err:
	case data_ok:
		validate_type(vars[command.returns[0].data_index].type,
			&command.returns[1], 0);
		break;

	case data_null:
		validate_variable(0, data_string, "NULL",
				  command.returns[0].data_index, 0);
		break;

	case data_nonnull:
		validate_variable(0, data_string, "NULL",
				  command.returns[0].data_index, 1);
		break;

	case data_string:
	case data_number:
		if (verbose) {
			fprintf(stderr, " %s == returned %s\n",
			    (const char *)command.returns[1].data_value,
			    (const char *)
			    vars[command.returns[0].data_index].value);
		}
		validate_variable(0, data_string,
		    command.returns[1].data_value,
		    command.returns[0].data_index, 0);
		break;

	case data_byte:
		vptr = &vars[command.returns[0].data_index];
		retvar.data_len = vptr->len;
		retvar.data_type = vptr->type;
		retvar.data_value = vptr->value;
		validate_byte(&retvar, &command.returns[1], 0);
		break;

	case data_cchar:
		validate_cchar(&vars[command.returns[0].data_index].cchar,
			(cchar_t *) command.returns[1].data_value, 0);
		break;

	case data_wchar:
		validate_wchar((wchar_t *) vars[command.returns[0].data_index].value,
			(wchar_t *) command.returns[1].data_value, 0);
		break;

	default:
		errx(1, "%s:%zu: Malformed check statement", cur_file, line);
		break;
	}

	init_parse_variables(0);
}

static void
delay_millis(const char *millis)
{
	/* set the inter-character delay */
	if (sscanf(millis, "%d", &input_delay) == 0)
		errx(1, "%s:%zu: Delay specification %s must be an int",
		    cur_file, line, millis);
	if (verbose) {
		fprintf(stderr, "Set input delay to %d ms\n", input_delay);
	}

	if (input_delay < DELAY_MIN)
		input_delay = DELAY_MIN;
	/*
	 * Fill in the timespec structure now ready for use later.
	 * The delay is specified in milliseconds so convert to timespec
	 * values
	 */
	delay_spec.tv_sec = input_delay / 1000;
	delay_spec.tv_nsec = (input_delay - 1000 * delay_spec.tv_sec) * 1000;
	if (verbose) {
		fprintf(stderr, "set delay to %jd.%jd\n",
		    (intmax_t)delay_spec.tv_sec,
		    (intmax_t)delay_spec.tv_nsec);
	}

	init_parse_variables(0);
}

static void
do_input(const char *s)
{
	if (input_str != NULL) {
		warnx("%s:%zu: Discarding unused input string", cur_file, line);
		free(input_str);
	}

	if ((input_str = strdup(s)) == NULL)
		err(2, "Cannot allocate memory for input string");
}

static void
do_noinput(void)
{
	if (input_str != NULL) {
		warnx("%s:%zu: Discarding unused input string", cur_file, line);
		free(input_str);
	}

	no_input = true;
}

/*
 * Initialise the command structure - if initial is non-zero then just set
 * everything to sane values otherwise free any memory that was allocated
 * when building the structure.
 */
void
init_parse_variables(int initial)
{
	int i, result;
	struct pollfd slave_pty;

	if (initial == 0) {
		free(command.function);
		for (i = 0; i < command.nrets; i++) {
			if (command.returns[i].data_type == data_number)
				free(command.returns[i].data_value);
		}
		free(command.returns);

		for (i = 0; i < command.nargs; i++) {
			if (command.args[i].arg_type != data_var)
				free(command.args[i].arg_string);
		}
		free(command.args);
	} else {
		line = 1;
		input_delay = 0;
		vars = NULL;
		nvars = 0;
		input_str = NULL;
		saved_output.allocated = 0;
		saved_output.count = 0;
		saved_output.readp = 0;
		saved_output.data = NULL;
	}

	no_input = false;
	command.function = NULL;
	command.nargs = 0;
	command.args = NULL;
	command.nrets = 0;
	command.returns = NULL;

	/*
	 * Check the slave pty for stray output from the slave, at this
	 * point we should not see any data as it should have been
	 * consumed by the test functions.  If we see data then we have
	 * either a bug or are not handling an output generating function
	 * correctly.
	 */
	slave_pty.fd = master;
	slave_pty.events = POLLIN;
	result = poll(&slave_pty, 1, 0);

	if (result < 0)
		err(2, "Poll of slave pty failed");
	else if (result > 0)
		warnx("%s:%zu: Unexpected data from slave", cur_file, line);
}

/*
 * Validate the response against the expected return.  The variable
 * i is the i into the rets array in command.
 */
static void
validate(int i, void *data)
{
	char *response;
	ct_data_t *byte_response;

	byte_response = data;
	if ((command.returns[i].data_type != data_byte) &&
	    (command.returns[i].data_type != data_err) &&
	    (command.returns[i].data_type != data_ok)) {
		if ((byte_response->data_type == data_byte) ||
		    (byte_response->data_type == data_err) ||
		    (byte_response->data_type == data_ok))
			errx(1,
			    "%s:%zu: %s: expecting type %s, received type %s",
			    cur_file, line, __func__,
			    enum_names[command.returns[i].data_type],
			    enum_names[byte_response->data_type]);

		response = byte_response->data_value;
	}

	switch (command.returns[i].data_type) {
	case data_err:
		validate_type(data_err, byte_response, 0);
		break;

	case data_ok:
		validate_type(data_ok, byte_response, 0);
		break;

	case data_null:
		validate_return("NULL", response, 0);
		break;

	case data_nonnull:
		validate_return("NULL", response, 1);
		break;

	case data_string:
	case data_number:
		validate_return(command.returns[i].data_value,
				response, 0);
		break;

	case data_ref:
		validate_reference(i, response);
		break;

	case data_byte:
		validate_byte(&command.returns[i], byte_response, 0);
		break;

	default:
		errx(1, "%s:%zu: Malformed statement", cur_file, line);
		break;
	}
}

/*
 * Validate the return against the contents of a variable.
 */
static void
validate_reference(int i, void *data)
{
	char *response;
	ct_data_t *byte_response;
	var_t *varp;

	varp = &vars[command.returns[i].data_index];

	byte_response = data;
	if (command.returns[i].data_type != data_byte)
		response = data;

	if (verbose) {
		fprintf(stderr,
		    "%s: return type of %s, value %s \n", __func__,
		    enum_names[varp->type],
		    (varp->type != data_cchar && varp->type != data_wchar)
			? (const char *)varp->value : "-");
	}

	switch (varp->type) {
	case data_string:
	case data_number:
		validate_return(varp->value, response, 0);
		break;

	case data_byte:
		validate_byte(varp->value, byte_response, 0);
		break;

	case data_cchar:
		validate_cchar(&(varp->cchar), (cchar_t *) response, 0);
		break;

	case data_wchar:
		validate_wchar((wchar_t *) varp->value, (wchar_t *) response, 0);
		break;

	default:
		errx(1, "%s:%zu: Invalid return type for reference",
		    cur_file, line);
		break;
	}
}

/*
 * Validate the return type against the expected type, throw an error
 * if they don't match.
 */
static void
validate_type(data_enum_t expected, ct_data_t *value, int check)
{
	if (((check == 0) && (expected != value->data_type)) ||
	    ((check == 1) && (expected == value->data_type)))
		errx(1, "%s:%zu: Validate expected type %s %s %s",
		    cur_file, line,
		    enum_names[expected],
		    (check == 0)? "matching" : "not matching",
		    enum_names[value->data_type]);

	if (verbose) {
		fprintf(stderr, "%s:%zu: Validated expected type %s %s %s\n",
		    cur_file, line,
		    enum_names[expected],
		    (check == 0)? "matching" : "not matching",
		    enum_names[value->data_type]);
	}
}

/*
 * Validate the return value against the expected value, throw an error
 * if they don't match.
 */
static void
validate_return(const char *expected, const char *value, int check)
{
	if (((check == 0) && strcmp(expected, value) != 0) ||
	    ((check == 1) && strcmp(expected, value) == 0))
		errx(1, "%s:%zu: Validate expected >%s< %s >%s<",
		    cur_file, line,
		    expected,
		    (check == 0)? "matching" : "not matching",
		    value);
	if (verbose) {
		fprintf(stderr,
		    "%s:%zu: Validated expected value >%s< %s >%s<\n",
		    cur_file, line,
		    expected,
		    (check == 0)? "matches" : "does not match",
		    value);
	}
}

/*
 * Validate the return value against the expected value, throw an error
 * if they don't match expectations.
 */
static void
validate_byte(ct_data_t *expected, ct_data_t *value, int check)
{
	char *ch;
	size_t i;

	if (verbose) {
		ch = value->data_value;
		fprintf(stderr, "checking returned byte stream: ");
		for (i = 0; i < value->data_len; i++)
			fprintf(stderr, "%s0x%x", (i != 0)? ", " : "", ch[i]);
		fprintf(stderr, "\n");

		fprintf(stderr, "%s byte stream: ",
			(check == 0)? "matches" : "does not match");
		ch = (char *) expected->data_value;
		for (i = 0; i < expected->data_len; i++)
			fprintf(stderr, "%s0x%x", (i != 0)? ", " : "", ch[i]);
		fprintf(stderr, "\n");
	}

	/*
	 * No chance of a match if lengths differ...
	 */
	if ((check == 0) && (expected->data_len != value->data_len))
		errx(1,
		    "Byte validation failed, length mismatch, "
		    "expected %zu, received %zu",
		    expected->data_len, value->data_len);

	/*
	 * If check is 0 then we want to throw an error IFF the byte streams
	 * do not match, if check is 1 then throw an error if the byte
	 * streams match.
	 */
	if (((check == 0) && memcmp(expected->data_value, value->data_value,
				    value->data_len) != 0) ||
	    ((check == 1) && (expected->data_len == value->data_len) &&
	     memcmp(expected->data_value, value->data_value,
		    value->data_len) == 0))
		errx(1, "%s:%zu: Validate expected %s byte stream",
		    cur_file, line,
		    (check == 0)? "matching" : "not matching");
	if (verbose) {
		fprintf(stderr, "%s:%zu: Validated expected %s byte stream\n",
		    cur_file, line,
		    (check == 0)? "matching" : "not matching");
	}
}

/*
 * Validate the return cchar against the expected cchar, throw an error
 * if they don't match expectations.
 */
static void
validate_cchar(cchar_t *expected, cchar_t *value, int check)
{
	unsigned j;

	/*
	 * No chance of a match if elements count differ...
	 */
	if ((expected->elements != value->elements)) {
		if (check == 0)
			errx(1,
			    "cchar validation failed, elements count mismatch, "
			    "expected %d, received %d",
			    expected->elements, value->elements);
		else {
			if (verbose)
				fprintf(stderr,
				    "%s:%zu: Validated expected %s cchar",
				    cur_file, line, "not matching");
			return;
		}
	}

	/*
	 * No chance of a match if attributes differ...
	 */

	if ((expected->attributes & WA_ATTRIBUTES) !=
			(value->attributes & WA_ATTRIBUTES )) {
		if (check == 0)
			errx(1,
			    "cchar validation failed, attributes mismatch, "
			    "expected 0x%x, received 0x%x",
			    expected->attributes & WA_ATTRIBUTES,
			    value->attributes & WA_ATTRIBUTES);
		else {
			if (verbose)
				fprintf(stderr,
				    "%s:%zu: Validated expected %s cchar\n",
				    cur_file, line, "not matching");
			return;
		}
	}

	/*
	 * If check is 0 then we want to throw an error IFF the vals
	 * do not match, if check is 1 then throw an error if the vals
	 * streams match.
	 */
	for(j = 0; j < expected->elements; j++) {
		if (expected->vals[j] != value->vals[j]) {
			if (check == 0)
				errx(1,
				    "cchar validation failed, vals mismatch, "
				    "expected 0x%x, received 0x%x",
				    expected->vals[j], value->vals[j]);
			else {
				if (verbose)
					fprintf(stderr,
					    "%s:%zu: Validated expected %s "
					    "cchar\n",
					    cur_file, line, "not matching");
				return;
			}
		}
	}

	if (verbose) {
		fprintf(stderr,
		    "%s:%zu: Validated expected %s cchar\n",
		    cur_file, line, (check == 0)? "matching" : "not matching");
	}
}

/*
 * Validate the return wchar string against the expected wchar, throw an
 * error if they don't match expectations.
 */
static void
validate_wchar(wchar_t *expected, wchar_t *value, int check)
{
	unsigned j;

	unsigned len1 = 0;
	unsigned len2 = 0;
	wchar_t *p;

	p = expected;
	while (*p++ != L'\0')
		len1++;

	p = value;
	while (*p++ != L'\0')
		len2++;

	/*
	 * No chance of a match if length differ...
	 */
	if (len1 != len2) {
		if (check == 0)
			errx(1,
			    "wchar string validation failed, length mismatch, "
			    "expected %d, received %d",
			    len1, len2);
		else {
			if (verbose)
				fprintf(stderr,
				    "%s:%zu: Validated expected %s wchar\n",
				    cur_file, line, "not matching");
			return;
		}
	}

	/*
	 * If check is 0 then we want to throw an error IFF the vals
	 * do not match, if check is 1 then throw an error if the vals
	 * streams match.
	 */
	for(j = 0; j < len1; j++) {
		if (expected[j] != value[j]) {
			if (check == 0)
				errx(1, "wchar validation failed at index %d, expected %d,"
				"received %d", j, expected[j], value[j]);
			else {
				if (verbose)
					fprintf(stderr,
					    "%s:%zu: Validated expected %s wchar\n",
					    cur_file, line, "not matching");
				return;
			}
		}
	}

	if (verbose) {
		fprintf(stderr,
		    "%s:%zu: Validated expected %s wchar\n",
		    cur_file, line,
		    (check == 0)? "matching" : "not matching");
	}
}

/*
 * Validate the variable at i against the expected value, throw an
 * error if they don't match, if check is non-zero then the match is
 * negated.
 */
static void
validate_variable(int ret, data_enum_t type, const void *value, int i,
    int check)
{
	ct_data_t *retval;
	var_t *varptr;

	retval = &command.returns[ret];
	varptr = &vars[command.returns[ret].data_index];

	if (varptr->value == NULL)
		errx(1, "Variable %s has no value assigned to it", varptr->name);


	if (varptr->type != type)
		errx(1, "Variable %s is not the expected type", varptr->name);

	if (type != data_byte) {
		if ((((check == 0) && strcmp(value, varptr->value) != 0))
		    || ((check == 1) && strcmp(value, varptr->value) == 0))
			errx(1, "%s:%zu: Variable %s contains %s instead of %s"
			    " value %s",
			    cur_file, line,
			    varptr->name, (const char *)varptr->value,
			    (check == 0)? "expected" : "not matching",
			    (const char *)value);
		if (verbose) {
			fprintf(stderr,
			    "%s:%zu: Variable %s contains %s value %s\n",
			    cur_file, line,
			    varptr->name,
			    (check == 0)? "expected" : "not matching",
			    (const char *)varptr->value);
		}
	} else {
		if ((check == 0) && (retval->data_len != varptr->len))
			errx(1, "Byte validation failed, length mismatch");

		/*
		 * If check is 0 then we want to throw an error IFF
		 * the byte streams do not match, if check is 1 then
		 * throw an error if the byte streams match.
		 */
		if (((check == 0) && memcmp(retval->data_value, varptr->value,
					    varptr->len) != 0) ||
		    ((check == 1) && (retval->data_len == varptr->len) &&
		     memcmp(retval->data_value, varptr->value,
			    varptr->len) == 0))
			errx(1, "%s:%zu: Validate expected %s byte stream",
			    cur_file, line,
			    (check == 0)? "matching" : "not matching");
		if (verbose) {
			fprintf(stderr,
			    "%s:%zu: Validated expected %s byte stream\n",
			    cur_file, line,
			    (check == 0)? "matching" : "not matching");
		}
	}
}

/*
 * Write a string to the command pipe - we feed the number of bytes coming
 * down first to allow storage allocation and then follow up with the data.
 * If cmd is NULL then feed a -1 down the pipe to say the end of the args.
 */
static void
write_cmd_pipe(char *cmd)
{
	args_t arg;
	size_t len;

	if (cmd == NULL)
		len = 0;
	else
		len = strlen(cmd);

	arg.arg_type = data_static;
	arg.arg_len = len;
	arg.arg_string = cmd;
	write_cmd_pipe_args(arg.arg_type, &arg);

}

static void
write_cmd_pipe_args(data_enum_t type, void *data)
{
	var_t *var_data;
	args_t *arg_data;
	int len, send_type;
	void *cmd;

	arg_data = data;
	switch (type) {
	case data_var:
		var_data = data;
		len = var_data->len;
		cmd = var_data->value;

		switch (var_data->type) {
		case data_byte:
			send_type = data_byte;
			break;

		case data_cchar:
			send_type = data_cchar;
			cmd = (void *) &var_data->cchar;
			len = sizeof(cchar_t);
			break;

		case data_wchar:
			send_type = data_wchar;
			break;

		default:
			send_type = data_string;
			break;
		}
		break;

	case data_null:
		send_type = data_null;
		len = 0;
		break;

	default:
		if ((arg_data->arg_len == 0) && (arg_data->arg_string == NULL))
			len = -1;
		else
			len = arg_data->arg_len;
		cmd = arg_data->arg_string;
		if (type == data_byte)
			send_type = data_byte;
		else
			send_type = data_string;
	}

	if (verbose) {
		fprintf(stderr, "Writing type %s to command pipe\n",
		    enum_names[send_type]);
	}

	if (write(to_slave, &send_type, sizeof(int)) < 0)
		err(1, "command pipe write for type failed");

	if (verbose) {
		if (send_type == data_cchar)
			fprintf(stderr,
			    "Writing cchar to command pipe\n");
		else if (send_type == data_wchar)
			fprintf(stderr,
			    "Writing wchar(%d sized) to command pipe\n", len);
		else
			fprintf(stderr,
			    "Writing length %d to command pipe\n", len);
	}

	if (write(to_slave, &len, sizeof(int)) < 0)
		err(1, "command pipe write for length failed");

	if (len > 0) {
		if (verbose) {
			fprintf(stderr, "Writing data >%s< to command pipe\n",
			    (const char *)cmd);
		}
		if (write(to_slave, cmd, len) < 0)
			err(1, "command pipe write of data failed");
	}
}

/*
 * Read a response from the command pipe, first we will receive the
 * length of the response then the actual data.
 */
static void
read_cmd_pipe(ct_data_t *response)
{
	int len, type;
	struct pollfd rfd[2];
	char *str;

	/*
	 * Check if there is data to read - just in case slave has died, we
	 * don't want to block on the read and just hang.  We also check
	 * output from the slave because the slave may be blocked waiting
	 * for a flush on its stdout.
	 */
	rfd[0].fd = from_slave;
	rfd[0].events = POLLIN;
	rfd[1].fd = master;
	rfd[1].events = POLLIN;

	do {
		if (poll(rfd, 2, 4000) == 0)
			errx(2, "%s:%zu: Command pipe read timeout",
			    cur_file, line);

		if ((rfd[1].revents & POLLIN) == POLLIN) {
			if (verbose) {
				fprintf(stderr,
				    "draining output from slave\n");
			}
			save_slave_output(false);
		}
	}
	while ((rfd[1].revents & POLLIN) == POLLIN);

	if (read(from_slave, &type, sizeof(int)) < 0)
		err(1, "command pipe read for type failed");
	response->data_type = type;

	if ((type != data_ok) && (type != data_err) && (type != data_count)) {
		if (read(from_slave, &len, sizeof(int)) < 0)
			err(1, "command pipe read for length failed");
		response->data_len = len;

		if (verbose) {
			fprintf(stderr,
			    "Reading %d bytes from command pipe\n", len);
		}

		if ((response->data_value = malloc(len + 1)) == NULL)
			err(1, "Failed to alloc memory for cmd pipe read");

		if (read(from_slave, response->data_value, len) < 0)
			err(1, "command pipe read of data failed");

		if (response->data_type != data_byte) {
			str = response->data_value;
			str[len] = '\0';

			if (verbose) {
				fprintf(stderr, "Read data >%s< from pipe\n",
				    (const char *)response->data_value);
			}
		}
	} else {
		response->data_value = NULL;
		if (type == data_count) {
			if (read(from_slave, &len, sizeof(int)) < 0)
				err(1, "command pipe read for number of "
				       "returns failed");
			response->data_len = len;
		}

		if (verbose) {
			fprintf(stderr, "Read type %s from pipe\n",
			    enum_names[type]);
		}
	}
}

/*
 * Check for writes from the slave on the pty, save the output into a
 * buffer for later checking if discard is false.
 */
#define MAX_DRAIN 256

static void
save_slave_output(bool discard)
{
	char *new_data, drain[MAX_DRAIN];
	size_t to_allocate;
	ssize_t result;
	size_t i;

	result = 0;
	for (;;) {
		if (result == -1)
			err(2, "poll of slave pty failed");
		result = MAX_DRAIN;
		if ((result = read(master, drain, result)) < 0) {
			if (errno == EAGAIN)
				break;
			else
				err(2, "draining slave pty failed");
		}
		if (result == 0)
			abort();

		if (!discard) {
			if ((size_t)result >
			    (saved_output.allocated - saved_output.count)) {
				to_allocate = 1024 * ((result / 1024) + 1);

				if ((new_data = realloc(saved_output.data,
					saved_output.allocated + to_allocate))
				    == NULL)
					err(2, "Realloc of saved_output failed");
				saved_output.data = new_data;
				saved_output.allocated += to_allocate;
			}

			if (verbose) {
				fprintf(stderr,
				    "count = %zu, allocated = %zu\n",
				    saved_output.count, saved_output.allocated);
				for (i = 0; i < (size_t)result; i++) {
					fprintf(stderr, "Saving slave output "
					    "at %zu: 0x%x (%c)\n",
					    saved_output.count + i, drain[i],
					    (drain[i] >= ' ')? drain[i] : '-');
				}
			}

			memcpy(&saved_output.data[saved_output.count], drain,
			       result);
			saved_output.count += result;

			if (verbose) {
				fprintf(stderr,
				    "count = %zu, allocated = %zu\n",
				    saved_output.count, saved_output.allocated);
			}
		} else {
			if (verbose) {
				for (i = 0; i < (size_t)result; i++) {
					fprintf(stderr, "Discarding slave "
					    "output 0x%x (%c)\n",
					    drain[i],
					    (drain[i] >= ' ')? drain[i] : '-');
				}
			}
		}
	}
}

static void
yyerror(const char *msg)
{
	errx(1, "%s:%zu: %s", cur_file, line, msg);
}
