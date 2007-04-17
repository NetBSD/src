/* 	$NetBSD: refuse_opt.c,v 1.9 2007/04/17 06:34:48 agc Exp $	*/

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
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without
 *    specific prior written permission.
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

/*
 * TODO:
 * 	* -oblah,foo... works, but the options are not enabled.
 * 	* -ofoo=%s (accepts a string) or -ofoo=%u (int) is not
 * 	  supported for now.
 * 	* void *data: how is it used? I think it's used to enable
 * 	  options or pass values for the matching options.
 */

#include "defs.h"
#include "fuse.h"
#include "fuse_opt.h"

#ifdef FUSE_OPT_DEBUG
#define DPRINTF(x)	do { printf x; } while ( /* CONSTCOND */ 0)
#else
#define DPRINTF(x)
#endif

enum {
	KEY_HELP,
	KEY_VERBOSE,
	KEY_VERSION
};

struct fuse_opt_option {
	const struct fuse_opt *fop;
	char *option;
	int key;
	void *data;
};

static int fuse_opt_popt(struct fuse_opt_option *, const struct fuse_opt *);

/* 
 * Public API.
 *
 * The following functions always return 0:
 *
 * int	fuse_opt_add_opt(char **, const char *);
 *
 * We implement the next ones:
 *
 * int	fuse_opt_add_arg(struct fuse_args *, const char *);
 * void	fuse_opt_free_args(struct fuse_args *);
 * int	fuse_opt_insert_arg(struct fuse_args *, const char *);
 * int	fuse_opt_match(const struct fuse_opt *, const char *);
 * int	fuse_opt_parse(struct fuse_args *, void *,
 * 		       const struct fuse_opt *, fuse_opt_proc_t);
 *
 */

/* function to perform a deep copy of the args structure */
static struct fuse_args *
deep_copy_args(int argc, char **argv)
{
	struct fuse_args	*ap;

	NEW(struct fuse_args, ap, "deep_copy_args", return NULL);
	/* deep copy args structure into channel args */
	ap->allocated = ((argc / 10) + 1) * 10;
	NEWARRAY(char *, ap->argv, ap->allocated, "fuse_mount", return NULL);
	for (ap->argc = 0 ; ap->argc < argc ; ap->argc++) {
		ap->argv[ap->argc] = strdup(argv[ap->argc]);
	}
	return ap;
}

/* ARGSUSED */
int
fuse_opt_add_arg(struct fuse_args *args, const char *arg)
{
	struct fuse_args	*ap;

	if (args->allocated == 0) {
		ap = deep_copy_args(args->argc, args->argv);
		args->argv = ap->argv;
		args->argc = ap->argc;
		args->allocated = ap->allocated;
		(void) free(ap);
	} else if (args->allocated == args->argc) {
		args->allocated += 10;
		RENEW(char *, args->argv, args->allocated,
		    "fuse_opt_add_arg", return -1);
	}
	DPRINTF(("%s: arguments passed: [arg:%s]\n", __func__, arg));
	args->argv[args->argc++] = strdup(arg);
        return 0;
}

/* ARGSUSED */
void
fuse_opt_free_args(struct fuse_args *args)
{
	int	i;

	for (i = 0 ; i < args->argc ; i++) {
		FREE(args->argv[i]);
	}
	FREE(args->argv);
	args->allocated = args->argc = 0;
}

/* ARGSUSED */
int
fuse_opt_insert_arg(struct fuse_args *args, int pos, const char *arg)
{
	int	i;

	DPRINTF(("%s: arguments passed: [pos=%d] [arg=%s]\n",
	    __func__, pos, arg));
	ALLOC(char *, args->argv, args->allocated, args->argc, 10, 10,
	    "fuse_opt_insert_org", return -1);
	for (i = args->argc++ ; i > pos ; --i) {
		args->argv[i] = args->argv[i - 1];
	}
	args->argv[pos] = strdup(arg);
	return 0;
}

/* ARGSUSED */
int fuse_opt_add_opt(char **opts, const char *opt)
{
	DPRINTF(("%s: arguments passed: [opts=%s] [opt=%s]\n",
	    __func__, *opts, opt));
	return 0;
}

/*
 * Returns 0 if opt was matched with any option from opts,
 * otherwise returns 1.
 */
int
fuse_opt_match(const struct fuse_opt *opts, const char *opt)
{
	while (opts++) {
		if (strcmp(opt, opts->templ) == 0)
			return 0;
	}

	return 1;
}

/*
 * Returns 0 if foo->option was matched with any option from opts,
 * and sets the following on match:
 *
 * 	* foo->key is set to the foo->fop->value if offset == -1.
 * 	* foo->fop points to the matched struct opts.
 *
 * otherwise returns 1.
 */
static int
fuse_opt_popt(struct fuse_opt_option *foo, const struct fuse_opt *opts)
{
	int i, found = 0;
	char *match;
	
	if (!foo->option) {
		(void)fprintf(stderr, "fuse: missing argument after -o\n");
		return 1;
	}
	/* 
	 * iterate over argv and opts to see
	 * if there's a match with any template.
	 */
	for (match = strtok(foo->option, ",");
	     match; match = strtok(NULL, ",")) {

		DPRINTF(("%s: specified option='%s'\n", __func__, match));
		found = 0;

		for (i = 0; opts && opts->templ; opts++, i++) {

			DPRINTF(("%s: opts->templ='%s' opts->offset=%d "
			    "opts->value=%d\n", __func__, opts->templ,
			    opts->offset, opts->value));

			/* option is ok */
			if (strcmp(match, opts->templ) == 0) {
				DPRINTF(("%s: option matched='%s'\n",
				    __func__, match));
				found++;
				/*
				 * our fop pointer now points 
				 * to the matched struct opts.
				 */
				foo->fop = opts;
				/* 
				 * assign default key value, necessary for
				 * KEY_HELP, KEY_VERSION and KEY_VERBOSE.
				 */
				if (foo->fop->offset == -1)
					foo->key = foo->fop->value;
				/* reset counter */
				opts -= i;
				break;
			}
		}
		/* invalid option */
		if (!found) {
			(void)fprintf(stderr, "fuse: '%s' is not a "
			    "valid option\n", match);
			return 1;
		}
	}

	return 0;
}

/* ARGSUSED1 */
int
fuse_opt_parse(struct fuse_args *args, void *data,
        const struct fuse_opt *opts, fuse_opt_proc_t proc)
{
	struct fuse_opt_option foo;
	char *buf;
	int i, rv = 0;

	if (!args || !args->argv || !args->argc || !proc)
		return 0;

	if (args->argc == 1)
		return proc(foo.data, *args->argv, FUSE_OPT_KEY_OPT, args);

	/* the real loop to process the arguments */
	for (i = 1; i < args->argc; i++) {

		/* assign current argv string */
		foo.option = buf = args->argv[i];

		/* argvn != -foo... */
		if (buf[0] != '-') {

			foo.key = FUSE_OPT_KEY_NONOPT;
			rv = proc(foo.data, foo.option, foo.key, args);
			if (rv != 0)
				break;

		/* -o was specified... */
		} else if (buf[0] == '-' && buf[1] == 'o') {

			/* -oblah,foo... */
			if (buf[2]) {
				/* skip -o */
				foo.option = args->argv[i] + 2;
			/* -o blah,foo... */
			} else {
				/* 
			 	 * skip current argv and pass to the
			 	 * next one to parse the options.
				 */
				++i;
				foo.option = args->argv[i];
			}

			rv = fuse_opt_popt(&foo, opts);
			if (rv != 0)
				break;

		/* help/version/verbose argument */
		} else if (buf[0] == '-' && buf[1] != 'o') {
			/* 
			 * check if the argument matches
			 * with any template in opts.
			 */
			rv = fuse_opt_popt(&foo, opts);
			if (rv != 0) {
				break;
			} else {
				DPRINTF(("%s: foo.fop->templ='%s' "
			    	    "foo.fop->offset: %d "
			    	    "foo.fop->value: %d\n",
			    	    __func__, foo.fop->templ,
			    	    foo.fop->offset, foo.fop->value));

				/* argument needs to be discarded */
				if (foo.key == FUSE_OPT_KEY_DISCARD) {
					rv = 1;
					break;
				}

				/* process help/version argument */
				if (foo.key != KEY_VERBOSE &&
				    foo.key != FUSE_OPT_KEY_KEEP) {
					rv = proc(foo.data, foo.option,
				    		  foo.key, args);
					break;
				} else {
					/* process verbose argument */
					rv = proc(foo.data, foo.option,
						       foo.key, args);
					if (rv != 0)
						break;
				}
			}
		/* unknown option, how could that happen? */
		} else {
			DPRINTF(("%s: unknown option\n", __func__));
			rv = 1;
			break;
		}
	}

	return rv;
}
