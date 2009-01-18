/* $NetBSD: pw_policy.c,v 1.14 2009/01/18 12:13:04 lukem Exp $ */

/*-
 * Copyright (c) 2005, 2006 Elad Efrat <elad@NetBSD.org>
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <stdio.h>
#include <util.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <assert.h>

#include <machine/int_limits.h>

#define	PW_POLICY_DEFAULTKEY	"pw_policy"

#define	LOAD_POLICY	0
#define	TEST_POLICY	1

#define	MINMAX_ERR(min, max, n)	((min == 0 || max == 0) && n != 0) ||	\
				 (min > 0 && min > n) ||		\
				 (max > 0 && max < n)

#define	HANDLER_PROTO	pw_policy_t, int, char *, void *, void *
#define	HANDLER_ARGS	pw_policy_t policy, int flag, char *pw, void *arg, void *arg2

static int pw_policy_parse_num(char *, int32_t *);
static int pw_policy_parse_range(char *, int32_t *, int32_t *);

static int pw_policy_handle_len(HANDLER_PROTO);
static int pw_policy_handle_charclass(HANDLER_PROTO);
static int pw_policy_handle_nclasses(HANDLER_PROTO);
static int pw_policy_handle_ntoggles(HANDLER_PROTO);

struct pw_policy {
	int32_t minlen;
	int32_t maxlen;
	int32_t minupper;
	int32_t maxupper;
	int32_t minlower;
	int32_t maxlower;
	int32_t mindigits;
	int32_t maxdigits;
	int32_t minpunct;
	int32_t maxpunct;
	int32_t mintoggles;
	int32_t maxtoggles;
	int32_t minclasses;
	int32_t maxclasses;
};

struct pw_policy_handler {
	const char *name;
	int (*handler)(HANDLER_PROTO);
	void *arg2;
};

static struct pw_policy_handler handlers[] = {
	{ "length", pw_policy_handle_len, NULL },
	{ "lowercase", pw_policy_handle_charclass, islower },
	{ "uppercase", pw_policy_handle_charclass, isupper },
	{ "digits", pw_policy_handle_charclass, isdigit },
	{ "punctuation", pw_policy_handle_charclass, ispunct },
	{ "nclasses", pw_policy_handle_nclasses, NULL },
	{ "ntoggles", pw_policy_handle_ntoggles, NULL },
	{ NULL, NULL, NULL },
};

static int
pw_policy_parse_num(char *s, int32_t *n)
{
	char *endptr = NULL;
	long l;

	if (s[0] == '*' && s[1] == '\0') {
		*n = -1;
		return 0;
	}

	errno = 0;
	l = strtol(s, &endptr, 10);
	if (*endptr != '\0')
		return EINVAL;
	if (errno == ERANGE && (l == LONG_MIN || l == LONG_MAX))
		return ERANGE;

	if (l < 0 || l >= UINT16_MAX)
		return ERANGE;

	*n = (int32_t)l;

	return 0;
}

static int
pw_policy_parse_range(char *range, int32_t *n, int32_t *m)
{
	char *p;

	while (isspace((unsigned char)*range))
		range++;
	p = &range[strlen(range) - 1];
	while (isspace((unsigned char)*p))
		*p-- = '\0';

	/* Single characters: * = any number, 0 = none. */
	if (range[0] == '0' && range[1] == '\0') {
		*n = *m = 0;
		return 0;
	}
	if (range[0] == '*' && range[1] == '\0') {
		*n = *m = -1;
		return 0;
	}

	/* Get range, N-M. */
	p = strchr(range, '-');
	if (p == NULL) {
		/* Exact match. */
		if (pw_policy_parse_num(range, n) != 0)
			return EINVAL;

		*m = *n;

		return 0;
	}
	*p++ = '\0';

	if (pw_policy_parse_num(range, n) != 0 ||
	    pw_policy_parse_num(p, m) != 0)
		return EINVAL;

	return (0);
}

/*ARGSUSED*/
static int
pw_policy_handle_len(HANDLER_ARGS __unused)
{
	size_t len;

	switch (flag) {
	case LOAD_POLICY:
		/* Here, '0' and '*' mean the same. */
		if (pw_policy_parse_range(arg, &policy->minlen,
		    &policy->maxlen) != 0)
			return EINVAL;

		return 0;

	case TEST_POLICY:
		len = strlen(pw);

		if ((policy->minlen > 0 && (int32_t)len < policy->minlen) ||
		    (policy->maxlen > 0 && (int32_t)len > policy->maxlen))
			return EPERM;
		return 0;
	default:

		return EINVAL;
	}
}

static int
pw_policy_handle_charclass(HANDLER_ARGS)
{
	int (*ischarclass)(int);
	int32_t *n = NULL, *m = NULL, count = 0;

	if (arg2 == islower) {
		n = &policy->minlower;
		m = &policy->maxlower;
	} else if (arg2 == isupper) {
		n = &policy->minupper;
		m = &policy->maxupper;
	} else if (arg2 == isdigit) {
		n = &policy->mindigits;
		m = &policy->maxdigits;
	} else if (arg2 == ispunct) {
		n = &policy->minpunct;
		m = &policy->maxpunct;
	} else
		return EINVAL;

	switch (flag) {
	case LOAD_POLICY:
		if ((pw_policy_parse_range(arg, n, m) != 0))
			return EINVAL;
		return 0;

	case TEST_POLICY:
		ischarclass = arg2;

		do {
			if (ischarclass((unsigned char)*pw++))
				count++;
		} while (*pw != '\0');

		if (MINMAX_ERR(*n, *m, count))
			return EPERM;
		return 0;

	default:
		return EINVAL;
	}
}

/*ARGSUSED*/
static int
pw_policy_handle_nclasses(HANDLER_ARGS __unused)
{
	switch (flag) {
	case LOAD_POLICY:
		if (pw_policy_parse_range(arg, &policy->minclasses,
		    &policy->maxclasses) != 0)
			return EINVAL;

		return 0;

	case TEST_POLICY: {
		int have_lower = 0, have_upper = 0, have_digit = 0,
		    have_punct = 0;
		int32_t nsets = 0;

		while (*pw != '\0') {
			if (islower((unsigned char)*pw) && !have_lower) {
				have_lower = 1;
				nsets++;
			} else if (isupper((unsigned char)*pw) && !have_upper) {
				have_upper = 1;
				nsets++;
			} else if (isdigit((unsigned char)*pw) && !have_digit) {
				have_digit = 1;
				nsets++;
			} else if (ispunct((unsigned char)*pw) && !have_punct) {
				have_punct = 1;
				nsets++;
			}
			pw++;
		}

		if (MINMAX_ERR(policy->minclasses, policy->maxclasses, nsets))
			return EPERM;
		return 0;
		}
	default:
		return EINVAL;
	}
}

/*ARGSUSED*/
static int
pw_policy_handle_ntoggles(HANDLER_ARGS __unused)
{
	switch (flag) {
	case LOAD_POLICY:
		if (pw_policy_parse_range(arg, &policy->mintoggles,
		    &policy->maxtoggles) != 0)
			return EINVAL;
		return 0;

	case TEST_POLICY: {
		int previous_set = 0, current_set = 0;
		int32_t ntoggles = 0, current_ntoggles = 0;

#define	CHAR_CLASS(c)		(islower((unsigned char)(c)) ? 1 :	\
				 isupper((unsigned char)(c)) ? 2 :	\
				 isdigit((unsigned char)(c)) ? 3 :	\
				 ispunct((unsigned char)(c)) ? 4 :	\
				 0)

		while (*pw != '\0') {
			if (!isspace((unsigned char)*pw)) {
				current_set = CHAR_CLASS(*pw);
				if (!current_set) {
					return EINVAL;
				}

				if (!previous_set ||
				    current_set == previous_set) {
					current_ntoggles++;
				} else {
					if (current_ntoggles > ntoggles)
						ntoggles = current_ntoggles;

					current_ntoggles = 1;
				}

				previous_set = current_set;
			}

			pw++;
		}

#undef CHAR_CLASS

		if (MINMAX_ERR(policy->mintoggles, policy->maxtoggles,
		    ntoggles))
			return EPERM;

		return 0;
		}
	default:
		return EINVAL;
	}
}

pw_policy_t
pw_policy_load(void *key, int how)
{
	pw_policy_t policy;
	struct pw_policy_handler *hp;
	char buf[BUFSIZ];

	/* If there's no /etc/passwd.conf, don't touch the policy. */
	if (access(_PATH_PASSWD_CONF, R_OK) == -1)
		return NULL;

	/* No key provided. Use default. */
	if (key == NULL) {
		key = __UNCONST(PW_POLICY_DEFAULTKEY);
		goto load_policies;
	}

	(void)memset(buf, 0, sizeof(buf));

	errno = 0;
	switch (how) {
	case PW_POLICY_BYSTRING:
		/*
		 * Check for provided key. If non-existant, fallback to
		 * the default.
		 */
		pw_getconf(buf, sizeof(buf), key, "foo");
		if (errno == ENOTDIR)
			key = __UNCONST(PW_POLICY_DEFAULTKEY);

		break;

	case PW_POLICY_BYPASSWD: {
		struct passwd *pentry = key;

		/*
		 * Check for policy for given user. If can't find any,
		 * try a policy for the login class. If can't find any,
		 * fallback to the default.
		 */

		pw_getconf(buf, sizeof(buf), pentry->pw_name, "foo");
		if (errno == ENOTDIR) {
			memset(buf, 0, sizeof(buf));
			pw_getconf(buf, sizeof(buf), pentry->pw_class, "foo");
			if (errno == ENOTDIR)
				key = __UNCONST(PW_POLICY_DEFAULTKEY);
			else
				key = pentry->pw_class;
		} else
			key = pentry->pw_name;

		break;
		}

	case PW_POLICY_BYGROUP: {
		struct group *gentry = key;

		/*
		 * Check for policy for given group. If can't find any,
		 * fallback to the default.
		 */
		pw_getconf(buf, sizeof(buf), gentry->gr_name, "foo");
		if (errno == ENOTDIR)
			key = __UNCONST(PW_POLICY_DEFAULTKEY);
		else
			key = gentry->gr_name;

		break;
		}

	default:
		/*
		 * Fail the policy because we don't know how to parse the
		 * key we were passed.
		 */
		errno = EINVAL;
		return NULL;
	}

 load_policies:
	policy = malloc(sizeof(struct pw_policy));
	if (policy == NULL)
		return NULL;

	memset(policy, 0xff, sizeof(struct pw_policy));

	hp = &handlers[0];
	while (hp->name != NULL) {
		int error;

		(void)memset(buf, 0, sizeof(buf));
		pw_getconf(buf, sizeof(buf), key, hp->name);
		if (*buf) {
			error = (*hp->handler)(policy, LOAD_POLICY, NULL, buf,
			    hp->arg2);
			if (error) {
				errno = error;
				return NULL;
			}
		}
		hp++;
	}

	return policy;
}

int
pw_policy_test(pw_policy_t policy, char *pw)
{
	struct pw_policy_handler *hp;

	if (policy == NULL) {
		errno = EINVAL;
		return 0;
	}

	hp = &handlers[0];
	while (hp->name != NULL) {
		int error;
		error = (*hp->handler)(policy, TEST_POLICY, pw, NULL, hp->arg2);
		if (error) {
			errno = error;
			return -1;
		}
		hp++;
	}

	return 0;
}

void
pw_policy_free(pw_policy_t policy)
{
	_DIAGASSERT(policy != NULL);

	free(policy);
}
