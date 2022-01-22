/* 	$NetBSD: refuse_opt.c,v 1.23 2022/01/22 07:58:32 pho Exp $	*/

/*-
 * Copyright (c) 2007 Juan Romero Pardines.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include <sys/types.h>

#include <err.h>
#include <fuse_internal.h>
#include <fuse_opt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef FUSE_OPT_DEBUG
#define DPRINTF(x)	do { printf x; } while ( /* CONSTCOND */ 0)
#else
#define DPRINTF(x)
#endif

/*
 * Public API.
 */

/* ARGSUSED */
int
fuse_opt_add_arg(struct fuse_args *args, const char *arg)
{
	struct fuse_args	*ap;

	if (args->allocated == 0) {
		ap = fuse_opt_deep_copy_args(args->argc, args->argv);
		args->argv = ap->argv;
		args->argc = ap->argc;
		args->allocated = ap->allocated;
		(void) free(ap);
	} else if (args->allocated == args->argc) {
		int na = args->allocated + 10;

		if (reallocarr(&args->argv, (size_t)na, sizeof(*args->argv)) != 0)
			return -1;

		args->allocated = na;
	}
	DPRINTF(("%s: arguments passed: [arg:%s]\n", __func__, arg));
	if ((args->argv[args->argc++] = strdup(arg)) == NULL)
		err(1, "fuse_opt_add_arg");
	args->argv[args->argc] = NULL;
        return 0;
}

struct fuse_args *
fuse_opt_deep_copy_args(int argc, char **argv)
{
	struct fuse_args	*ap;
	int			 i;

	if ((ap = malloc(sizeof(*ap))) == NULL)
		err(1, "_fuse_deep_copy_args");
	/* deep copy args structure into channel args */
	ap->allocated = ((argc / 10) + 1) * 10;

	if ((ap->argv = calloc((size_t)ap->allocated,
	    sizeof(*ap->argv))) == NULL)
		err(1, "_fuse_deep_copy_args");

	for (i = 0; i < argc; i++) {
		if ((ap->argv[i] = strdup(argv[i])) == NULL)
			err(1, "_fuse_deep_copy_args");
	}
	ap->argv[ap->argc = i] = NULL;
	return ap;
}

void
fuse_opt_free_args(struct fuse_args *ap)
{
	if (ap) {
		if (ap->allocated) {
			int	i;
			for (i = 0; i < ap->argc; i++) {
				free(ap->argv[i]);
			}
			free(ap->argv);
		}
		ap->argv = NULL;
		ap->allocated = ap->argc = 0;
	}
}

/* ARGSUSED */
int
fuse_opt_insert_arg(struct fuse_args *args, int pos, const char *arg)
{
	int	i;
	int	na;

	DPRINTF(("%s: arguments passed: [pos=%d] [arg=%s]\n",
	    __func__, pos, arg));
	if (args->argv == NULL) {
		na = 10;
	} else {
		na = args->allocated + 10;
	}
	if (reallocarr(&args->argv, (size_t)na, sizeof(*args->argv)) != 0) {
		warn("fuse_opt_insert_arg");
		return -1;
	}
	args->allocated = na;

	for (i = args->argc++; i > pos; --i) {
		args->argv[i] = args->argv[i - 1];
	}
	if ((args->argv[pos] = strdup(arg)) == NULL)
		err(1, "fuse_opt_insert_arg");
	args->argv[args->argc] = NULL;
	return 0;
}

static int add_opt(char **opts, const char *opt, bool escape)
{
	const size_t orig_len = *opts == NULL ? 0 : strlen(*opts);
	char* buf = realloc(*opts, orig_len + 1 + strlen(opt) * 2 + 1);

	if (buf == NULL) {
		return -1;
	}
	*opts = buf;

	if (orig_len > 0) {
		buf += orig_len;
		*buf++ = ',';
	}

	for (; *opt; opt++) {
		if (escape && (*opt == ',' || *opt == '\\')) {
			*buf++ = '\\';
		}
		*buf++ = *opt;
	}
	*buf = '\0';

	return 0;
}

int fuse_opt_add_opt(char **opts, const char *opt)
{
	DPRINTF(("%s: arguments passed: [opts=%s] [opt=%s]\n",
	    __func__, *opts, opt));
	return add_opt(opts, opt, false);
}

int fuse_opt_add_opt_escaped(char **opts, const char *opt)
{
	DPRINTF(("%s: arguments passed: [opts=%s] [opt=%s]\n",
	    __func__, *opts, opt));
	return add_opt(opts, opt, true);
}

static bool match_templ(const char *templ, const char *opt, ssize_t *sep_idx)
{
	const char *sep = strpbrk(templ, "= ");

	if (sep != NULL && (sep[1] == '\0' || sep[1] == '%')) {
		const size_t cmp_len =
			(size_t)(sep[0] == '=' ? sep - templ + 1 : sep - templ);

		if (strlen(opt) >= cmp_len && strncmp(templ, opt, cmp_len) == 0) {
			if (sep_idx != NULL)
				*sep_idx = (ssize_t)(sep - templ);
			return true;
		}
		else {
			return false;
		}
	}
	else {
		if (strcmp(templ, opt) == 0) {
			if (sep_idx != NULL)
				*sep_idx = -1;
			return true;
		}
		else {
			return false;
		}
	}
}

static const struct fuse_opt *
find_opt(const struct fuse_opt *opts, const char *opt, ssize_t *sep_idx)
{
	for (; opts != NULL && opts->templ != NULL; opts++) {
		if (match_templ(opts->templ, opt, sep_idx))
			return opts;
	}
	return NULL;
}

/*
 * Returns 1 if opt was matched with any option from opts,
 * otherwise returns 0.
 */
int
fuse_opt_match(const struct fuse_opt *opts, const char *opt)
{
	return find_opt(opts, opt, NULL) != NULL ? 1 : 0;
}

static int call_proc(fuse_opt_proc_t proc, void* data,
		const char* arg, int key, struct fuse_args *outargs, bool is_opt)
{
	if (key == FUSE_OPT_KEY_DISCARD)
		return 0;

	if (key != FUSE_OPT_KEY_KEEP && proc != NULL) {
		const int rv = proc(data, arg, key, outargs);

		if (rv == -1 || /* error   */
			rv ==  0    /* discard */)
			return rv;
	}

	if (is_opt) {
		/* Do we already have "-o" at the beginning of outargs? */
		if (outargs->argc >= 3 && strcmp(outargs->argv[1], "-o") == 0) {
			/* Append the option to the comma-separated list. */
			if (fuse_opt_add_opt_escaped(&outargs->argv[2], arg) == -1)
				return -1;
		}
		else {
			/* Insert -o arg at the beginning. */
			if (fuse_opt_insert_arg(outargs, 1, "-o") == -1)
				return -1;
			if (fuse_opt_insert_arg(outargs, 2, arg) == -1)
				return -1;
		}
	}
	else {
		if (fuse_opt_add_arg(outargs, arg) == -1)
			return -1;
	}

	return 0;
}

/* Skip the current argv if possible. */
static int next_arg(const struct fuse_args *args, int *i)
{
	if (*i + 1 >= args->argc) {
		(void)fprintf(stderr, "fuse: missing argument"
				" after '%s'\n", args->argv[*i]);
		return -1;
	}
	else {
		*i += 1;
		return 0;
	}
}

/* Parse a single argument with a matched template. */
static int
parse_matched_arg(const char* arg, struct fuse_args *outargs,
		const struct fuse_opt* opt, ssize_t sep_idx, void* data,
		fuse_opt_proc_t proc, bool is_opt)
{
	if (opt->offset == -1) {
		/* The option description does not want any variables to be
		 * updated.*/
		if (call_proc(proc, data, arg, opt->value, outargs, is_opt) == -1)
			return -1;
	}
	else {
		void *var = (char*)data + opt->offset;

		if (sep_idx > 0 && opt->templ[sep_idx + 1] == '%') {
			/* "foo=%y" or "-x %y" */
			const char* param =
				opt->templ[sep_idx] == '=' ? &arg[sep_idx + 1] : &arg[sep_idx];

			if (opt->templ[sep_idx + 2] == 's') {
				char* dup = strdup(param);
				if (dup == NULL)
					return -1;

				*(char **)var = dup;
			}
			else {
				/* The format string is not a literal. We all know
				 * this is a bad idea but it's exactly what fuse_opt
				 * wants to do... */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
				if (sscanf(param, &opt->templ[sep_idx + 1], var) == -1) {
#pragma GCC diagnostic pop
					(void)fprintf(stderr, "fuse: '%s' is not a "
								"valid parameter for option '%.*s'\n",
								param, (int)sep_idx, opt->templ);
					return -1;
				}
			}
		}
		else {
			/* No parameter format is given. */
			*(int *)var = opt->value;
		}
	}
	return 0;
}

/* Parse a single argument with matching templates. */
static int
parse_arg(struct fuse_args* args, int *argi, const char* arg,
		struct fuse_args *outargs, void *data,
		const struct fuse_opt *opts, fuse_opt_proc_t proc, bool is_opt)
{
	ssize_t sep_idx;
	const struct fuse_opt *opt = find_opt(opts, arg, &sep_idx);

	if (opt) {
		/* An argument can match to multiple templates. Process them
		 * all. */
		for (; opt != NULL && opt->templ != NULL;
			opt = find_opt(++opt, arg, &sep_idx)) {

			if (sep_idx > 0 && opt->templ[sep_idx] == ' ' &&
				arg[sep_idx] == '\0') {
				/* The template "-x %y" requests a separate
				 * parameter "%y". Try to find one. */
				char *new_arg;
				int rv;

				if (next_arg(args, argi) == -1)
					return -1;

				/* ...but processor callbacks expect a concatenated
				 * argument "-xfoo". */
				if ((new_arg = malloc((size_t)sep_idx +
									strlen(args->argv[*argi]) + 1)) == NULL)
					return -1;

				strncpy(new_arg, arg, (size_t)sep_idx); /* -x */
				strcpy(new_arg + sep_idx, args->argv[*argi]); /* foo */
				rv = parse_matched_arg(new_arg, outargs, opt, sep_idx,
									data, proc, is_opt);
				free(new_arg);

				if (rv == -1)
					return -1;
			}
			else {
				int rv;
				rv = parse_matched_arg(arg, outargs, opt, sep_idx,
									data, proc, is_opt);
				if (rv == -1)
					return -1;
			}
		}
		return 0;
	}
	else {
		/* No templates matched to it so just invoke the callback. */
		return call_proc(proc, data, arg, FUSE_OPT_KEY_OPT, outargs, is_opt);
	}
}

/* Parse a comma-separated list of options which possibly has escaped
 * characters. */
static int
parse_opts(struct fuse_args *args, int *argi, const char* arg,
		struct fuse_args *outargs, void *data,
		const struct fuse_opt *opts, fuse_opt_proc_t proc)
{
	char *opt;
	size_t i, opt_len = 0;

	/* An unescaped option can never be longer than the original
	 * list. */
	if ((opt = malloc(strlen(arg) + 1)) == NULL)
		return -1;

	for (i = 0; i < strlen(arg); i++) {
		if (arg[i] == ',') {
			opt[opt_len] = '\0';
			if (parse_arg(args, argi, opt, outargs,
						data, opts, proc, true) == -1) {
				free(opt);
				return -1;
			}
			/* Start a new option. */
			opt_len = 0;
		}
		else if (arg[i] == '\\' && arg[i+1] != '\0') {
			/* Unescape it. */
			opt[opt_len++] = arg[i+1];
			i++;
		}
		else {
			opt[opt_len++] = arg[i];
		}
	}

	/* Parse the last option here. */
	opt[opt_len] = '\0';
	if (parse_arg(args, argi, opt, outargs, data, opts, proc, true) == -1) {
		free(opt);
		return -1;
	}

	free(opt);
	return 0;
}

static int
parse_all(struct fuse_args *args, struct fuse_args *outargs, void *data,
		const struct fuse_opt *opts, fuse_opt_proc_t proc)
{
	bool nonopt = false; /* Have we seen the "--" marker? */
	int i;

	/* The first argument, the program name, is implicitly
	 * FUSE_OPT_KEY_KEEP. */
	if (args->argc > 0) {
		if (fuse_opt_add_arg(outargs, args->argv[0]) == -1)
			return -1;
	}

	/* the real loop to process the arguments */
	for (i = 1; i < args->argc; i++) {
		const char *arg = args->argv[i];

		/* argvn != -foo... */
		if (nonopt || arg[0] != '-') {
			if (call_proc(proc, data, arg, FUSE_OPT_KEY_NONOPT,
						outargs, false) == -1)
				return -1;
		}
		/* -o or -ofoo */
		else if (arg[1] == 'o') {
			/* -oblah,foo... */
			if (arg[2] != '\0') {
				/* skip -o */
				if (parse_opts(args, &i, arg + 2, outargs,
							data, opts, proc) == -1)
					return -1;
			}
			/* -o blah,foo... */
			else {
				if (next_arg(args, &i) == -1)
					return -1;
				if (parse_opts(args, &i, args->argv[i], outargs,
							data, opts, proc) == -1)
					return -1;
			}
		}
		/* -- */
		else if (arg[1] == '-' && arg[2] == '\0') {
			if (fuse_opt_add_arg(outargs, arg) == -1)
				return -1;
			nonopt = true;
		}
		/* -foo */
		else {
			if (parse_arg(args, &i, arg, outargs,
						data, opts, proc, false) == -1)
				return -1;
		}
	}

	/* The "--" marker at the last of outargs should be removed */
	if (nonopt && strcmp(outargs->argv[outargs->argc - 1], "--") == 0) {
		free(outargs->argv[outargs->argc - 1]);
		outargs->argv[--outargs->argc] = NULL;
	}

	return 0;
}

int
fuse_opt_parse(struct fuse_args *args, void *data,
		const struct fuse_opt *opts, fuse_opt_proc_t proc)
{
	struct fuse_args outargs = FUSE_ARGS_INIT(0, NULL);
	int rv;

	if (!args || !args->argv || !args->argc)
		return 0;

	rv = parse_all(args, &outargs, data, opts, proc);
	if (rv != -1) {
		/* Succeeded. Swap the outargs and args. */
		struct fuse_args tmp = *args;
		*args = outargs;
		outargs = tmp;
	}

	fuse_opt_free_args(&outargs);
	return rv;
}
