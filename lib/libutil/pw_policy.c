/* $NetBSD: pw_policy.c,v 1.2 2005/09/16 22:38:48 elad Exp $ */

/*-
 * Copyright 2005 Elad Efrat <elad@NetBSD.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

#include <machine/int_limits.h>

#define	PW_POLICY_DEFAULTKEY	"pw_policy"

#define	PW_GETCONF(buf, len, key, opt)			\
	do {						\
		memset(buf, 0, len);	\
		pw_getconf(buf, len, key, opt);		\
	} while (/*CONSTCOND*/0)

#define	NEED_NONE	0
#define	NEED_ARG	1
#define	NEED_ARGS	2

#define	HANDLER_PROTO	char *, size_t, void *, void *
#define	HANDLER_ARGS	char *pw, size_t len, void *arg, void *arg2
#define	HANDLER_SANE(needarg)					\
	((pw != NULL) && (len != 0) &&				\
	 ((/*CONSTCOND*/needarg == NEED_NONE) ||		\
	  (/*CONSTCOND*/needarg == NEED_ARG && arg != NULL) ||	\
	  (/*CONSTCOND*/needarg == NEED_ARGS && arg != NULL && arg2 != NULL)))

static int pw_policy_parse_num(char *, int32_t *);
static int pw_policy_parse_range(char *, int32_t *, int32_t *);
static int pw_policy_handle_len(HANDLER_PROTO);
static int pw_policy_handle_charclass(HANDLER_PROTO);
static int pw_policy_handle_nclasses(HANDLER_PROTO);
static int pw_policy_handle_ntoggles(HANDLER_PROTO);

struct pw_policy_handler {
	const char *name;
	int (*handler)(char *, size_t, void *, void *);
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
	unsigned long m;

	if (s == NULL || n == NULL)
		return (EFAULT);

	m = strtoul(s, &endptr, 10);
	if (*endptr != '\0' || m == ULONG_MAX)
		return (EINVAL);

	/*
	 * We receive a signed 32-bit integer, but we only allow
	 * unsigned 16-bit values.
	 */
	if (m >= (unsigned long)UINT16_MAX)
		return (ERANGE);

	*n = (int32_t)m;

	return (0);
}

static int
pw_policy_parse_range(char *range, int32_t *n, int32_t *m)
{
	char *p;

	if (range == NULL || n == NULL || m == NULL)
		return (EFAULT);

	while (isspace((int)*range))
		range++;
	p = &range[strlen(range) - 1];
	while (isspace((int)*p))
		*p-- = '\0';

	/* Single characters: * = any number, 0 = none. */
	if (range[0] == '0' && range[1] == '\0') {
		*n = *m = -1;
		return (0);
	}
	if (range[0] == '*' && range[1] == '\0') {
		*n = *m = 0;
		return (0);
	}

	/* Get range, N-M. */
	p = strchr(range, '-');
	if (p == NULL) {
		/* Exact match. */
		if (pw_policy_parse_num(range, n) != 0)
			return (EINVAL);

		*m = *n;

		return (0);
	}
	*p++ = '\0';

	if (pw_policy_parse_num(range, n) != 0 ||
	    pw_policy_parse_num(p, m) != 0)
		return (EINVAL);

	return (0);
}

static int
pw_policy_handle_len(HANDLER_ARGS)
{
	int32_t n = 0, m = 0;

	if (!HANDLER_SANE(NEED_ARG))
		return (EFAULT);

	/* Here, '0' and '*' mean the same. */
	if (pw_policy_parse_range(arg, &n, &m) != 0)
		return (EINVAL);

	if (n < 0)
		n = 0;
	if (m < 0)
		m = 0;

	if ((n && len < n) || (m && len > m))
		return (EPERM);

	return (0);
}

static int
pw_policy_handle_charclass(HANDLER_ARGS)
{
	int (*ischarclass)(int);
	int32_t n = 0, m = 0, count = 0;

	if (!HANDLER_SANE(NEED_ARGS))
		return (EFAULT);

	if (pw_policy_parse_range(arg, &n, &m) != 0)
		return (EINVAL);

	ischarclass = arg2;

	if (n == 0 && m == 0)
		return (0);

	do {
		if (ischarclass((int)*pw++))
			count++;
	} while (*pw != '\0');

	if (n == -1 && count)
		return (EPERM);

	if ((n >= 0 && count < n) || (m >= 0 && count > m))
		return (EPERM);

	return (0);
}

static int
pw_policy_handle_nclasses(HANDLER_ARGS)
{
	int have_lower = 0, have_upper = 0, have_digit = 0, have_punct = 0;
	int32_t n = 0, m = 0, nsets = 0;

	if (!HANDLER_SANE(NEED_ARG))
		return (EFAULT);

	if (pw_policy_parse_range(arg, &n, &m) != 0)
		return (EINVAL);

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

	if ((n >= 0 && nsets < n) || (m >= 0 && nsets > m))
		return (EPERM);

	return (0);
}

static int
pw_policy_handle_ntoggles(HANDLER_ARGS)
{
	int previous_set = 0, current_set = 0;
	int32_t n = 0, m = 0, ntoggles = 0;

	if (!HANDLER_SANE(NEED_ARG))
		return (EFAULT);

	if (pw_policy_parse_range(arg, &n, &m) != 0)
		return (EINVAL);

#define	CHAR_CLASS(c)		(islower((unsigned char)(c)) ? 1 :	\
				isupper((unsigned char)(c))  ? 2 :	\
				isdigit((unsigned char)(c))  ? 3 :	\
				ispunct((unsigned char)(c))  ? 4 :	\
				 0)

	while (*pw != '\0') {
		current_set = CHAR_CLASS(*pw);
		if (!current_set) {
			return (EINVAL);
		}

		if (!previous_set || current_set == previous_set) {
			ntoggles++;
		} else {
			ntoggles = 1;
		}

		previous_set = current_set;
		pw++;
	}

#undef CHAR_CLASS

	if ((n >= 0 && ntoggles < n) || (m >= 0 && ntoggles > m))
		return (EPERM);

	return (0);
}

int
pw_policy_test(char *pw, void *key, int how)
{
	struct pw_policy_handler *hp;
	char buf[BUFSIZ];
	size_t pwlen;

	/*
	 * No password provided, fail the test.
	 */
	if (pw == NULL)
		return (EFAULT);

	/*
	 * If there's no /etc/passwd.conf, we allow the password.
	 */
	if (access(_PATH_PASSWD_CONF, R_OK) == -1)
		return (ENOENT);

	/*
	 * No key provided. Use default.
	 */
	if (key == NULL) {
		key = __UNCONST(PW_POLICY_DEFAULTKEY);
		goto test_policies;
	}

	switch (how) {
	case PW_POLICY_BYSTRING:
		/*
		 * Check for provided key. If non-existant, fallback to
		 * the default.
		 */
		errno = 0;
		PW_GETCONF(buf, sizeof(buf), key, "foo");
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
		errno = 0;
		PW_GETCONF(buf, sizeof(buf), pentry->pw_name, "foo");
		if (errno == ENOTDIR) {
			PW_GETCONF(buf, sizeof(buf), pentry->pw_class, "foo");
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
		errno = 0;
		PW_GETCONF(buf, sizeof(buf), gentry->gr_name, "foo");
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
		return (EINVAL);
	}

 test_policies:
	pwlen = strlen(pw);

	hp = &handlers[0];
	while (hp->name != NULL) {
		PW_GETCONF(buf, sizeof(buf), key, hp->name);
		if (*buf && hp->handler(pw, pwlen, buf, hp->arg2) != 0)
			return (EPERM);

		hp++;
	}

	return (0);
}
