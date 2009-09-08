/* $NetBSD: cgdconfig.c,v 1.28 2009/09/08 21:36:35 pooka Exp $ */

/*-
 * Copyright (c) 2002, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roland C. Dowdeswell.
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

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 2002, 2003\
 The NetBSD Foundation, Inc.  All rights reserved.");
__RCSID("$NetBSD: cgdconfig.c,v 1.28 2009/09/08 21:36:35 pooka Exp $");
#endif

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <sys/ioctl.h>
#include <sys/disklabel.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/resource.h>

#include <dev/cgdvar.h>

#include <ufs/ffs/fs.h>

#include "params.h"
#include "pkcs5_pbkdf2.h"
#include "utils.h"
#include "cgd_kernelops.h"
#include "cgdconfig.h"

#define CGDCONFIG_DIR		"/etc/cgd"
#define CGDCONFIG_CFILE		CGDCONFIG_DIR "/cgd.conf"

enum action {
	 ACTION_DEFAULT,		/* default -> configure */
	 ACTION_CONFIGURE,		/* configure, with paramsfile */
	 ACTION_UNCONFIGURE,		/* unconfigure */
	 ACTION_GENERATE,		/* generate a paramsfile */
	 ACTION_GENERATE_CONVERT,	/* generate a ``dup'' paramsfile */
	 ACTION_CONFIGALL,		/* configure all from config file */
	 ACTION_UNCONFIGALL,		/* unconfigure all from config file */
	 ACTION_CONFIGSTDIN		/* configure, key from stdin */
};

/* if nflag is set, do not configure/unconfigure the cgd's */

int	nflag = 0;

/* if pflag is set to PFLAG_STDIN read from stdin rather than getpass(3) */

#define	PFLAG_GETPASS	0x01
#define	PFLAG_STDIN	0x02
int	pflag = PFLAG_GETPASS;

static int	configure(int, char **, struct params *, int);
static int	configure_stdin(struct params *, int argc, char **);
static int	generate(struct params *, int, char **, const char *);
static int	generate_convert(struct params *, int, char **, const char *);
static int	unconfigure(int, char **, struct params *, int);
static int	do_all(const char *, int, char **,
		       int (*)(int, char **, struct params *, int));

#define CONFIG_FLAGS_FROMALL	1	/* called from configure_all() */
#define CONFIG_FLAGS_FROMMAIN	2	/* called from main() */

static int	 configure_params(int, const char *, const char *,
				  struct params *);
static void	 eliminate_cores(void);
static bits_t	*getkey(const char *, struct keygen *, size_t);
static bits_t	*getkey_storedkey(const char *, struct keygen *, size_t);
static bits_t	*getkey_randomkey(const char *, struct keygen *, size_t, int);
static bits_t	*getkey_pkcs5_pbkdf2(const char *, struct keygen *, size_t,
				     int);
static bits_t	*getkey_shell_cmd(const char *, struct keygen *, size_t);
static char	*maybe_getpass(char *);
static int	 opendisk_werror(const char *, char *, size_t);
static int	 unconfigure_fd(int);
static int	 verify(struct params *, int);
static int	 verify_disklabel(int);
static int	 verify_ffs(int);
static int	 verify_reenter(struct params *);

static void	 usage(void);

/* Verbose Framework */
unsigned	verbose = 0;

#define VERBOSE(x,y)	if (verbose >= x) y
#define VPRINTF(x,y)	if (verbose >= x) (void)printf y

static void
usage(void)
{

	(void)fprintf(stderr, "usage: %s [-nv] [-V vmeth] cgd dev [paramsfile]\n",
	    getprogname());
	(void)fprintf(stderr, "       %s -C [-nv] [-f configfile]\n", getprogname());
	(void)fprintf(stderr, "       %s -U [-nv] [-f configfile]\n", getprogname());
	(void)fprintf(stderr, "       %s -G [-nv] [-i ivmeth] [-k kgmeth] "
	    "[-o outfile] paramsfile\n", getprogname());
	(void)fprintf(stderr, "       %s -g [-nv] [-i ivmeth] [-k kgmeth] "
	    "[-o outfile] alg [keylen]\n", getprogname());
	(void)fprintf(stderr, "       %s -s [-nv] [-i ivmeth] cgd dev alg "
	    "[keylen]\n", getprogname());
	(void)fprintf(stderr, "       %s -u [-nv] cgd\n", getprogname());
	exit(EXIT_FAILURE);
}

static int
parse_size_t(const char *s, size_t *l)
{
	char *endptr;
	long v;

	errno = 0;
	v = strtol(s, &endptr, 10);
	if ((v == LONG_MIN || v == LONG_MAX) && errno)
		return -1;
	if (v < INT_MIN || v > INT_MAX) {
		errno = ERANGE;
		return -1;
	}
	if (endptr == s) {
		errno = EINVAL;
		return -1;
	}
	*l = (size_t)v;
	return 0;
}

static void
set_action(enum action *action, enum action value)
{
	if (*action != ACTION_DEFAULT)
		usage();
	*action = value;
}

#ifndef CGDCONFIG_AS_LIB
int
main(int argc, char **argv)
{

	return cgdconfig(argc, argv);
}
#endif

int
cgdconfig(int argc, char *argv[])
{
	struct params *p;
	struct params *tp;
	struct keygen *kg;
	enum action action = ACTION_DEFAULT;
	int	ch;
	const char	*cfile = NULL;
	const char	*outfile = NULL;

	setprogname(*argv);
	eliminate_cores();
	if (mlockall(MCL_FUTURE))
		err(EXIT_FAILURE, "Can't lock memory");
	p = params_new();
	kg = NULL;

	while ((ch = getopt(argc, argv, "CGUV:b:f:gi:k:no:spuv")) != -1)
		switch (ch) {
		case 'C':
			set_action(&action, ACTION_CONFIGALL);
			break;
		case 'G':
			set_action(&action, ACTION_GENERATE_CONVERT);
			break;
		case 'U':
			set_action(&action, ACTION_UNCONFIGALL);
			break;
		case 'V':
			tp = params_verify_method(string_fromcharstar(optarg));
			if (!tp)
				usage();
			p = params_combine(p, tp);
			break;
		case 'b':
			{
				size_t size;

				if (parse_size_t(optarg, &size) == -1)
					usage();
				tp = params_bsize(size);
				if (!tp)
					usage();
				p = params_combine(p, tp);
			}
			break;
		case 'f':
			if (cfile)
				usage();
			cfile = estrdup(optarg);
			break;
		case 'g':
			set_action(&action, ACTION_GENERATE);
			break;
		case 'i':
			tp = params_ivmeth(string_fromcharstar(optarg));
			p = params_combine(p, tp);
			break;
		case 'k':
			kg = keygen_method(string_fromcharstar(optarg));
			if (!kg)
				usage();
			keygen_addlist(&p->keygen, kg);
			break;
		case 'n':
			nflag = 1;
			break;
		case 'o':
			if (outfile)
				usage();
			outfile = estrdup(optarg);
			break;
		case 'p':
			pflag = PFLAG_STDIN;
			break;
		case 's':
			set_action(&action, ACTION_CONFIGSTDIN);
			break;

		case 'u':
			set_action(&action, ACTION_UNCONFIGURE);
			break;
		case 'v':
			verbose++;
			break;
		default:
			usage();
			/* NOTREACHED */
		}

	argc -= optind;
	argv += optind;

	if (!outfile)
		outfile = "";
	if (!cfile)
		cfile = "";

	/* validate the consistency of the arguments */

	switch (action) {
	case ACTION_DEFAULT:	/* ACTION_CONFIGURE is the default */
	case ACTION_CONFIGURE:
		return configure(argc, argv, p, CONFIG_FLAGS_FROMMAIN);
	case ACTION_UNCONFIGURE:
		return unconfigure(argc, argv, NULL, CONFIG_FLAGS_FROMMAIN);
	case ACTION_GENERATE:
		return generate(p, argc, argv, outfile);
	case ACTION_GENERATE_CONVERT:
		return generate_convert(p, argc, argv, outfile);
	case ACTION_CONFIGALL:
		return do_all(cfile, argc, argv, configure);
	case ACTION_UNCONFIGALL:
		return do_all(cfile, argc, argv, unconfigure);
	case ACTION_CONFIGSTDIN:
		return configure_stdin(p, argc, argv);
	default:
		errx(EXIT_FAILURE, "undefined action");
		/* NOTREACHED */
	}
}

static bits_t *
getkey(const char *dev, struct keygen *kg, size_t len)
{
	bits_t	*ret = NULL;
	bits_t	*tmp;

	VPRINTF(3, ("getkey(\"%s\", %p, %zu) called\n", dev, kg, len));
	for (; kg; kg=kg->next) {
		switch (kg->kg_method) {
		case KEYGEN_STOREDKEY:
			tmp = getkey_storedkey(dev, kg, len);
			break;
		case KEYGEN_RANDOMKEY:
			tmp = getkey_randomkey(dev, kg, len, 1);
			break;
		case KEYGEN_URANDOMKEY:
			tmp = getkey_randomkey(dev, kg, len, 0);
			break;
		case KEYGEN_PKCS5_PBKDF2_SHA1:
			tmp = getkey_pkcs5_pbkdf2(dev, kg, len, 0);
			break;
		/* provide backwards compatibility for old config files */
		case KEYGEN_PKCS5_PBKDF2_OLD:
			tmp = getkey_pkcs5_pbkdf2(dev, kg, len, 1);
			break;
		case KEYGEN_SHELL_CMD:
			tmp = getkey_shell_cmd(dev, kg, len);
			break;
		default:
			warnx("unrecognised keygen method %d in getkey()",
			    kg->kg_method);
			if (ret)
				bits_free(ret);
			return NULL;
		}

		if (ret)
			ret = bits_xor_d(tmp, ret);
		else
			ret = tmp;
	}

	return ret;
}

/*ARGSUSED*/
static bits_t *
getkey_storedkey(const char *target, struct keygen *kg, size_t keylen)
{
	return bits_dup(kg->kg_key);
}

/*ARGSUSED*/
static bits_t *
getkey_randomkey(const char *target, struct keygen *kg, size_t keylen, int hard)
{
	return bits_getrandombits(keylen, hard);
}

static char *
maybe_getpass(char *prompt)
{
	char	 buf[1024];
	char	*p = buf;
	char	*tmp;

	switch (pflag) {
	case PFLAG_GETPASS:
		p = getpass(prompt);
		break;

	case PFLAG_STDIN:
		p = fgets(buf, sizeof(buf), stdin);
		if (p) {
			tmp = strchr(p, '\n');
			if (tmp)
				*tmp = '\0';
		}
		break;

	default:
		errx(EXIT_FAILURE, "pflag set inappropriately?");
	}

	if (!p)
		err(EXIT_FAILURE, "failed to read passphrase");

	return estrdup(p);
}

/*ARGSUSED*/
/* 
 * XXX take, and pass through, a compat flag that indicates whether we
 * provide backwards compatibility with a previous bug.  The previous
 * behaviour is indicated by the keygen method pkcs5_pbkdf2, and a
 * non-zero compat flag. The new default, and correct keygen method is
 * called pcks5_pbkdf2/sha1.  When the old method is removed, so will
 * be the compat argument.
 */
static bits_t *
getkey_pkcs5_pbkdf2(const char *target, struct keygen *kg, size_t keylen,
    int compat)
{
	bits_t		*ret;
	char		*passp;
	char		 buf[1024];
	u_int8_t	*tmp;

	snprintf(buf, sizeof(buf), "%s's passphrase:", target);
	passp = maybe_getpass(buf);
	if (pkcs5_pbkdf2(&tmp, BITS2BYTES(keylen), (uint8_t *)passp,
	    strlen(passp),
	    bits_getbuf(kg->kg_salt), BITS2BYTES(bits_len(kg->kg_salt)),
	    kg->kg_iterations, compat)) {
		warnx("failed to generate PKCS#5 PBKDF2 key");
		return NULL;
	}

	ret = bits_new(tmp, keylen);
	kg->kg_key = bits_dup(ret);
	memset(passp, 0, strlen(passp));
	free(passp);
	free(tmp);
	return ret;
}

/*ARGSUSED*/
static bits_t *
getkey_shell_cmd(const char *target, struct keygen *kg, size_t keylen)
{
	FILE	*f;
	bits_t	*ret;

	f = popen(string_tocharstar(kg->kg_cmd), "r");
	ret = bits_fget(f, keylen);
	pclose(f);

	return ret;
}

/*ARGSUSED*/
static int
unconfigure(int argc, char **argv, struct params *inparams, int flags)
{
	int	fd;
	int	ret;
	char	buf[MAXPATHLEN] = "";

	/* only complain about additional arguments, if called from main() */
	if (flags == CONFIG_FLAGS_FROMMAIN && argc != 1)
		usage();

	/* if called from do_all(), then ensure that 2 or 3 args exist */
	if (flags == CONFIG_FLAGS_FROMALL && (argc < 2 || argc > 3))
		return -1;

	fd = opendisk1(*argv, O_RDWR, buf, sizeof(buf), 1, cgd_kops.ko_open);
	if (fd == -1) {
		int saved_errno = errno;

		warn("can't open cgd \"%s\", \"%s\"", *argv, buf);

		/* this isn't fatal with nflag != 0 */
		if (!nflag)
			return saved_errno;
	}

	VPRINTF(1, ("%s (%s): clearing\n", *argv, buf));

	if (nflag)
		return 0;

	ret = unconfigure_fd(fd);
	(void)cgd_kops.ko_close(fd);
	return ret;
}

static int
unconfigure_fd(int fd)
{
	struct	cgd_ioctl ci;

	if (cgd_kops.ko_ioctl(fd, CGDIOCCLR, &ci) == -1) {
		warn("ioctl");
		return -1;
	}

	return 0;
}

/*ARGSUSED*/
static int
configure(int argc, char **argv, struct params *inparams, int flags)
{
	struct params	*p;
	struct keygen	*kg;
	int		 fd;
	int		 loop = 0;
	int		 ret;
	char		 cgdname[PATH_MAX];

	if (argc == 2) {	
		char *pfile;

		if (asprintf(&pfile, "%s/%s",
		    CGDCONFIG_DIR, basename(argv[1])) == -1)
			return -1;

		p = params_cget(pfile);
		free(pfile);
	} else if (argc == 3) {
		p = params_cget(argv[2]);
	} else {
		/* print usage and exit, only if called from main() */
		if (flags == CONFIG_FLAGS_FROMMAIN) {
			warnx("wrong number of args");
			usage();
		}
		return -1;
	}

	if (!p)
		return -1;

	/*
	 * over-ride with command line specifications and fill in default
	 * values.
	 */

	p = params_combine(p, inparams);
	ret = params_filldefaults(p);
	if (ret) {
		params_free(p);
		return ret;
	}

	if (!params_verify(p)) {
		warnx("params invalid");
		return -1;
	}

	/*
	 * loop over configuring the disk and checking to see if it
	 * verifies properly.  We open and close the disk device each
	 * time, because if the user passes us the block device we
	 * need to flush the buffer cache.
	 *
	 * We only loop if one of the verification methods prompts for
	 * a password.
	 */

	for (kg = p->keygen; pflag == PFLAG_GETPASS && kg; kg = kg->next)
		if ((kg->kg_method == KEYGEN_PKCS5_PBKDF2_SHA1) ||
		    (kg->kg_method == KEYGEN_PKCS5_PBKDF2_OLD )) {
			loop = 1;
			break;
		}

	for (;;) {
		fd = opendisk_werror(argv[0], cgdname, sizeof(cgdname));
		if (fd == -1)
			return -1;

		if (p->key)
			bits_free(p->key);

		p->key = getkey(argv[1], p->keygen, p->keylen);
		if (!p->key)
			goto bail_err;

		ret = configure_params(fd, cgdname, argv[1], p);
		if (ret)
			goto bail_err;

		ret = verify(p, fd);
		if (ret == -1)
			goto bail_err;
		if (!ret)
			break;

		(void)unconfigure_fd(fd);
		(void)cgd_kops.ko_close(fd);

		if (!loop) {
			warnx("verification failed permanently");
			goto bail_err;
		}

		warnx("verification failed, please reenter passphrase");
	}

	params_free(p);
	(void)cgd_kops.ko_close(fd);
	return 0;
bail_err:
	params_free(p);
	(void)cgd_kops.ko_close(fd);
	return -1;
}

static int
configure_stdin(struct params *p, int argc, char **argv)
{
	int	fd;
	int	ret;
	char	cgdname[PATH_MAX];

	if (argc < 3 || argc > 4)
		usage();

	p->algorithm = string_fromcharstar(argv[2]);
	if (argc > 3) {
		size_t keylen;

		if (parse_size_t(argv[3], &keylen) == -1) {
			warn("failed to parse key length");
			return -1;
		}
		p->keylen = keylen;
	}

	ret = params_filldefaults(p);
	if (ret)
		return ret;

	fd = opendisk_werror(argv[0], cgdname, sizeof(cgdname));
	if (fd == -1)
		return -1;

	p->key = bits_fget(stdin, p->keylen);
	if (!p->key) {
		warnx("failed to read key from stdin");
		return -1;
	}

	return configure_params(fd, cgdname, argv[1], p);
}

static int
opendisk_werror(const char *cgd, char *buf, size_t buflen)
{
	int	fd;

	VPRINTF(3, ("opendisk_werror(%s, %s, %zu) called.\n", cgd, buf, buflen));

	/* sanity */
	if (!cgd || !buf)
		return -1;

	if (nflag) {
		if (strlcpy(buf, cgd, buflen) >= buflen)
			return -1;
		return 0;
	}

	fd = opendisk1(cgd, O_RDWR, buf, buflen, 0, cgd_kops.ko_open);
	if (fd == -1)
		warnx("can't open cgd \"%s\", \"%s\"", cgd, buf);

	return fd;
}

static int
configure_params(int fd, const char *cgd, const char *dev, struct params *p)
{
	struct cgd_ioctl ci;

	/* sanity */
	if (!cgd || !dev)
		return -1;

	(void)memset(&ci, 0x0, sizeof(ci));
	ci.ci_disk = dev;
	ci.ci_alg = string_tocharstar(p->algorithm);
	ci.ci_ivmethod = string_tocharstar(p->ivmeth);
	ci.ci_key = bits_getbuf(p->key);
	ci.ci_keylen = p->keylen;
	ci.ci_blocksize = p->bsize;

	VPRINTF(1, ("    with alg %s keylen %zu blocksize %zu ivmethod %s\n",
	    string_tocharstar(p->algorithm), p->keylen, p->bsize,
	    string_tocharstar(p->ivmeth)));
	VPRINTF(2, ("key: "));
	VERBOSE(2, bits_fprint(stdout, p->key));
	VPRINTF(2, ("\n"));

	if (nflag)
		return 0;

	if (cgd_kops.ko_ioctl(fd, CGDIOCSET, &ci) == -1) {
		int saved_errno = errno;
		warn("ioctl");
		return saved_errno;
	}

	return 0;
}

/*
 * verify returns 0 for success, -1 for unrecoverable error, or 1 for retry.
 */

#define SCANSIZE	8192

static int
verify(struct params *p, int fd)
{

	switch (p->verify_method) {
	case VERIFY_NONE:
		return 0;
	case VERIFY_DISKLABEL:
		return verify_disklabel(fd);
	case VERIFY_FFS:
		return verify_ffs(fd);
	case VERIFY_REENTER:
		return verify_reenter(p);
	default:
		warnx("unimplemented verification method");
		return -1;
	}
}

static int
verify_disklabel(int fd)
{
	struct	disklabel l;
	ssize_t	ret;
	char	buf[SCANSIZE];

	/*
	 * we simply scan the first few blocks for a disklabel, ignoring
	 * any MBR/filecore sorts of logic.  MSDOS and RiscOS can't read
	 * a cgd, anyway, so it is unlikely that there will be non-native
	 * partition information.
	 */

	ret = cgd_kops.ko_pread(fd, buf, 8192, 0);
	if (ret < 0) {
		warn("can't read disklabel area");
		return -1;
	}

	/* now scan for the disklabel */

	return disklabel_scan(&l, buf, (size_t)ret);
}

static off_t sblock_try[] = SBLOCKSEARCH;

static int
verify_ffs(int fd)
{
	size_t	i;

	for (i = 0; sblock_try[i] != -1; i++) {
		union {
		    char	buf[SBLOCKSIZE];
		    struct	fs fs;
		} u;
		ssize_t ret;

		ret = cgd_kops.ko_pread(fd, &u, sizeof(u), sblock_try[i]);
		if (ret < 0) {
			warn("pread");
			break;
		} else if ((size_t)ret < sizeof(u)) {
			warnx("pread: incomplete block");
			break;
		}
		switch (u.fs.fs_magic) {
		case FS_UFS1_MAGIC:
		case FS_UFS2_MAGIC:
		case FS_UFS1_MAGIC_SWAPPED:
		case FS_UFS2_MAGIC_SWAPPED:
			return 0;
		default:
			continue;
		}
	}

	return 1;	/* failure */
}

static int
verify_reenter(struct params *p)
{
	struct keygen *kg;
	bits_t *orig_key, *key;
	int ret;

	ret = 0;
	for (kg = p->keygen; kg && !ret; kg = kg->next) {
		if ((kg->kg_method != KEYGEN_PKCS5_PBKDF2_SHA1) && 
		    (kg->kg_method != KEYGEN_PKCS5_PBKDF2_OLD ))
			continue;

		orig_key = kg->kg_key;
		kg->kg_key = NULL;

		/* add a compat flag till the _OLD method goes away */
		key = getkey_pkcs5_pbkdf2("re-enter device", kg,
			bits_len(orig_key), kg->kg_method == KEYGEN_PKCS5_PBKDF2_OLD);
		ret = !bits_match(key, orig_key);

		bits_free(key);
		bits_free(kg->kg_key);
		kg->kg_key = orig_key;
	}

	return ret;
}

static int
generate(struct params *p, int argc, char **argv, const char *outfile)
{
	int	 ret;

	if (argc < 1 || argc > 2)
		usage();

	p->algorithm = string_fromcharstar(argv[0]);
	if (argc > 1) {
		size_t keylen;

		if (parse_size_t(argv[1], &keylen) == -1) {
			warn("Failed to parse key length");
			return -1;
		}
		p->keylen = keylen;
	}

	ret = params_filldefaults(p);
	if (ret)
		return ret;

	if (!p->keygen) {
		p->keygen = keygen_generate(KEYGEN_PKCS5_PBKDF2_SHA1);
		if (!p->keygen)
			return -1;
	}

	if (keygen_filldefaults(p->keygen, p->keylen)) {
		warnx("Failed to generate defaults for keygen");
		return -1;
	}

	if (!params_verify(p)) {
		warnx("invalid parameters generated");
		return -1;
	}

	return params_cput(p, outfile);
}

static int
generate_convert(struct params *p, int argc, char **argv, const char *outfile)
{
	struct params	*oldp;
	struct keygen	*kg;

	if (argc != 1)
		usage();

	oldp = params_cget(*argv);
	if (!oldp)
		return -1;

	/* for sanity, we ensure that none of the keygens are randomkey */
	for (kg=p->keygen; kg; kg=kg->next)
		if (kg->kg_method == KEYGEN_RANDOMKEY)
			goto bail;
	for (kg=oldp->keygen; kg; kg=kg->next)
		if (kg->kg_method == KEYGEN_RANDOMKEY)
			goto bail;

	if (!params_verify(oldp)) {
		warnx("invalid old parameters file \"%s\"", *argv);
		return -1;
	}

	oldp->key = getkey("old file", oldp->keygen, oldp->keylen);

	/* we copy across the non-keygen info, here. */

	string_free(p->algorithm);
	string_free(p->ivmeth);

	p->algorithm = string_dup(oldp->algorithm);
	p->ivmeth = string_dup(oldp->ivmeth);
	p->keylen = oldp->keylen;
	p->bsize = oldp->bsize;
	if (p->verify_method == VERIFY_UNKNOWN)
		p->verify_method = oldp->verify_method;

	params_free(oldp);

	if (!p->keygen) {
		p->keygen = keygen_generate(KEYGEN_PKCS5_PBKDF2_SHA1);
		if (!p->keygen)
			return -1;
	}
	(void)params_filldefaults(p);
	(void)keygen_filldefaults(p->keygen, p->keylen);
	p->key = getkey("new file", p->keygen, p->keylen);

	kg = keygen_generate(KEYGEN_STOREDKEY);
	kg->kg_key = bits_xor(p->key, oldp->key);
	keygen_addlist(&p->keygen, kg);

	if (!params_verify(p)) {
		warnx("can't generate new parameters file");
		return -1;
	}

	return params_cput(p, outfile);
bail:
	params_free(oldp);
	return -1;
}

static int
/*ARGSUSED*/
do_all(const char *cfile, int argc, char **argv,
       int (*conf)(int, char **, struct params *, int))
{
	FILE		 *f;
	size_t		  len;
	size_t		  lineno;
	int		  my_argc;
	int		  ret;
	const char	 *fn;
	char		 *line;
	char		**my_argv;

	if (argc > 0)
		usage();

	if (!cfile[0])
		fn = CGDCONFIG_CFILE;
	else
		fn = cfile;

	f = fopen(fn, "r");
	if (!f) {
		warn("could not open config file \"%s\"", fn);
		return -1;
	}

	ret = chdir(CGDCONFIG_DIR);
	if (ret == -1)
		warn("could not chdir to %s", CGDCONFIG_DIR);

	ret = 0;
	lineno = 0;
	for (;;) {
		line = fparseln(f, &len, &lineno, "\\\\#", FPARSELN_UNESCALL);
		if (!line)
			break;
		if (!*line)
			continue;

		my_argv = words(line, &my_argc);
		ret = conf(my_argc, my_argv, NULL, CONFIG_FLAGS_FROMALL);
		if (ret) {
			warnx("action failed on \"%s\" line %lu", fn,
			    (u_long)lineno);
			break;
		}
		words_free(my_argv, my_argc);
	}
	return ret;
}

static void
eliminate_cores(void)
{
	struct rlimit	rlp;

	rlp.rlim_cur = 0;
	rlp.rlim_max = 0;
	if (setrlimit(RLIMIT_CORE, &rlp) == -1)
		err(EXIT_FAILURE, "Can't disable cores");
}
