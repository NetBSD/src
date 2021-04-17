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
#include <wchar.h>

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
static char	*cmd_parse_commands_to_string(struct cmd_parse_commands *);
static void	 cmd_parse_print_commands(struct cmd_parse_input *, u_int,
		     struct cmd_list *);

#line 86 "cmd-parse.y"
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
#line 97 "cmd-parse.c"
#define ERROR 257
#define HIDDEN 258
#define IF 259
#define ELSE 260
#define ELIF 261
#define ENDIF 262
#define FORMAT 263
#define TOKEN 264
#define EQUALS 265
#define YYERRCODE 256
const short yylhs[] =
	{                                        -1,
    0,    0,   10,   10,   11,   11,   11,   11,    3,    3,
    2,   17,   17,   18,   16,    5,   19,    6,   20,   13,
   13,   13,   13,    7,    7,   12,   12,   12,   12,   12,
   15,   15,   15,   14,   14,   14,   14,    8,    8,    4,
    4,    1,    1,    1,    9,    9,
};
const short yylen[] =
	{                                         2,
    0,    1,    2,    3,    0,    1,    1,    1,    1,    1,
    1,    0,    1,    1,    2,    2,    1,    2,    1,    4,
    7,    5,    8,    3,    4,    1,    2,    3,    3,    1,
    1,    2,    3,    3,    5,    4,    6,    2,    3,    1,
    2,    1,    1,    2,    2,    3,
};
const short yydefred[] =
	{                                      0,
    0,    0,   14,    0,    0,    0,    0,    0,    7,   30,
   26,    6,    0,    0,   15,    9,   10,   16,   11,    0,
    0,    0,    0,    3,    0,    0,    0,   17,    0,   19,
    0,    0,    0,   34,    4,   28,   29,   42,   43,    0,
    0,   33,    0,    0,    0,   20,   18,    0,    0,   36,
    0,   44,    0,    0,   41,    0,    0,   22,    0,   39,
    0,   35,    0,   45,    0,    0,    0,   37,   46,   25,
    0,   21,   23,
};
const short yydgoto[] =
	{                                       4,
   41,   18,   19,   42,    5,   31,   44,   32,   52,    6,
    7,    8,    9,   10,   11,   12,   13,   14,   33,   34,
};
const short yysindex[] =
	{                                   -175,
 -227, -172,    0,    0,  -10, -175,   46,   10,    0,    0,
    0,    0, -205,    0,    0,    0,    0,    0,    0, -175,
 -238,  -56,   53,    0, -238, -118, -228,    0, -172,    0,
 -238, -234, -238,    0,    0,    0,    0,    0,    0, -175,
 -118,    0,   63, -234,   66,    0,    0,  -52, -238,    0,
  -55,    0, -175,    3,    0, -175,   68,    0, -175,    0,
  -55,    0,    4,    0, -208, -175, -219,    0,    0,    0,
 -219,    0,    0,};
const short yyrindex[] =
	{                                      1,
    0,    0,    0,    0, -184,    2,    0,    5,    0,    0,
    0,    0,    0,   -4,    0,    0,    0,    0,    0,    6,
 -184,    0,    0,    0,    7,   12,    6,    0,    0,    0,
 -184,    0, -184,    0,    0,    0,    0,    0,    0,   -2,
   15,    0,    0,    0,    0,    0,    0, -215, -184,    0,
    0,    0,   -2,    0,    0,    6,    0,    0,    6,    0,
    0,    0,    0,    0,   -1,    6,    6,    0,    0,    0,
    6,    0,    0,};
const short yygindex[] =
	{                                      0,
    0,   57,    0,   41,   39,  -17,   28,   47,    0,    9,
   14,   56,    0,   69,   71,    0,    0,    0,   -8,   -9,
};
#define YYTABLESIZE 277
const short yytable[] =
	{                                      20,
    1,    2,   25,   25,   40,   31,   25,    5,    5,   43,
    5,    5,   24,   35,    8,    5,   27,   46,   45,   23,
    2,   32,   50,   49,   40,   28,    3,   30,   27,    1,
    2,   28,   29,   30,   58,   57,    3,   15,    1,    2,
   23,   62,   30,   21,   38,    3,   38,   43,   53,    1,
    2,   68,   29,   54,   31,   24,    3,   72,   26,   21,
   22,   73,   35,   21,   65,   27,   63,   67,   25,   21,
   32,   21,   56,   40,   71,   59,   22,   66,   23,   12,
   23,   55,    1,    2,   23,   47,   48,   21,   51,    3,
   16,   17,   70,   36,   60,   37,    0,    0,    0,    0,
    0,    0,    0,    0,   61,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   31,    0,    5,    0,    0,    0,    0,   64,   69,    8,
    0,   27,    0,    0,    0,    0,   32,    0,    0,   40,
    0,    0,    0,    0,    0,   38,   39,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   28,   29,   30,   30,    0,   29,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    2,    0,
    0,    0,    0,    0,    3,   31,   31,   31,   24,   13,
   24,   12,   12,    0,   12,   12,   27,   27,   27,   12,
   12,   32,   32,   32,   40,   40,   40,
};
const short yycheck[] =
	{                                      10,
    0,    0,   59,   59,  123,   10,   59,   10,   10,   27,
   10,   10,   10,   10,   10,   10,   10,   27,   27,    6,
  259,   10,   32,   32,   10,  260,  265,  262,   20,  258,
  259,  260,  261,  262,   44,   44,  265,  265,  258,  259,
   27,   51,  262,    5,  260,  265,  262,   65,   40,  258,
  259,   61,  261,   40,   59,   10,  265,   67,  264,   21,
    5,   71,   10,   25,   56,   59,   53,   59,   59,   31,
   59,   33,   10,   59,   66,   10,   21,   10,   65,  264,
   67,   41,  258,  259,   71,   29,   31,   49,   33,  265,
  263,  264,   65,   25,   48,   25,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   49,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
  125,   -1,  125,   -1,   -1,   -1,   -1,  125,  125,  125,
   -1,  125,   -1,   -1,   -1,   -1,  125,   -1,   -1,  125,
   -1,   -1,   -1,   -1,   -1,  264,  265,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,  260,  261,  262,  262,   -1,  261,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  259,   -1,
   -1,   -1,   -1,   -1,  265,  260,  261,  262,  260,  264,
  262,  264,  264,   -1,  264,  264,  260,  261,  262,  264,
  264,  260,  261,  262,  260,  261,  262,
};
#define YYFINAL 4
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 265
#if YYDEBUG
const char * const yyname[] =
	{
"end-of-file",0,0,0,0,0,0,0,0,0,"'\\n'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"';'",0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'{'",0,"'}'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"ERROR",
"HIDDEN","IF","ELSE","ELIF","ENDIF","FORMAT","TOKEN","EQUALS",
};
const char * const yyrule[] =
	{"$accept : lines",
"lines :",
"lines : statements",
"statements : statement '\\n'",
"statements : statements statement '\\n'",
"statement :",
"statement : hidden_assignment",
"statement : condition",
"statement : commands",
"format : FORMAT",
"format : TOKEN",
"expanded : format",
"optional_assignment :",
"optional_assignment : assignment",
"assignment : EQUALS",
"hidden_assignment : HIDDEN EQUALS",
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
"command : assignment",
"command : optional_assignment TOKEN",
"command : optional_assignment TOKEN arguments",
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
"argument : '{' argument_statements",
"argument_statements : statement '}'",
"argument_statements : statements statement '}'",
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
#line 546 "cmd-parse.y"

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

static char *
cmd_parse_commands_to_string(struct cmd_parse_commands *cmds)
{
	struct cmd_parse_command	 *cmd;
	char				 *string = NULL, *s, *line;

	TAILQ_FOREACH(cmd, cmds, entry) {
		line = cmd_stringify_argv(cmd->argc, cmd->argv);
		if (string == NULL)
			s = line;
		else {
			xasprintf(&s, "%s ; %s", s, line);
			free(line);
		}

		free(string);
		string = s;
	}
	if (string == NULL)
		string = xstrdup("");
	log_debug("%s: %s", __func__, string);
	return (string);
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
	char				*name, *alias, *cause, *s;

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
		name = cmd->argv[0];

		alias = cmd_get_alias(name);
		if (alias == NULL)
			continue;

		line = cmd->line;
		log_debug("%s: %u %s = %s", __func__, line, name, alias);

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
		for (i = 1; i < cmd->argc; i++)
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
	 * for each line (unless the flag is set) so they get a new group (so
	 * the queue knows which ones to remove if a command fails when
	 * executed).
	 */
	result = cmd_list_new();
	TAILQ_FOREACH(cmd, cmds, entry) {
		name = cmd->argv[0];
		log_debug("%s: %u %s", __func__, cmd->line, name);
		cmd_log_argv(cmd->argc, cmd->argv, __func__);

		if (cmdlist == NULL ||
		    ((~pi->flags & CMD_PARSE_ONEGROUP) && cmd->line != line)) {
			if (cmdlist != NULL) {
				cmd_parse_print_commands(pi, line, cmdlist);
				cmd_list_move(result, cmdlist);
				cmd_list_free(cmdlist);
			}
			cmdlist = cmd_list_new();
		}
		line = cmd->line;

		add = cmd_parse(cmd->argc, cmd->argv, pi->file, line, &cause);
		if (add == NULL) {
			cmd_list_free(result);
			pr.status = CMD_PARSE_ERROR;
			pr.error = cmd_parse_get_error(pi->file, line, cause);
			free(cause);
			cmd_list_free(cmdlist);
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
	struct cmd_parse_input	input;

	if (pi == NULL) {
		memset(&input, 0, sizeof input);
		pi = &input;
	}

	/*
	 * When parsing a string, put commands in one group even if there are
	 * multiple lines. This means { a \n b } is identical to "a ; b" when
	 * given as an argument to another command.
	 */
	pi->flags |= CMD_PARSE_ONEGROUP;
	return (cmd_parse_from_buffer(s, strlen(s), pi));
}

enum cmd_parse_status
cmd_parse_and_insert(const char *s, struct cmd_parse_input *pi,
    struct cmdq_item *after, struct cmdq_state *state, char **error)
{
	struct cmd_parse_result	*pr;
	struct cmdq_item	*item;

	pr = cmd_parse_from_string(s, pi);
	switch (pr->status) {
	case CMD_PARSE_EMPTY:
		break;
	case CMD_PARSE_ERROR:
		if (error != NULL)
			*error = pr->error;
		else
			free(pr->error);
		break;
	case CMD_PARSE_SUCCESS:
		item = cmdq_get_command(pr->cmdlist, state);
		cmdq_insert_after(after, item);
		cmd_list_free(pr->cmdlist);
		break;
	}
	return (pr->status);
}

enum cmd_parse_status
cmd_parse_and_append(const char *s, struct cmd_parse_input *pi,
    struct client *c, struct cmdq_state *state, char **error)
{
	struct cmd_parse_result	*pr;
	struct cmdq_item	*item;

	pr = cmd_parse_from_string(s, pi);
	switch (pr->status) {
	case CMD_PARSE_EMPTY:
		break;
	case CMD_PARSE_ERROR:
		if (error != NULL)
			*error = pr->error;
		else
			free(pr->error);
		break;
	case CMD_PARSE_SUCCESS:
		item = cmdq_get_command(pr->cmdlist, state);
		cmdq_append(c, item);
		cmd_list_free(pr->cmdlist);
		break;
	}
	return (pr->status);
}

struct cmd_parse_result *
cmd_parse_from_buffer(const void *buf, size_t len, struct cmd_parse_input *pi)
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

	if (len == 0) {
		pr.status = CMD_PARSE_EMPTY;
		pr.cmdlist = NULL;
		pr.error = NULL;
		return (&pr);
	}

	cmds = cmd_parse_do_buffer(buf, len, pi, &cause);
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
			cmd->line = pi->line;

			cmd->argc = new_argc;
			cmd->argv = cmd_copy_argv(new_argc, new_argv);

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
			cmd->line = pi->line;

			cmd->argc = new_argc;
			cmd->argv = cmd_copy_argv(new_argc, new_argv);

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

		if (ch == ';' || ch == '{' || ch == '}') {
			/*
			 * A semicolon or { or } is itself.
			 */
			return (ch);
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
			if (strcmp(yylval.token, "%hidden") == 0) {
				free(yylval.token);
				return (HIDDEN);
			}
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
	int	 ch, type, o2, o3, mlen;
	u_int	 size, i, tmp;
	char	 s[9], m[MB_LEN_MAX];

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
	mlen = wctomb(m, tmp);
	if (mlen <= 0 || mlen > (int)sizeof m) {
		yyerror("invalid \\%c argument", type);
		return (0);
	}
	yylex_append(buf, len, m, mlen);
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
	if (envent != NULL && envent->value != NULL) {
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
		/* EOF or \n are always the end of the token. */
		if (ch == EOF || (state == NONE && ch == '\n'))
			break;

		/* Whitespace or ; or } ends a token unless inside quotes. */
		if ((ch == ' ' || ch == '\t' || ch == ';' || ch == '}') &&
		    state == NONE)
			break;

		/*
		 * Spaces and comments inside quotes after \n are removed but
		 * the \n is left.
		 */
		if (ch == '\n' && state != NONE) {
			yylex_append1(&buf, &len, '\n');
			while ((ch = yylex_getc()) == ' ' || ch == '\t')
				/* nothing */;
			if (ch != '#')
				continue;
			ch = yylex_getc();
			if (strchr(",#{}:", ch) != NULL) {
				yylex_ungetc(ch);
				ch = '#';
			} else {
				while ((ch = yylex_getc()) != '\n' && ch != EOF)
					/* nothing */;
			}
			continue;
		}

		/* \ ~ and $ are expanded except in single quotes. */
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
		if (ch == '}' && state == NONE)
			goto error;  /* unmatched (matched ones were handled) */

		/* ' and " starts or end quotes (and is consumed). */
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

		/* Otherwise add the character to the buffer. */
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
#line 1357 "cmd-parse.c"
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
    newss = (short *)realloc(yyss, newsize * sizeof *newss);
    if (newss == NULL)
        goto bail;
    yyss = newss;
    yyssp = newss + sslen;
    if (newsize && YY_SIZE_MAX / newsize < sizeof *newvs)
        goto bail;
    newvs = (YYSTYPE *)realloc(yyvs, newsize * sizeof *newvs);
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
#line 122 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;

			ps->commands = yyvsp[0].commands;
		}
break;
case 3:
#line 129 "cmd-parse.y"
{
			yyval.commands = yyvsp[-1].commands;
		}
break;
case 4:
#line 133 "cmd-parse.y"
{
			yyval.commands = yyvsp[-2].commands;
			TAILQ_CONCAT(yyval.commands, yyvsp[-1].commands, entry);
			free(yyvsp[-1].commands);
		}
break;
case 5:
#line 140 "cmd-parse.y"
{
			yyval.commands = xmalloc (sizeof *yyval.commands);
			TAILQ_INIT(yyval.commands);
		}
break;
case 6:
#line 145 "cmd-parse.y"
{
			yyval.commands = xmalloc (sizeof *yyval.commands);
			TAILQ_INIT(yyval.commands);
		}
break;
case 7:
#line 150 "cmd-parse.y"
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
#line 161 "cmd-parse.y"
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
case 9:
#line 173 "cmd-parse.y"
{
			yyval.token = yyvsp[0].token;
		}
break;
case 10:
#line 177 "cmd-parse.y"
{
			yyval.token = yyvsp[0].token;
		}
break;
case 11:
#line 182 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;
			struct cmd_parse_input	*pi = ps->input;
			struct format_tree	*ft;
			struct client		*c = pi->c;
			struct cmd_find_state	*fsp;
			struct cmd_find_state	 fs;
			int			 flags = FORMAT_NOJOBS;

			if (cmd_find_valid_state(&pi->fs))
				fsp = &pi->fs;
			else {
				cmd_find_from_client(&fs, c, 0);
				fsp = &fs;
			}
			ft = format_create(NULL, pi->item, FORMAT_NONE, flags);
			format_defaults(ft, c, fsp->s, fsp->wl, fsp->wp);

			yyval.token = format_expand(ft, yyvsp[0].token);
			format_free(ft);
			free(yyvsp[0].token);
		}
break;
case 14:
#line 209 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;
			int			 flags = ps->input->flags;

			if ((~flags & CMD_PARSE_PARSEONLY) &&
			    (ps->scope == NULL || ps->scope->flag))
				environ_put(global_environ, yyvsp[0].token, 0);
			free(yyvsp[0].token);
		}
break;
case 15:
#line 220 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;
			int			 flags = ps->input->flags;

			if ((~flags & CMD_PARSE_PARSEONLY) &&
			    (ps->scope == NULL || ps->scope->flag))
				environ_put(global_environ, yyvsp[0].token, ENVIRON_HIDDEN);
			free(yyvsp[0].token);
		}
break;
case 16:
#line 231 "cmd-parse.y"
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
case 17:
#line 245 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;
			struct cmd_parse_scope	*scope;

			scope = xmalloc(sizeof *scope);
			scope->flag = !ps->scope->flag;

			free(ps->scope);
			ps->scope = scope;
		}
break;
case 18:
#line 257 "cmd-parse.y"
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
case 19:
#line 270 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;

			free(ps->scope);
			ps->scope = TAILQ_FIRST(&ps->stack);
			if (ps->scope != NULL)
				TAILQ_REMOVE(&ps->stack, ps->scope, entry);
		}
break;
case 20:
#line 280 "cmd-parse.y"
{
			if (yyvsp[-3].flag)
				yyval.commands = yyvsp[-1].commands;
			else {
				yyval.commands = cmd_parse_new_commands();
				cmd_parse_free_commands(yyvsp[-1].commands);
			}
		}
break;
case 21:
#line 289 "cmd-parse.y"
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
case 22:
#line 299 "cmd-parse.y"
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
case 23:
#line 313 "cmd-parse.y"
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
case 24:
#line 330 "cmd-parse.y"
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
case 25:
#line 341 "cmd-parse.y"
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
case 26:
#line 359 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;

			yyval.commands = cmd_parse_new_commands();
			if (yyvsp[0].command->argc != 0 &&
			    (ps->scope == NULL || ps->scope->flag))
				TAILQ_INSERT_TAIL(yyval.commands, yyvsp[0].command, entry);
			else
				cmd_parse_free_command(yyvsp[0].command);
		}
break;
case 27:
#line 370 "cmd-parse.y"
{
			yyval.commands = yyvsp[-1].commands;
		}
break;
case 28:
#line 374 "cmd-parse.y"
{
			yyval.commands = yyvsp[-2].commands;
			TAILQ_CONCAT(yyval.commands, yyvsp[0].commands, entry);
			free(yyvsp[0].commands);
		}
break;
case 29:
#line 380 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;

			if (yyvsp[0].command->argc != 0 &&
			    (ps->scope == NULL || ps->scope->flag)) {
				yyval.commands = yyvsp[-2].commands;
				TAILQ_INSERT_TAIL(yyval.commands, yyvsp[0].command, entry);
			} else {
				yyval.commands = cmd_parse_new_commands();
				cmd_parse_free_commands(yyvsp[-2].commands);
				cmd_parse_free_command(yyvsp[0].command);
			}
		}
break;
case 30:
#line 394 "cmd-parse.y"
{
			yyval.commands = yyvsp[0].commands;
		}
break;
case 31:
#line 399 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;

			yyval.command = xcalloc(1, sizeof *yyval.command);
			yyval.command->line = ps->input->line;
		}
break;
case 32:
#line 406 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;

			yyval.command = xcalloc(1, sizeof *yyval.command);
			yyval.command->line = ps->input->line;

			cmd_prepend_argv(&yyval.command->argc, &yyval.command->argv, yyvsp[0].token);

		}
break;
case 33:
#line 416 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;

			yyval.command = xcalloc(1, sizeof *yyval.command);
			yyval.command->line = ps->input->line;

			yyval.command->argc = yyvsp[0].arguments.argc;
			yyval.command->argv = yyvsp[0].arguments.argv;
			cmd_prepend_argv(&yyval.command->argc, &yyval.command->argv, yyvsp[-1].token);
		}
break;
case 34:
#line 428 "cmd-parse.y"
{
			if (yyvsp[-2].flag)
				yyval.commands = yyvsp[-1].commands;
			else {
				yyval.commands = cmd_parse_new_commands();
				cmd_parse_free_commands(yyvsp[-1].commands);
			}
		}
break;
case 35:
#line 437 "cmd-parse.y"
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
case 36:
#line 447 "cmd-parse.y"
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
case 37:
#line 461 "cmd-parse.y"
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
case 38:
#line 478 "cmd-parse.y"
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
case 39:
#line 489 "cmd-parse.y"
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
case 40:
#line 507 "cmd-parse.y"
{
			yyval.arguments.argc = 1;
			yyval.arguments.argv = xreallocarray(NULL, 1, sizeof *yyval.arguments.argv);

			yyval.arguments.argv[0] = yyvsp[0].token;
		}
break;
case 41:
#line 514 "cmd-parse.y"
{
			cmd_prepend_argv(&yyvsp[0].arguments.argc, &yyvsp[0].arguments.argv, yyvsp[-1].token);
			free(yyvsp[-1].token);
			yyval.arguments = yyvsp[0].arguments;
		}
break;
case 42:
#line 521 "cmd-parse.y"
{
			yyval.token = yyvsp[0].token;
		}
break;
case 43:
#line 525 "cmd-parse.y"
{
			yyval.token = yyvsp[0].token;
		}
break;
case 44:
#line 529 "cmd-parse.y"
{
			yyval.token = cmd_parse_commands_to_string(yyvsp[0].commands);
			cmd_parse_free_commands(yyvsp[0].commands);
		}
break;
case 45:
#line 535 "cmd-parse.y"
{
				yyval.commands = yyvsp[-1].commands;
			}
break;
case 46:
#line 539 "cmd-parse.y"
{
				yyval.commands = yyvsp[-2].commands;
				TAILQ_CONCAT(yyval.commands, yyvsp[-1].commands, entry);
				free(yyvsp[-1].commands);
			}
break;
#line 2036 "cmd-parse.c"
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
