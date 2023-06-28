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

enum cmd_parse_argument_type {
	CMD_PARSE_STRING,
	CMD_PARSE_COMMANDS,
	CMD_PARSE_PARSED_COMMANDS
};

struct cmd_parse_argument {
	enum cmd_parse_argument_type	 type;
	char				*string;
	struct cmd_parse_commands	*commands;
	struct cmd_list			*cmdlist;

	TAILQ_ENTRY(cmd_parse_argument)	 entry;
};
TAILQ_HEAD(cmd_parse_arguments, cmd_parse_argument);

struct cmd_parse_command {
	u_int				 line;
	struct cmd_parse_arguments	 arguments;

	TAILQ_ENTRY(cmd_parse_command)	 entry;
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
static void	 cmd_parse_build_commands(struct cmd_parse_commands *,
		     struct cmd_parse_input *, struct cmd_parse_result *);
static void	 cmd_parse_print_commands(struct cmd_parse_input *,
		     struct cmd_list *);

#line 101 "cmd-parse.y"
#ifndef YYSTYPE_DEFINED
#define YYSTYPE_DEFINED
typedef union
{
	char					 *token;
	struct cmd_parse_arguments		 *arguments;
	struct cmd_parse_argument		 *argument;
	int					  flag;
	struct {
		int				  flag;
		struct cmd_parse_commands	 *commands;
	} elif;
	struct cmd_parse_commands		 *commands;
	struct cmd_parse_command		 *command;
} YYSTYPE;
#endif /* YYSTYPE_DEFINED */
#line 110 "cmd-parse.c"
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
    0,    0,   10,   10,   11,   11,   11,   11,    2,    2,
    1,   17,   17,   18,   16,    5,   19,    6,   20,   13,
   13,   13,   13,    7,    7,   12,   12,   12,   12,   12,
   15,   15,   15,   14,   14,   14,   14,    8,    8,    3,
    3,    4,    4,    4,    9,    9,
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
   33,    0,    0,    0,    0,   20,   18,    0,    0,   36,
    0,   44,    0,    0,   41,    0,    0,   22,    0,   39,
    0,   35,    0,   45,    0,    0,    0,   37,   46,   25,
    0,   21,   23,
};
const short yydgoto[] =
	{                                       4,
   18,   19,   41,   42,    5,   31,   44,   32,   52,    6,
    7,    8,    9,   10,   11,   12,   13,   14,   33,   34,
};
const short yysindex[] =
	{                                   -175,
 -227, -172,    0,    0,  -10, -175,   46,   10,    0,    0,
    0,    0, -205,    0,    0,    0,    0,    0,    0, -175,
 -238,  -56,   53,    0, -238, -118, -228,    0, -172,    0,
 -238, -234, -238,    0,    0,    0,    0,    0,    0, -175,
    0, -118,   63, -234,   66,    0,    0,  -52, -238,    0,
  -55,    0, -175,    3,    0, -175,   68,    0, -175,    0,
  -55,    0,    4,    0, -208, -175, -219,    0,    0,    0,
 -219,    0,    0,};
const short yyrindex[] =
	{                                      1,
    0,    0,    0,    0, -184,    2,    0,    5,    0,    0,
    0,    0,    0,   -4,    0,    0,    0,    0,    0,    6,
 -184,    0,    0,    0,    7,   12,    6,    0,    0,    0,
 -184,    0, -184,    0,    0,    0,    0,    0,    0,   -2,
    0,   15,    0,    0,    0,    0,    0, -215, -184,    0,
    0,    0,   -2,    0,    0,    6,    0,    0,    6,    0,
    0,    0,    0,    0,   -1,    6,    6,    0,    0,    0,
    6,    0,    0,};
const short yygindex[] =
	{                                      0,
   57,    0,   40,    0,   39,  -17,   28,   47,    0,    9,
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
   67,   42,  258,  259,   71,   29,   31,   49,   33,  265,
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
#line 575 "cmd-parse.y"

static char *
cmd_parse_get_error(const char *file, u_int line, const char *error)
{
	char	*s;

	if (file == NULL)
		s = xstrdup(error);
	else
		xasprintf(&s, "%s:%u: %s", file, line, error);
	return (s);
}

static void
cmd_parse_print_commands(struct cmd_parse_input *pi, struct cmd_list *cmdlist)
{
	char	*s;

	if (pi->item == NULL || (~pi->flags & CMD_PARSE_VERBOSE))
		return;
	s = cmd_list_print(cmdlist, 0);
	if (pi->file != NULL)
		cmdq_print(pi->item, "%s:%u: %s", pi->file, pi->line, s);
	else
		cmdq_print(pi->item, "%u: %s", pi->line, s);
	free(s);
}

static void
cmd_parse_free_argument(struct cmd_parse_argument *arg)
{
	switch (arg->type) {
	case CMD_PARSE_STRING:
		free(arg->string);
		break;
	case CMD_PARSE_COMMANDS:
		cmd_parse_free_commands(arg->commands);
		break;
	case CMD_PARSE_PARSED_COMMANDS:
		cmd_list_free(arg->cmdlist);
		break;
	}
	free(arg);
}

static void
cmd_parse_free_arguments(struct cmd_parse_arguments *args)
{
	struct cmd_parse_argument	*arg, *arg1;

	TAILQ_FOREACH_SAFE(arg, args, entry, arg1) {
		TAILQ_REMOVE(args, arg, entry);
		cmd_parse_free_argument(arg);
	}
}

static void
cmd_parse_free_command(struct cmd_parse_command *cmd)
{
	cmd_parse_free_arguments(&cmd->arguments);
	free(cmd);
}

static struct cmd_parse_commands *
cmd_parse_new_commands(void)
{
	struct cmd_parse_commands	*cmds;

	cmds = xmalloc(sizeof *cmds);
	TAILQ_INIT(cmds);
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

static void
cmd_parse_log_commands(struct cmd_parse_commands *cmds, const char *prefix)
{
	struct cmd_parse_command	*cmd;
	struct cmd_parse_argument	*arg;
	u_int				 i, j;
	char				*s;

	i = 0;
	TAILQ_FOREACH(cmd, cmds, entry) {
		j = 0;
		TAILQ_FOREACH(arg, &cmd->arguments, entry) {
			switch (arg->type) {
			case CMD_PARSE_STRING:
				log_debug("%s %u:%u: %s", prefix, i, j,
				    arg->string);
				break;
			case CMD_PARSE_COMMANDS:
				xasprintf(&s, "%s %u:%u", prefix, i, j);
				cmd_parse_log_commands(arg->commands, s);
				free(s);
				break;
			case CMD_PARSE_PARSED_COMMANDS:
				s = cmd_list_print(arg->cmdlist, 0);
				log_debug("%s %u:%u: %s", prefix, i, j, s);
				free(s);
				break;
			}
			j++;
		}
		i++;
	}
}

static int
cmd_parse_expand_alias(struct cmd_parse_command *cmd,
    struct cmd_parse_input *pi, struct cmd_parse_result *pr)
{
	struct cmd_parse_argument	*arg, *arg1, *first;
	struct cmd_parse_commands	*cmds;
	struct cmd_parse_command	*last;
	char				*alias, *name, *cause;

	if (pi->flags & CMD_PARSE_NOALIAS)
		return (0);
	memset(pr, 0, sizeof *pr);

	first = TAILQ_FIRST(&cmd->arguments);
	if (first == NULL || first->type != CMD_PARSE_STRING) {
		pr->status = CMD_PARSE_SUCCESS;
		pr->cmdlist = cmd_list_new();
		return (1);
	}
	name = first->string;

	alias = cmd_get_alias(name);
	if (alias == NULL)
		return (0);
	log_debug("%s: %u alias %s = %s", __func__, pi->line, name, alias);

	cmds = cmd_parse_do_buffer(alias, strlen(alias), pi, &cause);
	free(alias);
	if (cmds == NULL) {
		pr->status = CMD_PARSE_ERROR;
		pr->error = cause;
		return (1);
	}

	last = TAILQ_LAST(cmds, cmd_parse_commands);
	if (last == NULL) {
		pr->status = CMD_PARSE_SUCCESS;
		pr->cmdlist = cmd_list_new();
		return (1);
	}

	TAILQ_REMOVE(&cmd->arguments, first, entry);
	cmd_parse_free_argument(first);

	TAILQ_FOREACH_SAFE(arg, &cmd->arguments, entry, arg1) {
		TAILQ_REMOVE(&cmd->arguments, arg, entry);
		TAILQ_INSERT_TAIL(&last->arguments, arg, entry);
	}
	cmd_parse_log_commands(cmds, __func__);

	pi->flags |= CMD_PARSE_NOALIAS;
	cmd_parse_build_commands(cmds, pi, pr);
	pi->flags &= ~CMD_PARSE_NOALIAS;
	return (1);
}

static void
cmd_parse_build_command(struct cmd_parse_command *cmd,
    struct cmd_parse_input *pi, struct cmd_parse_result *pr)
{
	struct cmd_parse_argument	*arg;
	struct cmd			*add;
	char				*cause;
	struct args_value		*values = NULL;
	u_int				 count = 0, idx;

	memset(pr, 0, sizeof *pr);

	if (cmd_parse_expand_alias(cmd, pi, pr))
		return;

	TAILQ_FOREACH(arg, &cmd->arguments, entry) {
		values = xrecallocarray(values, count, count + 1,
		    sizeof *values);
		switch (arg->type) {
		case CMD_PARSE_STRING:
			values[count].type = ARGS_STRING;
			values[count].string = xstrdup(arg->string);
			break;
		case CMD_PARSE_COMMANDS:
			cmd_parse_build_commands(arg->commands, pi, pr);
			if (pr->status != CMD_PARSE_SUCCESS)
				goto out;
			values[count].type = ARGS_COMMANDS;
			values[count].cmdlist = pr->cmdlist;
			break;
		case CMD_PARSE_PARSED_COMMANDS:
			values[count].type = ARGS_COMMANDS;
			values[count].cmdlist = arg->cmdlist;
			values[count].cmdlist->references++;
			break;
		}
		count++;
	}

	add = cmd_parse(values, count, pi->file, pi->line, &cause);
	if (add == NULL) {
		pr->status = CMD_PARSE_ERROR;
		pr->error = cmd_parse_get_error(pi->file, pi->line, cause);
		free(cause);
		goto out;
	}
	pr->status = CMD_PARSE_SUCCESS;
	pr->cmdlist = cmd_list_new();
	cmd_list_append(pr->cmdlist, add);

out:
	for (idx = 0; idx < count; idx++)
		args_free_value(&values[idx]);
	free(values);
}

static void
cmd_parse_build_commands(struct cmd_parse_commands *cmds,
    struct cmd_parse_input *pi, struct cmd_parse_result *pr)
{
	struct cmd_parse_command	*cmd;
	u_int				 line = UINT_MAX;
	struct cmd_list			*current = NULL, *result;
	char				*s;

	memset(pr, 0, sizeof *pr);

	/* Check for an empty list. */
	if (TAILQ_EMPTY(cmds)) {
		pr->status = CMD_PARSE_SUCCESS;
		pr->cmdlist = cmd_list_new();
		return;
	}
	cmd_parse_log_commands(cmds, __func__);

	/*
	 * Parse each command into a command list. Create a new command list
	 * for each line (unless the flag is set) so they get a new group (so
	 * the queue knows which ones to remove if a command fails when
	 * executed).
	 */
	result = cmd_list_new();
	TAILQ_FOREACH(cmd, cmds, entry) {
		if (((~pi->flags & CMD_PARSE_ONEGROUP) && cmd->line != line)) {
			if (current != NULL) {
				cmd_parse_print_commands(pi, current);
				cmd_list_move(result, current);
				cmd_list_free(current);
			}
			current = cmd_list_new();
		}
		if (current == NULL)
			current = cmd_list_new();
		line = pi->line = cmd->line;

		cmd_parse_build_command(cmd, pi, pr);
		if (pr->status != CMD_PARSE_SUCCESS) {
			cmd_list_free(result);
			cmd_list_free(current);
			return;
		}
		cmd_list_append_all(current, pr->cmdlist);
		cmd_list_free(pr->cmdlist);
	}
	if (current != NULL) {
		cmd_parse_print_commands(pi, current);
		cmd_list_move(result, current);
		cmd_list_free(current);
	}

	s = cmd_list_print(result, 0);
	log_debug("%s: %s", __func__, s);
	free(s);

	pr->status = CMD_PARSE_SUCCESS;
	pr->cmdlist = result;
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
	cmd_parse_build_commands(cmds, pi, &pr);
	cmd_parse_free_commands(cmds);
	return (&pr);

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
		pr.status = CMD_PARSE_SUCCESS;
		pr.cmdlist = cmd_list_new();
		return (&pr);
	}

	cmds = cmd_parse_do_buffer(buf, len, pi, &cause);
	if (cmds == NULL) {
		pr.status = CMD_PARSE_ERROR;
		pr.error = cause;
		return (&pr);
	}
	cmd_parse_build_commands(cmds, pi, &pr);
	cmd_parse_free_commands(cmds);
	return (&pr);
}

struct cmd_parse_result *
cmd_parse_from_arguments(struct args_value *values, u_int count,
    struct cmd_parse_input *pi)
{
	static struct cmd_parse_result	 pr;
	struct cmd_parse_input		 input;
	struct cmd_parse_commands	*cmds;
	struct cmd_parse_command	*cmd;
	struct cmd_parse_argument	*arg;
	u_int				 i;
	char				*copy;
	size_t				 size;
	int				 end;

	/*
	 * The commands are already split up into arguments, so just separate
	 * into a set of commands by ';'.
	 */

	if (pi == NULL) {
		memset(&input, 0, sizeof input);
		pi = &input;
	}
	memset(&pr, 0, sizeof pr);

	cmds = cmd_parse_new_commands();

	cmd = xcalloc(1, sizeof *cmd);
	cmd->line = pi->line;
	TAILQ_INIT(&cmd->arguments);

	for (i = 0; i < count; i++) {
		end = 0;
		if (values[i].type == ARGS_STRING) {
			copy = xstrdup(values[i].string);
			size = strlen(copy);
			if (size != 0 && copy[size - 1] == ';') {
				copy[--size] = '\0';
				if (size > 0 && copy[size - 1] == '\\')
					copy[size - 1] = ';';
				else
					end = 1;
			}
			if (!end || size != 0) {
				arg = xcalloc(1, sizeof *arg);
				arg->type = CMD_PARSE_STRING;
				arg->string = copy;
				TAILQ_INSERT_TAIL(&cmd->arguments, arg, entry);
			}
		} else if (values[i].type == ARGS_COMMANDS) {
			arg = xcalloc(1, sizeof *arg);
			arg->type = CMD_PARSE_PARSED_COMMANDS;
			arg->cmdlist = values[i].cmdlist;
			arg->cmdlist->references++;
			TAILQ_INSERT_TAIL(&cmd->arguments, arg, entry);
		} else
			fatalx("unknown argument type");
		if (end) {
			TAILQ_INSERT_TAIL(cmds, cmd, entry);
			cmd = xcalloc(1, sizeof *cmd);
			cmd->line = pi->line;
			TAILQ_INIT(&cmd->arguments);
		}
	}
	if (!TAILQ_EMPTY(&cmd->arguments))
		TAILQ_INSERT_TAIL(cmds, cmd, entry);
	else
		free(cmd);

	cmd_parse_build_commands(cmds, pi, &pr);
	cmd_parse_free_commands(cmds);
	return (&pr);
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
#line 1458 "cmd-parse.c"
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
#line 136 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;

			ps->commands = yyvsp[0].commands;
		}
break;
case 3:
#line 143 "cmd-parse.y"
{
			yyval.commands = yyvsp[-1].commands;
		}
break;
case 4:
#line 147 "cmd-parse.y"
{
			yyval.commands = yyvsp[-2].commands;
			TAILQ_CONCAT(yyval.commands, yyvsp[-1].commands, entry);
			free(yyvsp[-1].commands);
		}
break;
case 5:
#line 154 "cmd-parse.y"
{
			yyval.commands = xmalloc (sizeof *yyval.commands);
			TAILQ_INIT(yyval.commands);
		}
break;
case 6:
#line 159 "cmd-parse.y"
{
			yyval.commands = xmalloc (sizeof *yyval.commands);
			TAILQ_INIT(yyval.commands);
		}
break;
case 7:
#line 164 "cmd-parse.y"
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
#line 175 "cmd-parse.y"
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
#line 187 "cmd-parse.y"
{
			yyval.token = yyvsp[0].token;
		}
break;
case 10:
#line 191 "cmd-parse.y"
{
			yyval.token = yyvsp[0].token;
		}
break;
case 11:
#line 196 "cmd-parse.y"
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
#line 223 "cmd-parse.y"
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
#line 234 "cmd-parse.y"
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
#line 245 "cmd-parse.y"
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
#line 259 "cmd-parse.y"
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
#line 271 "cmd-parse.y"
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
#line 284 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;

			free(ps->scope);
			ps->scope = TAILQ_FIRST(&ps->stack);
			if (ps->scope != NULL)
				TAILQ_REMOVE(&ps->stack, ps->scope, entry);
		}
break;
case 20:
#line 294 "cmd-parse.y"
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
#line 303 "cmd-parse.y"
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
#line 313 "cmd-parse.y"
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
#line 327 "cmd-parse.y"
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
#line 344 "cmd-parse.y"
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
#line 355 "cmd-parse.y"
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
#line 373 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;

			yyval.commands = cmd_parse_new_commands();
			if (!TAILQ_EMPTY(&yyvsp[0].command->arguments) &&
			    (ps->scope == NULL || ps->scope->flag))
				TAILQ_INSERT_TAIL(yyval.commands, yyvsp[0].command, entry);
			else
				cmd_parse_free_command(yyvsp[0].command);
		}
break;
case 27:
#line 384 "cmd-parse.y"
{
			yyval.commands = yyvsp[-1].commands;
		}
break;
case 28:
#line 388 "cmd-parse.y"
{
			yyval.commands = yyvsp[-2].commands;
			TAILQ_CONCAT(yyval.commands, yyvsp[0].commands, entry);
			free(yyvsp[0].commands);
		}
break;
case 29:
#line 394 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;

			if (!TAILQ_EMPTY(&yyvsp[0].command->arguments) &&
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
#line 408 "cmd-parse.y"
{
			yyval.commands = yyvsp[0].commands;
		}
break;
case 31:
#line 413 "cmd-parse.y"
{
			struct cmd_parse_state	*ps = &parse_state;

			yyval.command = xcalloc(1, sizeof *yyval.command);
			yyval.command->line = ps->input->line;
			TAILQ_INIT(&yyval.command->arguments);
		}
break;
case 32:
#line 421 "cmd-parse.y"
{
			struct cmd_parse_state		*ps = &parse_state;
			struct cmd_parse_argument	*arg;

			yyval.command = xcalloc(1, sizeof *yyval.command);
			yyval.command->line = ps->input->line;
			TAILQ_INIT(&yyval.command->arguments);

			arg = xcalloc(1, sizeof *arg);
			arg->type = CMD_PARSE_STRING;
			arg->string = yyvsp[0].token;
			TAILQ_INSERT_HEAD(&yyval.command->arguments, arg, entry);
		}
break;
case 33:
#line 435 "cmd-parse.y"
{
			struct cmd_parse_state		*ps = &parse_state;
			struct cmd_parse_argument	*arg;

			yyval.command = xcalloc(1, sizeof *yyval.command);
			yyval.command->line = ps->input->line;
			TAILQ_INIT(&yyval.command->arguments);

			TAILQ_CONCAT(&yyval.command->arguments, yyvsp[0].arguments, entry);
			free(yyvsp[0].arguments);

			arg = xcalloc(1, sizeof *arg);
			arg->type = CMD_PARSE_STRING;
			arg->string = yyvsp[-1].token;
			TAILQ_INSERT_HEAD(&yyval.command->arguments, arg, entry);
		}
break;
case 34:
#line 453 "cmd-parse.y"
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
#line 462 "cmd-parse.y"
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
#line 472 "cmd-parse.y"
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
#line 486 "cmd-parse.y"
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
#line 503 "cmd-parse.y"
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
#line 514 "cmd-parse.y"
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
#line 532 "cmd-parse.y"
{
			yyval.arguments = xcalloc(1, sizeof *yyval.arguments);
			TAILQ_INIT(yyval.arguments);

			TAILQ_INSERT_HEAD(yyval.arguments, yyvsp[0].argument, entry);
		}
break;
case 41:
#line 539 "cmd-parse.y"
{
			TAILQ_INSERT_HEAD(yyvsp[0].arguments, yyvsp[-1].argument, entry);
			yyval.arguments = yyvsp[0].arguments;
		}
break;
case 42:
#line 545 "cmd-parse.y"
{
			yyval.argument = xcalloc(1, sizeof *yyval.argument);
			yyval.argument->type = CMD_PARSE_STRING;
			yyval.argument->string = yyvsp[0].token;
		}
break;
case 43:
#line 551 "cmd-parse.y"
{
			yyval.argument = xcalloc(1, sizeof *yyval.argument);
			yyval.argument->type = CMD_PARSE_STRING;
			yyval.argument->string = yyvsp[0].token;
		}
break;
case 44:
#line 557 "cmd-parse.y"
{
			yyval.argument = xcalloc(1, sizeof *yyval.argument);
			yyval.argument->type = CMD_PARSE_COMMANDS;
			yyval.argument->commands = yyvsp[0].commands;
		}
break;
case 45:
#line 564 "cmd-parse.y"
{
				yyval.commands = yyvsp[-1].commands;
			}
break;
case 46:
#line 568 "cmd-parse.y"
{
				yyval.commands = yyvsp[-2].commands;
				TAILQ_CONCAT(yyval.commands, yyvsp[-1].commands, entry);
				free(yyvsp[-1].commands);
			}
break;
#line 2152 "cmd-parse.c"
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
