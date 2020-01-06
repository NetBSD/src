#include <stdlib.h>
#include <string.h>
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define YYLEX yylex()
#define YYEMPTY -1
#define yyclearin (yychar=(YYEMPTY))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING() (yyerrflag!=0)
#define YYPREFIX "yy"
#line 20 "cmd-parse.y"

#include <sys/types.h>

#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tmux.h"

static int			 yylex(void);
static int			 yyparse(void);
static int printflike(1,2)	 yyerror(const char *, ...);

static char			*yylex_token(int);
static char			*yylex_format(void);

struct cmd_parse_scope {
	int				 flag;
	TAILQ_ENTRY (cmd_parse_scope)	 entry;
};

struct cmd_parse_command {
	char				 *name;
	u_int				  line;

	int				  argc;
	char				**argv;

	TAILQ_ENTRY(cmd_parse_command)	  entry;
};
TAILQ_HEAD(cmd_parse_commands, cmd_parse_command);

struct cmd_parse_state {
	FILE				*f;

	const char			*buf;
	size_t				 len;
	size_t				 off;

	int				 condition;
	int				 eol;
	int				 eof;
	struct cmd_parse_input		*input;
	u_int				 escapes;

	char				*error;
	struct cmd_parse_commands	*commands;

	struct cmd_parse_scope		*scope;
	TAILQ_HEAD(, cmd_parse_scope)	 stack;
};
static struct cmd_parse_state parse_state;

static char	*cmd_parse_get_error(const char *, u_int, const char *);
static void	 cmd_parse_free_command(struct cmd_parse_command *);
static struct cmd_parse_commands *cmd_parse_new_commands(void);
static void	 cmd_parse_free_commands(struct cmd_parse_commands *);
static void	 cmd_parse_print_commands(struct cmd_parse_input *, u_int,
		     struct cmd_list *);

#line 85 "cmd-parse.y"
#ifndef YYSTYPE_DEFINED
#define YYSTYPE_DEFINED
typedef union
{
	char					 *token;
	struct {
		int				  argc;
		char				**argv;
	} arguments;
	int					  flag;
	struct {
		int				  flag;
		struct cmd_parse_commands	 *commands;
	} elif;
	struct cmd_parse_commands		 *commands;
	struct cmd_parse_command		 *command;
} YYSTYPE;
#endif /* YYSTYPE_DEFINED */
#line 96 "cmd-parse.c"
#define ERROR 257
#define IF 258
#define ELSE 259
#define ELIF 260
#define ENDIF 261
#define FORMAT 262
#define TOKEN 263
#define EQUALS 264
#define YYERRCODE 256
const short yylhs[] =
	{                                        -1,
    0,    0,    9,    9,   10,   10,   10,    3,    3,    2,
   15,   15,    5,   16,    6,   17,   12,   12,   12,   12,
    7,    7,   11,   11,   11,   11,   11,   14,   14,   13,
   13,   13,   13,    8,    8,    4,    4,    1,    1,
};
const short yylen[] =
	{                                         2,
    0,    1,    2,    3,    1,    1,    1,    1,    1,    1,
    0,    1,    2,    1,    2,    1,    4,    7,    5,    8,
    3,    4,    1,    2,    3,    3,    1,    2,    3,    3,
    5,    4,    6,    2,    3,    1,    2,    1,    1,
};
const short yydefred[] =
	{                                      0,
    0,   12,    0,    0,    0,    0,    0,    5,   27,   23,
    0,    8,    9,   13,   10,    0,    0,    0,    0,    0,
    3,    0,    0,    0,   14,    0,   16,    0,    0,    0,
   30,    4,   25,   26,   38,   39,    0,   29,    0,    0,
    0,   17,   15,    0,    0,   32,    0,   37,    0,    0,
   19,    0,   35,    0,   31,    0,    0,    0,   33,   22,
    0,   18,   20,
};
const short yydgoto[] =
	{                                       3,
   37,   14,   15,   38,    4,   28,   40,   29,    5,    6,
    7,    8,    9,   10,   11,   30,   31,
};
const short yysindex[] =
	{                                   -214,
 -241,    0,    0,  -10, -214,    3,  -41,    0,    0,    0,
 -232,    0,    0,    0,    0, -214, -214,  -56, -232,   25,
    0, -214, -201, -234,    0, -241,    0, -214, -184, -214,
    0,    0,    0,    0,    0,    0, -201,    0,   29, -184,
   39,    0,    0,  -52, -214,    0,  -55,    0, -214,   49,
    0, -214,    0,  -55,    0, -206, -214, -185,    0,    0,
 -185,    0,    0,};
const short yyrindex[] =
	{                                      1,
    0,    0,    0, -195,    2,    0,   64,    0,    0,    0,
   70,    0,    0,    0,    0,   -2, -195,    0,    0,    0,
    0,   -4,    7,   -2,    0,    0,    0, -195,    0, -195,
    0,    0,    0,    0,    0,    0,   10,    0,    0,    0,
    0,    0,    0, -178, -195,    0,    0,    0,   -2,    0,
    0,   -2,    0,    0,    0,   -1,   -2,   -2,    0,    0,
   -2,    0,    0,};
const short yygindex[] =
	{                                      0,
    0,   58,    0,   50,    6,   -9,   30,   44,  -11,    9,
   12,    0,   67,   68,   15,   32,   24,
};
#define YYTABLESIZE 271
const short yytable[] =
	{                                      16,
    1,    2,   22,   22,   24,   24,   22,   11,   11,   17,
   11,   11,   21,   20,   39,   18,   28,   22,   19,   36,
   12,   13,   17,    1,   25,   26,   27,   17,   18,    2,
   23,   19,   20,   17,   32,   17,   19,   56,   49,   44,
   58,   47,   19,    1,   19,   61,   39,   42,   52,    2,
   17,    1,   46,   26,   24,   41,   54,    2,   57,   19,
   45,   35,   36,   51,   20,   28,   20,   11,   36,   20,
   55,   50,    1,    7,   25,   27,   27,   59,    2,    6,
   34,   62,   34,   43,   63,   60,   48,   53,   33,   34,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   25,   26,   27,   27,    0,   26,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    1,    0,    0,
    0,    0,    0,    2,   24,   24,   24,   21,   11,   21,
   11,   11,    0,   11,   11,   28,   28,   28,   36,   36,
   36,
};
const short yycheck[] =
	{                                      10,
    0,    0,   59,   59,   16,   10,   59,   10,   10,    4,
   10,   10,   10,    5,   24,    4,   10,   59,    4,   10,
  262,  263,   17,  258,  259,  260,  261,   22,   17,  264,
  263,   17,   24,   28,   10,   30,   22,   49,   10,   28,
   52,   30,   28,  258,   30,   57,   56,   24,   10,  264,
   45,  258,   29,  260,   59,   24,   45,  264,   10,   45,
   29,  263,  264,   40,   56,   59,   58,  263,   59,   61,
   47,   40,  258,   10,  259,  261,  261,   54,  264,   10,
  259,   58,  261,   26,   61,   56,   37,   44,   22,   22,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,  259,  260,  261,  261,   -1,  260,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  258,   -1,   -1,
   -1,   -1,   -1,  264,  259,  260,  261,  259,  263,  261,
  263,  263,   -1,  263,  263,  259,  260,  261,  259,  260,
  261,
};
#define YYFINAL 3
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 264
#if YYDEBUG
const char * const yyname[] =
	{
"end-of-file",0,0,0,0,0,0,0,0,0,"'\\n'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"';'",0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"ERROR","IF","ELSE",
"ELIF","ENDIF","FORMAT","TOKEN","EQUALS",
};
const char * const yyrule[] =
	{"$accept : lines",
"lines :",
"lines : statements",
"statements : statement '\\n'",
"statements : statements statement '\\n'",
"statement : condition",
"statement : assignment",
"statement : commands",
"format : FORMAT",
"format : TOKEN",
"expanded : format",
"assignment :",
"assignment : EQUALS",
"if_open : IF expanded",
"if_else : ELSE",
"if_elif : ELIF expanded",
"if_close : ENDIF",
"condition : if_open '\\n' statements if_close",
"condition : if_open '\\n' statements if_else '\\n' statements if_close",
"condition : if_open '\\n' statements elif if_close",
"condition : if_open '\\n' statements elif if_else '\\n' statements if_close",
"elif : if_elif '\\n' statements",
"elif : if_elif '\\n' statements elif",
"commands : command",
"commands : commands ';'",
"commands : commands ';' condition1",
"commands : commands ';' command",
"commands : condition1",
"command : assignment TOKEN",
"command : assignment TOKEN arguments",
"condition1 : if_open commands if_close",
"condition1 : if_open commands if_else commands if_close",
"condition1 : if_open commands elif1 if_close",
"condition1 : if_open commands elif1 if_else commands if_close",
"elif1 : if_elif commands",
"elif1 : if_elif commands elif1",
"arguments : argument",
"arguments : argument arguments",
"argument : TOKEN",
"argument : EQUALS",
};
#endif
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 10000
#define YYMAXDEPTH 10000
#endif
#endif
#define YYINITSTACKSIZE 200
/* LINTUSED */
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
short *yyss;
short *yysslim;
YYSTYPE *yyvs;
unsigned int yystacksize;
int yyparse(void);
#line 499 "cmd-parse.y"

static char *
cmd_parse_get_error(const char *file, u_int line, const char *error)
{
	char	*s;

	if (file == NULL)
		s = xstrdup(error);
	else
		xasprintf (&s, "%s:%u: %s", file, line, error);
	return (s);
}

static void
cmd_parse_print_commands(struct cmd_parse_input *pi, u_int line,
    struct cmd_list *cmdlist)
{
	char	*s;

	if (pi->item != NULL && (pi->flags & CMD_PARSE_VERBOSE)) {
		s = cmd_list_print(cmdlist, 0);
		if (pi->file != NULL)
			cmdq_print(pi->item, "%s:%u: %s", pi->file, line, s);
		else
			cmdq_print(pi->item, "%u: %s", line, s);
		free(s);
	}
}

static void
cmd_parse_free_command(struct cmd_parse_command *cmd)
{
	free(cmd->name);
	cmd_free_argv(cmd->argc, cmd->argv);
	free(cmd);
}

static struct cmd_parse_commands *
cmd_parse_new_commands(void)
{
	struct cmd_parse_commands	*cmds;

	cmds = xmalloc(sizeof *cmds);
	TAILQ_INIT (cmds);
	return (cmds);
}

static void
cmd_parse_free_commands(struct cmd_parse_commands *cmds)
{
	struct cmd_parse_command	*cmd, *cmd1;

	TAILQ_FOREACH_SAFE(cmd, cmds, entry, cmd1) {
		TAILQ_REMOVE(cmds, cmd, entry);
		cmd_parse_free_command(cmd);
	}
	free(cmds);
}

static struct cmd_parse_commands *
cmd_parse_run_parser(char **cause)
{
	struct cmd_parse_state	*ps = &parse_state;
	struct cmd_parse_scope	*scope, *scope1;
	int			 retval;

	ps->commands = NULL;
	TAILQ_INIT(&ps->stack);

	retval = yyparse();
	TAILQ_FOREACH_SAFE(scope, &ps->stack, entry, scope1) {
		TAILQ_REMOVE(&ps->stack, scope, entry);
		free(scope);
	}
	if (retval != 0) {
		*cause = ps->error;
		return (NULL);
	}

	if (ps->commands == NULL)
		return (cmd_parse_new_commands());
	return (ps->commands);
}

static struct cmd_parse_commands *
cmd_parse_do_file(FILE *f, struct cmd_parse_input *pi, char **cause)
{
	struct cmd_parse_state	*ps = &parse_state;

	memset(ps, 0, sizeof *ps);
	ps->input = pi;
	ps->f = f;
	return (cmd_parse_run_parser(cause));
}

static struct cmd_parse_commands *
cmd_parse_do_buffer(const char *buf, size_t len, struct cmd_parse_input *pi,
    char **cause)
{
	struct cmd_parse_state	*ps = &parse_state;

	memset(ps, 0, sizeof *ps);
	ps->input = pi;
	ps->buf = buf;
	ps->len = len;
	return (cmd_parse_run_parser(cause));
}

static struct cmd_parse_result *
cmd_parse_build_commands(struct cmd_parse_commands *cmds,
    struct cmd_parse_input *pi)
{
	static struct cmd_parse_result	 pr;
	struct cmd_parse_commands	*cmds2;
	struct cmd_parse_command	*cmd, *cmd2, *next, *next2, *after;
	u_int				 line = UINT_MAX;
	int				 i;
	struct cmd_list			*cmdlist = NULL, *result;
	struct cmd			*add;
	char				*alias, *cause, *s;

	/* Check for an empty list. */
	if (TAILQ_EMPTY(cmds)) {
		cmd_parse_free_commands(cmds);
		pr.status = CMD_PARSE_EMPTY;
		return (&pr);
	}

	/*
	 * Walk the commands and expand any aliases. Each alias is parsed
	 * individually to a new command list, any trailing arguments appended
	 * to the last command, and all commands inserted into the original
	 * command list.
	 */
	TAILQ_FOREACH_SAFE(cmd, cmds, entry, next) {
		alias = cmd_get_alias(cmd->name);
		if (alias == NULL)
			continue;

		line = cmd->line;
		log_debug("%s: %u %s = %s", __func__, line, cmd->name, alias);

		pi->line = line;
		cmds2 = cmd_parse_do_buffer(alias, strlen(alias), pi, &cause);
		free(alias);
		if (cmds2 == NULL) {
			pr.status = CMD_PARSE_ERROR;
			pr.error = cause;
			goto out;
		}

		cmd2 = TAILQ_LAST(cmds2, cmd_parse_commands);
		if (cmd2 == NULL) {
			TAILQ_REMOVE(cmds, cmd, entry);
			cmd_parse_free_command(cmd);
			continue;
		}
		for (i = 0; i < cmd->argc; i++)
			cmd_append_argv(&cmd2->argc, &cmd2->argv, cmd->argv[i]);

		after = cmd;
		TAILQ_FOREACH_SAFE(cmd2, cmds2, entry, next2) {
			cmd2->line = line;
			TAILQ_REMOVE(cmds2, cmd2, entry);
			TAILQ_INSERT_AFTER(cmds, after, cmd2, entry);
			after = cmd2;
		}
		cmd_parse_free_commands(cmds2);

		TAILQ_REMOVE(cmds, cmd, entry);
		cmd_parse_free_command(cmd);
	}

	/*
	 * Parse each command into a command list. Create a new command list
	 * for each line so they get a new group (so the queue knows which ones
	 * to remove if a command fails when executed).
	 */
	result = cmd_list_new();
	TAILQ_FOREACH(cmd, cmds, entry) {
		log_debug("%s: %u %s", __func__, cmd->line, cmd->name);
		cmd_log_argv(cmd->argc, cmd->argv, __func__);

		if (cmdlist == NULL || cmd->line != line) {
			if (cmdlist != NULL) {
				cmd_parse_print_commands(pi, line, cmdlist);
				cmd_list_move(result, cmdlist);
				cmd_list_free(cmdlist);
			}
			cmdlist = cmd_list_new();
		}
		line = cmd->line;

		cmd_prepend_argv(&cmd->argc, &cmd->argv, cmd->name);
		add = cmd_parse(cmd->argc, cmd->argv, pi->file, line, &cause);
		if (add == NULL) {
			cmd_list_free(result);
			pr.status = CMD_PARSE_ERROR;
			pr.error = cmd_parse_get_error(pi->file, line, cause);
			free(cause);
			goto out;
		}
		cmd_list_append(cmdlist, add);
	}
	if (cmdlist != NULL) {
		cmd_parse_print_commands(pi, line, cmdlist);
		cmd_list_move(result, cmdlist);
		cmd_list_free(cmdlist);
	}

	s = cmd_list_print(result, 0);
	log_debug("%s: %s", __func__, s);
	free(s);

	pr.status = CMD_PARSE_SUCCESS;
	pr.cmdlist = result;

out:
	cmd_parse_free_commands(cmds);

	return (&pr);
}

struct cmd_parse_result *
cmd_parse_from_file(FILE *f, struct cmd_parse_input *pi)
{
	static struct cmd_parse_result	 pr;
	struct cmd_parse_input		 input;
	struct cmd_parse_commands	*cmds;
	char				*cause;

	if (pi == NULL) {
		memset(&input, 0, sizeof input);
		pi = &input;
	}
	memset(&pr, 0, sizeof pr);

	cmds = cmd_parse_do_file(f, pi, &cause);
	if (cmds == NULL) {
		pr.status = CMD_PARSE_ERROR;
		pr.error = cause;
		return (&pr);
	}
	return (cmd_parse_build_commands(cmds, pi));
}

struct cmd_parse_result *
cmd_parse_from_string(const char *s, struct cmd_parse_input *pi)
{
	static struct cmd_parse_result	 pr;
	struct cmd_parse_input		 input;
	struct cmd_parse_commands	*cmds;
	char				*cause;

	if (pi == NULL) {
		memset(&input, 0, sizeof input);
		pi = &input;
	}
	memset(&pr, 0, sizeof pr);

	if (*s == '\0') {
		pr.status = CMD_PARSE_EMPTY;
		pr.cmdlist = NULL;
		pr.error = NULL;
		return (&pr);
	}

	cmds = cmd_parse_do_buffer(s, strlen(s), pi, &cause);
	if (cmds == NULL) {
		pr.status = CMD_PARSE_ERROR;
		pr.error = cause;
		return (&pr);
	}
	return (cmd_parse_build_commands(cmds, pi));
}

struct cmd_parse_result *
cmd_parse_from_arguments(int argc, char **argv, struct cmd_parse_input *pi)
{
	struct cmd_parse_input		  input;
	struct cmd_parse_commands	 *cmds;
	struct cmd_parse_command	 *cmd;
	char				**copy, **new_argv;
	size_t				  size;
	int				  i, last, new_argc;

	/*
	 * The commands are already split up into arguments, so just separate
	 * into a set of commands by ';'.
	 */

	if (pi == NULL) {
		memset(&input, 0, sizeof input);
		pi = &input;
	}
	cmd_log_argv(argc, argv, "%s", __func__);

	cmds = cmd_parse_new_commands();
	copy = cmd_copy_argv(argc, argv);

	last = 0;
	for (i = 0; i < argc; i++) {
		size = strlen(copy[i]);
		if (size == 0 || copy[i][size - 1] != ';')
			continue;
		copy[i][--size] = '\0';
		if (size > 0 && copy[i][size - 1] == '\\') {
			copy[i][size - 1] = ';';
			continue;
		}

		new_argc = i - last;
		new_argv = copy + last;
		if (size != 0)
			new_argc++;

		if (new_argc != 0) {
			cmd_log_argv(new_argc, new_argv, "%s: at %u", __func__,
			    i);

			cmd = xcalloc(1, sizeof *cmd);
			cmd->name = xstrdup(new_argv[0]);
			cmd->line = pi->line;

			cmd->argc = new_argc - 1;
			cmd->argv = cmd_copy_argv(new_argc - 1, new_argv + 1);

			TAILQ_INSERT_TAIL(cmds, cmd, entry);
		}

		last = i + 1;
	}
	if (last != argc) {
		new_argv = copy + last;
		new_argc = argc - last;

		if (new_argc != 0) {
			cmd_log_argv(new_argc, new_argv, "%s: at %u", __func__,
			    last);

			cmd = xcalloc(1, sizeof *cmd);
			cmd->name = xstrdup(new_argv[0]);
			cmd->line = pi->line;

			cmd->argc = new_argc - 1;
			cmd->argv = cmd_copy_argv(new_argc - 1, new_argv + 1);

			TAILQ_INSERT_TAIL(cmds, cmd, entry);
		}
	}

	cmd_free_argv(argc, copy);
	return (cmd_parse_build_commands(cmds, pi));
}

static int printflike(1, 2)
yyerror(const char *fmt, ...)
{
	struct cmd_parse_state	*ps = &parse_state;
	struct cmd_parse_input	*pi = ps->input;
	va_list			 ap;
	char			*error;

	if (ps->error != NULL)
		return (0);

	va_start(ap, fmt);
	xvasprintf(&error, fmt, ap);
	va_end(ap);

	ps->error = cmd_parse_get_error(pi->file, pi->line, error);
	free(error);
	return (0);
}

static int
yylex_is_var(char ch, int first)
{
	if (ch == '=')
		return (0);
	if (first && isdigit((u_char)ch))
		return (0);
	return (isalnum((u_char)ch) || ch == '_');
}

static void
yylex_append(char **buf, size_t *len, const char *add, size_t addlen)
{
	if (addlen > SIZE_MAX - 1 || *len > SIZE_MAX - 1 - addlen)
		fatalx("buffer is too big");
	*buf = xrealloc(*buf, (*len) + 1 + addlen);
	memcpy((*buf) + *len, add, addlen);
	(*len) += addlen;
}

static void
yylex_append1(char **buf, size_t *len, char add)
{
	yylex_append(buf, len, &add, 1);
}

static int
yylex_getc1(void)
{
	struct cmd_parse_state	*ps = &parse_state;
	int			 ch;

	if (ps->f != NULL)
		ch = getc(ps->f);
	else {
		if (ps->off == ps->len)
			ch = EOF;
		else
			ch = ps->buf[ps->off++];
	}
	return (ch);
}

static void
yylex_ungetc(int ch)
{
	struct cmd_parse_state	*ps = &parse_state;

	if (ps->f != NULL)
		ungetc(ch, ps->f);
	else if (ps->off > 0 && ch != EOF)
		ps->off--;
}

static int
yylex_getc(void)
{
	struct cmd_parse_state	*ps = &parse_state;
	int			 ch;

	if (ps->escapes != 0) {
		ps->escapes--;
		return ('\\');
	}
	for (;;) {
		ch = yylex_getc1();
		if (ch == '\\') {
			ps->escapes++;
			continue;
		}
		if (ch == '\n' && (ps->escapes % 2) == 1) {
			ps->input->line++;
			ps->escapes--;
			continue;
		}

		if (ps->escapes != 0) {
			yylex_ungetc(ch);
			ps->escapes--;
			return ('\\');
		}
		return (ch);
	}
}

static char *
yylex_get_word(int ch)
{
	char	*buf;
	size_t	 len;

	len = 0;
	buf = xmalloc(1);

	do
		yylex_append1(&buf, &len, ch);
	while ((ch = yylex_getc()) != EOF && strchr(" \t\n", ch) == NULL);
	yylex_ungetc(ch);

	buf[len] = '\0';
	log_debug("%s: %s", __func__, buf);
	return (buf);
}

static int
yylex(void)
{
	struct cmd_parse_state	*ps = &parse_state;
	char			*token, *cp;
	int			 ch, next, condition;

	if (ps->eol)
		ps->input->line++;
	ps->eol = 0;

	condition = ps->condition;
	ps->condition = 0;

	for (;;) {
		ch = yylex_getc();

		if (ch == EOF) {
			/*
			 * Ensure every file or string is terminated by a
			 * newline. This keeps the parser simpler and avoids
			 * having to add a newline to each string.
			 */
			if (ps->eof)
				break;
			ps->eof = 1;
			return ('\n');
		}

		if (ch == ' ' || ch == '\t') {
			/*
			 * Ignore whitespace.
			 */
			continue;
		}

		if (ch == '\n') {
			/*
			 * End of line. Update the line number.
			 */
			ps->eol = 1;
			return ('\n');
		}

		if (ch == ';') {
			/*
			 * A semicolon is itself.
			 */
			return (';');
		}

		if (ch == '#') {
			/*
			 * #{ after a condition opens a format; anything else
			 * is a comment, ignore up to the end of the line.
			 */
			next = yylex_getc();
			if (condition && next == '{') {
				yylval.token = yylex_format();
				if (yylval.token == NULL)
					return (ERROR);
				return (FORMAT);
			}
			while (next != '\n' && next != EOF)
				next = yylex_getc();
			if (next == '\n') {
				ps->input->line++;
				return ('\n');
			}
			continue;
		}

		if (ch == '%') {
			/*
			 * % is a condition unless it is all % or all numbers,
			 * then it is a token.
			 */
			yylval.token = yylex_get_word('%');
			for (cp = yylval.token; *cp != '\0'; cp++) {
				if (*cp != '%' && !isdigit((u_char)*cp))
					break;
			}
			if (*cp == '\0')
				return (TOKEN);
			ps->condition = 1;
			if (strcmp(yylval.token, "%if") == 0) {
				free(yylval.token);
				return (IF);
			}
			if (strcmp(yylval.token, "%else") == 0) {
				free(yylval.token);
				return (ELSE);
			}
			if (strcmp(yylval.token, "%elif") == 0) {
				free(yylval.token);
				return (ELIF);
			}
			if (strcmp(yylval.token, "%endif") == 0) {
				free(yylval.token);
				return (ENDIF);
			}
			free(yylval.token);
			return (ERROR);
		}

		/*
		 * Otherwise this is a token.
		 */
		token = yylex_token(ch);
		if (token == NULL)
			return (ERROR);
		yylval.token = token;

		if (strchr(token, '=') != NULL && yylex_is_var(*token, 1)) {
			for (cp = token + 1; *cp != '='; cp++) {
				if (!yylex_is_var(*cp, 0))
					break;
			}
			if (*cp == '=')
				return (EQUALS);
		}
		return (TOKEN);
	}
	return (0);
}

static char *
yylex_format(void)
{
	char	*buf;
	size_t	 len;
	int	 ch, brackets = 1;

	len = 0;
	buf = xmalloc(1);

	yylex_append(&buf, &len, "#{", 2);
	for (;;) {
		if ((ch = yylex_getc()) == EOF || ch == '\n')
			goto error;
		if (ch == '#') {
			if ((ch = yylex_getc()) == EOF || ch == '\n')
				goto error;
			if (ch == '{')
				brackets++;
			yylex_append1(&buf, &len, '#');
		} else if (ch == '}') {
			if (brackets != 0 && --brackets == 0) {
				yylex_append1(&buf, &len, ch);
				break;
			}
		}
		yylex_append1(&buf, &len, ch);
	}
	if (brackets != 0)
		goto error;

	buf[len] = '\0';
	log_debug("%s: %s", __func__, buf);
	return (buf);

error:
	free(buf);
	return (NULL);
}

static int
yylex_token_escape(char **buf, size_t *len)
{
	int			 ch, type, o2, o3;
	u_int			 size, i, tmp;
	char			 s[9];
	struct utf8_data	 ud;

	ch = yylex_getc();

	if (ch >= '4' && ch <= '7') {
		yyerror("invalid octal escape");
		return (0);
	}
	if (ch >= '0' && ch <= '3') {
		o2 = yylex_getc();
		if (o2 >= '0' && o2 <= '7') {
			o3 = yylex_getc();
			if (o3 >= '0' && o3 <= '7') {
				ch = 64 * (ch - '0') +
				      8 * (o2 - '0') +
				          (o3 - '0');
				yylex_append1(buf, len, ch);
				return (1);
			}
		}
		yyerror("invalid octal escape");
		return (0);
	}

	switch (ch) {
	case EOF:
		return (0);
	case 'a':
		ch = '\a';
		break;
	case 'b':
		ch = '\b';
		break;
	case 'e':
		ch = '\033';
		break;
	case 'f':
		ch = '\f';
		break;
	case 's':
		ch = ' ';
		break;
	case 'v':
		ch = '\v';
		break;
	case 'r':
		ch = '\r';
		break;
	case 'n':
		ch = '\n';
		break;
	case 't':
		ch = '\t';
		break;
	case 'u':
		type = 'u';
		size = 4;
		goto unicode;
	case 'U':
		type = 'U';
		size = 8;
		goto unicode;
	}

	yylex_append1(buf, len, ch);
	return (1);

unicode:
	for (i = 0; i < size; i++) {
		ch = yylex_getc();
		if (ch == EOF || ch == '\n')
			return (0);
		if (!isxdigit((u_char)ch)) {
			yyerror("invalid \\%c argument", type);
			return (0);
		}
		s[i] = ch;
	}
	s[i] = '\0';

	if ((size == 4 && sscanf(s, "%4x", &tmp) != 1) ||
	    (size == 8 && sscanf(s, "%8x", &tmp) != 1)) {
		yyerror("invalid \\%c argument", type);
		return (0);
	}
	if (utf8_split(tmp, &ud) != UTF8_DONE) {
		yyerror("invalid \\%c argument", type);
		return (0);
	}
	yylex_append(buf, len, ud.data, ud.size);
	return (1);
}

static int
yylex_token_variable(char **buf, size_t *len)
{
	struct environ_entry	*envent;
	int			 ch, brackets = 0;
	char			 name[1024];
	size_t			 namelen = 0;
	const char		*value;

	ch = yylex_getc();
	if (ch == EOF)
		return (0);
	if (ch == '{')
		brackets = 1;
	else {
		if (!yylex_is_var(ch, 1)) {
			yylex_append1(buf, len, '$');
			yylex_ungetc(ch);
			return (1);
		}
		name[namelen++] = ch;
	}

	for (;;) {
		ch = yylex_getc();
		if (brackets && ch == '}')
			break;
		if (ch == EOF || !yylex_is_var(ch, 0)) {
			if (!brackets) {
				yylex_ungetc(ch);
				break;
			}
			yyerror("invalid environment variable");
			return (0);
		}
		if (namelen == (sizeof name) - 2) {
			yyerror("environment variable is too long");
			return (0);
		}
		name[namelen++] = ch;
	}
	name[namelen] = '\0';

	envent = environ_find(global_environ, name);
	if (envent != NULL) {
		value = envent->value;
		log_debug("%s: %s -> %s", __func__, name, value);
		yylex_append(buf, len, value, strlen(value));
	}
	return (1);
}

static int
yylex_token_tilde(char **buf, size_t *len)
{
	struct environ_entry	*envent;
	int			 ch;
	char			 name[1024];
	size_t			 namelen = 0;
	struct passwd		*pw;
	const char		*home = NULL;

	for (;;) {
		ch = yylex_getc();
		if (ch == EOF || strchr("/ \t\n\"'", ch) != NULL) {
			yylex_ungetc(ch);
			break;
		}
		if (namelen == (sizeof name) - 2) {
			yyerror("user name is too long");
			return (0);
		}
		name[namelen++] = ch;
	}
	name[namelen] = '\0';

	if (*name == '\0') {
		envent = environ_find(global_environ, "HOME");
		if (envent != NULL && *envent->value != '\0')
			home = envent->value;
		else if ((pw = getpwuid(getuid())) != NULL)
			home = pw->pw_dir;
	} else {
		if ((pw = getpwnam(name)) != NULL)
			home = pw->pw_dir;
	}
	if (home == NULL)
		return (0);

	log_debug("%s: ~%s -> %s", __func__, name, home);
	yylex_append(buf, len, home, strlen(home));
	return (1);
}

static int
yylex_token_brace(char **buf, size_t *len)
{
	struct cmd_parse_state	*ps = &parse_state;
	int 			 ch, lines = 0, nesting = 1, escape = 0;
	int			 quote = '\0', token = 0;

	/*
	 * Extract a string up to the matching unquoted '}', including newlines
	 * and handling nested braces.
	 *
	 * To detect the final and intermediate braces which affect the nesting
	 * depth, we scan the input as if it was a tmux config file, and ignore
	 * braces which would be considered quoted, escaped, or in a comment.
	 *
	 * We update the token state after every character because '#' begins a
	 * comment only when it begins a token. For simplicity, we treat an
	 * unquoted directive format as comment.
	 *
	 * The result is verbatim copy of the input excluding the final brace.
	 */

	for (ch = yylex_getc1(); ch != EOF; ch = yylex_getc1()) {
		yylex_append1(buf, len, ch);
		if (ch == '\n')
			lines++;

		/*
		 * If the previous character was a backslash (escape is set),
		 * escape anything if unquoted or in double quotes, otherwise
		 * escape only '\n' and '\\'.
		 */
		if (escape &&
		    (quote == '\0' ||
		    quote == '"' ||
		    ch == '\n' ||
		    ch == '\\')) {
			escape = 0;
			if (ch != '\n')
				token = 1;
			continue;
		}

		/*
		 * The character is not escaped. If it is a backslash, set the
		 * escape flag.
		 */
		if (ch == '\\') {
			escape = 1;
			continue;
		}
		escape = 0;

		/* A newline always resets to unquoted. */
		if (ch == '\n') {
			quote = token = 0;
			continue;
		}

		if (quote) {
			/*
			 * Inside quotes or comment. Check if this is the
			 * closing quote.
			 */
			if (ch == quote && quote != '#')
				quote = 0;
			token = 1;  /* token continues regardless */
		} else {
			/* Not inside quotes or comment. */
			switch (ch) {
			case '"':
			case '\'':
			case '#':
				/* Beginning of quote or maybe comment. */
				if (ch != '#' || !token)
					quote = ch;
				token = 1;
				break;
			case ' ':
			case '\t':
			case ';':
				/* Delimiter - token resets. */
				token = 0;
				break;
			case '{':
				nesting++;
				token = 0; /* new commands set - token resets */
				break;
			case '}':
				nesting--;
				token = 1;  /* same as after quotes */
				if (nesting == 0) {
					(*len)--; /* remove closing } */
					ps->input->line += lines;
					return (1);
				}
				break;
			default:
				token = 1;
				break;
			}
		}
	}

	/*
	 * Update line count after error as reporting the opening line is more
	 * useful than EOF.
	 */
	yyerror("unterminated brace string");
	ps->input->line += lines;
	return (0);
}

static char *
yylex_token(int ch)
{
	char			*buf;
	size_t			 len;
	enum { START,
	       NONE,
	       DOUBLE_QUOTES,
	       SINGLE_QUOTES }	 state = NONE, last = START;

	len = 0;
	buf = xmalloc(1);

	for (;;) {
		/*
		 * EOF or \n are always the end of the token. If inside quotes
		 * they are an error.
		 */
		if (ch == EOF || ch == '\n') {
			if (state != NONE)
				goto error;
			break;
		}

		/* Whitespace or ; ends a token unless inside quotes. */
		if ((ch == ' ' || ch == '\t' || ch == ';') && state == NONE)
			break;

		/*
		 * \ ~ and $ are expanded except in single quotes.
		 */
		if (ch == '\\' && state != SINGLE_QUOTES) {
			if (!yylex_token_escape(&buf, &len))
				goto error;
			goto skip;
		}
		if (ch == '~' && last != state && state != SINGLE_QUOTES) {
			if (!yylex_token_tilde(&buf, &len))
				goto error;
			goto skip;
		}
		if (ch == '$' && state != SINGLE_QUOTES) {
			if (!yylex_token_variable(&buf, &len))
				goto error;
			goto skip;
		}
		if (ch == '{' && state == NONE) {
			if (!yylex_token_brace(&buf, &len))
				goto error;
			goto skip;
		}
		if (ch == '}' && state == NONE)
			goto error;  /* unmatched (matched ones were handled) */

		/*
		 * ' and " starts or end quotes (and is consumed).
		 */
		if (ch == '\'') {
			if (state == NONE) {
				state = SINGLE_QUOTES;
				goto next;
			}
			if (state == SINGLE_QUOTES) {
				state = NONE;
				goto next;
			}
		}
		if (ch == '"') {
			if (state == NONE) {
				state = DOUBLE_QUOTES;
				goto next;
			}
			if (state == DOUBLE_QUOTES) {
				state = NONE;
				goto next;
			}
		}

		/*
		 * Otherwise add the character to the buffer.
		 */
		yylex_append1(&buf, &len, ch);

	skip:
		last = state;

	next:
		ch = yylex_getc();
	}
	yylex_ungetc(ch);

	buf[len] = '\0';
	log_debug("%s: %s", __func__, buf);
	return (buf);

error:
	free(buf);
	return (NULL);
}
#line 1350 "cmd-parse.c"
/* allocate initial stack or double stack size, up to YYMAXDEPTH */
static int yygrowstack(void)
{
    unsigned int newsize;
    long sslen;
    short *newss;
    YYSTYPE *newvs;

    if ((newsize = yystacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return -1;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;
    sslen = yyssp - yyss;
#ifdef SIZE_MAX
#define YY_SIZE_MAX SIZE_MAX
#else
#define YY_SIZE_MAX 0xffffffffU
#endif
    if (newsize && YY_SIZE_MAX / newsize < sizeof *newss)
        goto bail;
    newss = yyss ? (short *)realloc(yyss, newsize * sizeof *newss) :
      (short *)malloc(newsize * sizeof *newss); /* overflow check above */
    if (newss == NULL)
        goto bail;
    yyss = newss;
    yyssp = newss + sslen;
    if (newsize && YY_SIZE_MAX / newsize < sizeof *newvs)
        goto bail;
    newvs = yyvs ? (YYSTYPE *)realloc(yyvs, newsize * sizeof *newvs) :
      (YYSTYPE *)malloc(newsize * sizeof *newvs); /* overflow check above */
    if (newvs == NULL)
        goto bail;
    yyvs = newvs;
    yyvsp = newvs + sslen;
    yystacksize = newsize;
    yysslim = yyss + newsize - 1;
    return 0;
bail:
    if (yyss)
            free(yyss);
    if (yyvs)
            free(yyvs);
    yyss = yyssp = NULL;
    yyvs = yyvsp = NULL;
    yystacksize = 0;
    return -1;
}

#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab
int
yyparse(void)
{
    int yym, yyn, yystate;
#if YYDEBUG
    const char *yys;

    if ((yys = getenv("YYDEBUG")))
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif /* YYDEBUG */

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    if (yyss == NULL && yygrowstack()) goto yyoverflow;
    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if ((yyn = yydefred[yystate]) != 0) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yyssp >= yysslim && yygrowstack())
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = (-1);
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
#if defined(__GNUC__)
    goto yynewerror;
#endif
yynewerror:
    yyerror("syntax error");
#if defined(__GNUC__)
    goto yyerrlab;
#endif
yyerrlab:
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yysslim && yygrowstack())
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    if (yym)
        yyval = yyvsp[1-yym];
    else
        memset(&yyval, 0, sizeof yyval);
    switch (yyn)
    {
case 2:
#line 119 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;

			ps->commands = yyvsp[0].commands;
		}
break;
case 3:
#line 126 "cmd-parse.y"
{
			yyval.commands = yyvsp[-1].commands;
		}
break;
case 4:
#line 130 "cmd-parse.y"
{
			yyval.commands = yyvsp[-2].commands;
			TAILQ_CONCAT(yyval.commands, yyvsp[-1].commands, entry);
			free(yyvsp[-1].commands);
		}
break;
case 5:
#line 137 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;

			if (ps->scope == NULL || ps->scope->flag)
				yyval.commands = yyvsp[0].commands;
			else {
				yyval.commands = cmd_parse_new_commands();
				cmd_parse_free_commands(yyvsp[0].commands);
			}
		}
break;
case 6:
#line 148 "cmd-parse.y"
{
			yyval.commands = xmalloc (sizeof *yyval.commands);
			TAILQ_INIT(yyval.commands);
		}
break;
case 7:
#line 153 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;

			if (ps->scope == NULL || ps->scope->flag)
				yyval.commands = yyvsp[0].commands;
			else {
				yyval.commands = cmd_parse_new_commands();
				cmd_parse_free_commands(yyvsp[0].commands);
			}
		}
break;
case 8:
#line 165 "cmd-parse.y"
{
			yyval.token = yyvsp[0].token;
		}
break;
case 9:
#line 169 "cmd-parse.y"
{
			yyval.token = yyvsp[0].token;
		}
break;
case 10:
#line 174 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;
			struct cmd_parse_input	*pi = ps->input;
			struct format_tree	*ft;
			struct client		*c = pi->c;
			struct cmd_find_state	*fs;
			int			 flags = FORMAT_NOJOBS;

			if (cmd_find_valid_state(&pi->fs))
				fs = &pi->fs;
			else
				fs = NULL;
			ft = format_create(NULL, pi->item, FORMAT_NONE, flags);
			if (fs != NULL)
				format_defaults(ft, c, fs->s, fs->wl, fs->wp);
			else
				format_defaults(ft, c, NULL, NULL, NULL);

			yyval.token = format_expand(ft, yyvsp[0].token);
			format_free(ft);
			free(yyvsp[0].token);
		}
break;
case 12:
#line 199 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;
			int			 flags = ps->input->flags;

			if ((~flags & CMD_PARSE_PARSEONLY) &&
			    (ps->scope == NULL || ps->scope->flag))
				environ_put(global_environ, yyvsp[0].token);
			free(yyvsp[0].token);
		}
break;
case 13:
#line 210 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;
			struct cmd_parse_scope	*scope;

			scope = xmalloc(sizeof *scope);
			yyval.flag = scope->flag = format_true(yyvsp[0].token);
			free(yyvsp[0].token);

			if (ps->scope != NULL)
				TAILQ_INSERT_HEAD(&ps->stack, ps->scope, entry);
			ps->scope = scope;
		}
break;
case 14:
#line 224 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;
			struct cmd_parse_scope	*scope;

			scope = xmalloc(sizeof *scope);
			scope->flag = !ps->scope->flag;

			free(ps->scope);
			ps->scope = scope;
		}
break;
case 15:
#line 236 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;
			struct cmd_parse_scope	*scope;

			scope = xmalloc(sizeof *scope);
			yyval.flag = scope->flag = format_true(yyvsp[0].token);
			free(yyvsp[0].token);

			free(ps->scope);
			ps->scope = scope;
		}
break;
case 16:
#line 249 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;

			free(ps->scope);
			ps->scope = TAILQ_FIRST(&ps->stack);
			if (ps->scope != NULL)
				TAILQ_REMOVE(&ps->stack, ps->scope, entry);
		}
break;
case 17:
#line 259 "cmd-parse.y"
{
			if (yyvsp[-3].flag)
				yyval.commands = yyvsp[-1].commands;
			else {
				yyval.commands = cmd_parse_new_commands();
				cmd_parse_free_commands(yyvsp[-1].commands);
			}
		}
break;
case 18:
#line 268 "cmd-parse.y"
{
			if (yyvsp[-6].flag) {
				yyval.commands = yyvsp[-4].commands;
				cmd_parse_free_commands(yyvsp[-1].commands);
			} else {
				yyval.commands = yyvsp[-1].commands;
				cmd_parse_free_commands(yyvsp[-4].commands);
			}
		}
break;
case 19:
#line 278 "cmd-parse.y"
{
			if (yyvsp[-4].flag) {
				yyval.commands = yyvsp[-2].commands;
				cmd_parse_free_commands(yyvsp[-1].elif.commands);
			} else if (yyvsp[-1].elif.flag) {
				yyval.commands = yyvsp[-1].elif.commands;
				cmd_parse_free_commands(yyvsp[-2].commands);
			} else {
				yyval.commands = cmd_parse_new_commands();
				cmd_parse_free_commands(yyvsp[-2].commands);
				cmd_parse_free_commands(yyvsp[-1].elif.commands);
			}
		}
break;
case 20:
#line 292 "cmd-parse.y"
{
			if (yyvsp[-7].flag) {
				yyval.commands = yyvsp[-5].commands;
				cmd_parse_free_commands(yyvsp[-4].elif.commands);
				cmd_parse_free_commands(yyvsp[-1].commands);
			} else if (yyvsp[-4].elif.flag) {
				yyval.commands = yyvsp[-4].elif.commands;
				cmd_parse_free_commands(yyvsp[-5].commands);
				cmd_parse_free_commands(yyvsp[-1].commands);
			} else {
				yyval.commands = yyvsp[-1].commands;
				cmd_parse_free_commands(yyvsp[-5].commands);
				cmd_parse_free_commands(yyvsp[-4].elif.commands);
			}
		}
break;
case 21:
#line 309 "cmd-parse.y"
{
			if (yyvsp[-2].flag) {
				yyval.elif.flag = 1;
				yyval.elif.commands = yyvsp[0].commands;
			} else {
				yyval.elif.flag = 0;
				yyval.elif.commands = cmd_parse_new_commands();
				cmd_parse_free_commands(yyvsp[0].commands);
			}
		}
break;
case 22:
#line 320 "cmd-parse.y"
{
			if (yyvsp[-3].flag) {
				yyval.elif.flag = 1;
				yyval.elif.commands = yyvsp[-1].commands;
				cmd_parse_free_commands(yyvsp[0].elif.commands);
			} else if (yyvsp[0].elif.flag) {
				yyval.elif.flag = 1;
				yyval.elif.commands = yyvsp[0].elif.commands;
				cmd_parse_free_commands(yyvsp[-1].commands);
			} else {
				yyval.elif.flag = 0;
				yyval.elif.commands = cmd_parse_new_commands();
				cmd_parse_free_commands(yyvsp[-1].commands);
				cmd_parse_free_commands(yyvsp[0].elif.commands);
			}
		}
break;
case 23:
#line 338 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;

			yyval.commands = cmd_parse_new_commands();
			if (ps->scope == NULL || ps->scope->flag)
				TAILQ_INSERT_TAIL(yyval.commands, yyvsp[0].command, entry);
			else
				cmd_parse_free_command(yyvsp[0].command);
		}
break;
case 24:
#line 348 "cmd-parse.y"
{
			yyval.commands = yyvsp[-1].commands;
		}
break;
case 25:
#line 352 "cmd-parse.y"
{
			yyval.commands = yyvsp[-2].commands;
			TAILQ_CONCAT(yyval.commands, yyvsp[0].commands, entry);
			free(yyvsp[0].commands);
		}
break;
case 26:
#line 358 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;

			if (ps->scope == NULL || ps->scope->flag) {
				yyval.commands = yyvsp[-2].commands;
				TAILQ_INSERT_TAIL(yyval.commands, yyvsp[0].command, entry);
			} else {
				yyval.commands = cmd_parse_new_commands();
				cmd_parse_free_commands(yyvsp[-2].commands);
				cmd_parse_free_command(yyvsp[0].command);
			}
		}
break;
case 27:
#line 371 "cmd-parse.y"
{
			yyval.commands = yyvsp[0].commands;
		}
break;
case 28:
#line 376 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;

			yyval.command = xcalloc(1, sizeof *yyval.command);
			yyval.command->name = yyvsp[0].token;
			yyval.command->line = ps->input->line;

		}
break;
case 29:
#line 385 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;

			yyval.command = xcalloc(1, sizeof *yyval.command);
			yyval.command->name = yyvsp[-1].token;
			yyval.command->line = ps->input->line;

			yyval.command->argc = yyvsp[0].arguments.argc;
			yyval.command->argv = yyvsp[0].arguments.argv;
		}
break;
case 30:
#line 397 "cmd-parse.y"
{
			if (yyvsp[-2].flag)
				yyval.commands = yyvsp[-1].commands;
			else {
				yyval.commands = cmd_parse_new_commands();
				cmd_parse_free_commands(yyvsp[-1].commands);
			}
		}
break;
case 31:
#line 406 "cmd-parse.y"
{
			if (yyvsp[-4].flag) {
				yyval.commands = yyvsp[-3].commands;
				cmd_parse_free_commands(yyvsp[-1].commands);
			} else {
				yyval.commands = yyvsp[-1].commands;
				cmd_parse_free_commands(yyvsp[-3].commands);
			}
		}
break;
case 32:
#line 416 "cmd-parse.y"
{
			if (yyvsp[-3].flag) {
				yyval.commands = yyvsp[-2].commands;
				cmd_parse_free_commands(yyvsp[-1].elif.commands);
			} else if (yyvsp[-1].elif.flag) {
				yyval.commands = yyvsp[-1].elif.commands;
				cmd_parse_free_commands(yyvsp[-2].commands);
			} else {
				yyval.commands = cmd_parse_new_commands();
				cmd_parse_free_commands(yyvsp[-2].commands);
				cmd_parse_free_commands(yyvsp[-1].elif.commands);
			}
		}
break;
case 33:
#line 430 "cmd-parse.y"
{
			if (yyvsp[-5].flag) {
				yyval.commands = yyvsp[-4].commands;
				cmd_parse_free_commands(yyvsp[-3].elif.commands);
				cmd_parse_free_commands(yyvsp[-1].commands);
			} else if (yyvsp[-3].elif.flag) {
				yyval.commands = yyvsp[-3].elif.commands;
				cmd_parse_free_commands(yyvsp[-4].commands);
				cmd_parse_free_commands(yyvsp[-1].commands);
			} else {
				yyval.commands = yyvsp[-1].commands;
				cmd_parse_free_commands(yyvsp[-4].commands);
				cmd_parse_free_commands(yyvsp[-3].elif.commands);
			}
		}
break;
case 34:
#line 447 "cmd-parse.y"
{
			if (yyvsp[-1].flag) {
				yyval.elif.flag = 1;
				yyval.elif.commands = yyvsp[0].commands;
			} else {
				yyval.elif.flag = 0;
				yyval.elif.commands = cmd_parse_new_commands();
				cmd_parse_free_commands(yyvsp[0].commands);
			}
		}
break;
case 35:
#line 458 "cmd-parse.y"
{
			if (yyvsp[-2].flag) {
				yyval.elif.flag = 1;
				yyval.elif.commands = yyvsp[-1].commands;
				cmd_parse_free_commands(yyvsp[0].elif.commands);
			} else if (yyvsp[0].elif.flag) {
				yyval.elif.flag = 1;
				yyval.elif.commands = yyvsp[0].elif.commands;
				cmd_parse_free_commands(yyvsp[-1].commands);
			} else {
				yyval.elif.flag = 0;
				yyval.elif.commands = cmd_parse_new_commands();
				cmd_parse_free_commands(yyvsp[-1].commands);
				cmd_parse_free_commands(yyvsp[0].elif.commands);
			}
		}
break;
case 36:
#line 476 "cmd-parse.y"
{
			yyval.arguments.argc = 1;
			yyval.arguments.argv = xreallocarray(NULL, 1, sizeof *yyval.arguments.argv);

			yyval.arguments.argv[0] = yyvsp[0].token;
		}
break;
case 37:
#line 483 "cmd-parse.y"
{
			cmd_prepend_argv(&yyvsp[0].arguments.argc, &yyvsp[0].arguments.argv, yyvsp[-1].token);
			free(yyvsp[-1].token);
			yyval.arguments = yyvsp[0].arguments;
		}
break;
case 38:
#line 490 "cmd-parse.y"
{
			yyval.token = yyvsp[0].token;
		}
break;
case 39:
#line 494 "cmd-parse.y"
{
			yyval.token = yyvsp[0].token;
		}
break;
#line 1979 "cmd-parse.c"
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if (yyssp >= yysslim && yygrowstack())
    {
        goto yyoverflow;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    if (yyss)
            free(yyss);
    if (yyvs)
            free(yyvs);
    yyss = yyssp = NULL;
    yyvs = yyvsp = NULL;
    yystacksize = 0;
    return (1);
yyaccept:
    if (yyss)
            free(yyss);
    if (yyvs)
            free(yyvs);
    yyss = yyssp = NULL;
    yyvs = yyvsp = NULL;
    yystacksize = 0;
    return (0);
}
