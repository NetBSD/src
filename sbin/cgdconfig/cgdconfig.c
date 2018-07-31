/* $NetBSD: cgdconfig.c,v 1.41.6.1 2018/07/31 16:01:12 martin Exp $ */

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
__RCSID("$NetBSD: cgdconfig.c,v 1.41.6.1 2018/07/31 16:01:12 martin Exp $");
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
#include <paths.h>
#include <dirent.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/bootblock.h>
#include <sys/disklabel.h>
#include <sys/disklabel_gpt.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/statvfs.h>
#include <sys/bitops.h>

#include <dev/cgdvar.h>

#include <ufs/ffs/fs.h>

#include "params.h"
#include "pkcs5_pbkdf2.h"
#include "utils.h"
#include "cgdconfig.h"
#include "prog_ops.h"

#define CGDCONFIG_CFILE		CGDCONFIG_DIR "/cgd.conf"

enum action {
	 ACTION_DEFAULT,		/* default -> configure */
	 ACTION_CONFIGURE,		/* configure, with paramsfile */
	 ACTION_UNCONFIGURE,		/* unconfigure */
	 ACTION_GENERATE,		/* generate a paramsfile */
	 ACTION_GENERATE_CONVERT,	/* generate a ``dup'' paramsfile */
	 ACTION_CONFIGALL,		/* configure all from config file */
	 ACTION_UNCONFIGALL,		/* unconfigure all from config file */
	 ACTION_CONFIGSTDIN,		/* configure, key from stdin */
	 ACTION_LIST			/* list configured devices */
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
static int	do_list(int, char **);

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
static int	 verify_mbr(int);
static int	 verify_gpt(int);

__dead static void	 usage(void);

/* Verbose Framework */
unsigned	verbose = 0;

#define VERBOSE(x,y)	if (verbose >= x) y
#define VPRINTF(x,y)	if (verbose >= x) (void)printf y

static void
usage(void)
{

	(void)fprintf(stderr, "usage: %s [-npv] [-V vmeth] cgd dev "
	    "[paramsfile]\n", getprogname());
	(void)fprintf(stderr, "       %s -C [-npv] [-f configfile]\n",
	    getprogname());
	(void)fprintf(stderr, "       %s -G [-npv] [-i ivmeth] [-k kgmeth] "
	    "[-o outfile] paramsfile\n", getprogname());
	(void)fprintf(stderr, "       %s -g [-nv] [-i ivmeth] [-k kgmeth] "
	    "[-o outfile] alg [keylen]\n", getprogname());
	(void)fprintf(stderr, "       %s -l [-v[v]] [cgd]\n", getprogname());
	(void)fprintf(stderr, "       %s -s [-nv] [-i ivmeth] cgd dev alg "
	    "[keylen]\n", getprogname());
	(void)fprintf(stderr, "       %s -U [-nv] [-f configfile]\n",
	    getprogname());
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

int
main(int argc, char **argv)
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

	while ((ch = getopt(argc, argv, "CGUV:b:f:gi:k:lno:spuv")) != -1)
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
		case 'l':
			set_action(&action, ACTION_LIST);
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

	if (prog_init && prog_init() == -1)
		err(1, "init failed");

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
	case ACTION_LIST:
		return do_list(argc, argv);
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

	fd = opendisk1(*argv, O_RDWR, buf, sizeof(buf), 1, prog_open);
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
	(void)prog_close(fd);
	return ret;
}

static int
unconfigure_fd(int fd)
{
	struct	cgd_ioctl ci;

	if (prog_ioctl(fd, CGDIOCCLR, &ci) == -1) {
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
	char		 devicename[PATH_MAX];
	const char	*dev = NULL;	/* XXX: gcc */

	if (argc < 2 || argc > 3) {
		/* print usage and exit, only if called from main() */
		if (flags == CONFIG_FLAGS_FROMMAIN) {
			warnx("wrong number of args");
			usage();
		}
		return -1;
	}

	if ((
	  fd = opendisk1(*argv, O_RDWR, cgdname, sizeof(cgdname), 1, prog_open)
	    ) != -1) {
		struct cgd_user cgu;

		cgu.cgu_unit = -1;
		if (prog_ioctl(fd, CGDIOCGET, &cgu) != -1 && cgu.cgu_dev != 0) {
			warnx("device %s already in use", *argv);
			prog_close(fd);
			return -1;
		}
		prog_close(fd);
	}

	dev = getfsspecname(devicename, sizeof(devicename), argv[1]);
	if (dev == NULL) {
		warnx("getfsspecname failed: %s", devicename);
		return -1;
	}

	if (argc == 2) {
		char pfile[MAXPATHLEN];

		/* make string writable for basename */
		strlcpy(pfile, dev, sizeof(pfile));
		p = params_cget(basename(pfile));
	} else
		p = params_cget(argv[2]);

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

		ret = configure_params(fd, cgdname, dev, p);
		if (ret)
			goto bail_err;

		ret = verify(p, fd);
		if (ret == -1)
			goto bail_err;
		if (!ret)
			break;

		(void)unconfigure_fd(fd);
		(void)prog_close(fd);

		if (!loop) {
			warnx("verification failed permanently");
			goto bail_err;
		}

		warnx("verification failed, please reenter passphrase");
	}

	params_free(p);
	(void)prog_close(fd);
	return 0;
bail_err:
	params_free(p);
	(void)prog_close(fd);
	return -1;
}

static int
configure_stdin(struct params *p, int argc, char **argv)
{
	int		 fd;
	int		 ret;
	char		 cgdname[PATH_MAX];
	char		 devicename[PATH_MAX];
	const char	*dev;

	if (argc < 3 || argc > 4)
		usage();

	dev = getfsspecname(devicename, sizeof(devicename), argv[1]);
	if (dev == NULL) {
		warnx("getfsspecname failed: %s", devicename);
		return -1;
	}

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

	return configure_params(fd, cgdname, dev, p);
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

	fd = opendisk1(cgd, O_RDWR, buf, buflen, 0, prog_open);
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

	if (prog_ioctl(fd, CGDIOCSET, &ci) == -1) {
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
	case VERIFY_MBR:
		return verify_mbr(fd);
	case VERIFY_GPT:
		return verify_gpt(fd);
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

	ret = prog_pread(fd, buf, SCANSIZE, 0);
	if (ret < 0) {
		warn("can't read disklabel area");
		return -1;
	}

	/* now scan for the disklabel */

	return disklabel_scan(&l, buf, (size_t)ret);
}

static int
verify_mbr(int fd)
{
	struct mbr_sector mbr;
	ssize_t	ret;
	char	buf[SCANSIZE];

	/*
	 * we read the first blocks to avoid sector size issues and
	 * verify the MBR in the beginning
	 */

	ret = prog_pread(fd, buf, SCANSIZE, 0);
	if (ret < 0) {
		warn("can't read mbr area");
		return -1;
	}

	memcpy(&mbr, buf, sizeof(mbr));
	if (le16toh(mbr.mbr_magic) != MBR_MAGIC)
		return -1;

	return 0;
}

static uint32_t crc32_tab[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static uint32_t
crc32(const void *buf, size_t size)
{
	const uint8_t *p;
	uint32_t crc;

	p = buf;
	crc = ~0U;

	while (size--)
		crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

	return crc ^ ~0U;
}

static int
verify_gpt(int fd)
{
	struct	 gpt_hdr hdr;
	ssize_t	 ret;
	char	 buf[SCANSIZE];
	unsigned blksize;
	size_t	 off;

	/*
	 * we read the first blocks to avoid sector size issues and
	 * verify the GPT header.
	 */

	ret = prog_pread(fd, buf, SCANSIZE, 0);
	if (ret < 0) {
		warn("can't read gpt area");
		return -1;
	}

	ret = -1;
	for (blksize=DEV_BSIZE;
             (off = blksize * GPT_HDR_BLKNO) <= SCANSIZE - sizeof(hdr);
             blksize <<= 1) {

		memcpy(&hdr, &buf[off], sizeof(hdr));
		if (memcmp(hdr.hdr_sig, GPT_HDR_SIG, sizeof(hdr.hdr_sig)) == 0 &&
		    le32toh(hdr.hdr_revision) == GPT_HDR_REVISION &&
		    le32toh(hdr.hdr_size) == GPT_HDR_SIZE) {

			hdr.hdr_crc_self = 0;
			if (crc32(&hdr, sizeof(hdr))) {
				ret = 0;
				break;
			}
		}
	}

	return ret;
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

		ret = prog_pread(fd, &u, sizeof(u), sblock_try[i]);
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
		if ((kg->kg_method == KEYGEN_RANDOMKEY) ||
		    (kg->kg_method == KEYGEN_URANDOMKEY)) {
			warnx("can't preserve randomly generated key");
			goto bail;
		}
	for (kg=oldp->keygen; kg; kg=kg->next)
		if ((kg->kg_method == KEYGEN_RANDOMKEY) ||
		    (kg->kg_method == KEYGEN_URANDOMKEY)) {
			warnx("can't preserve randomly generated key");
			goto bail;
		}

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

static const char *
iv_method(int mode)
{

	switch (mode) {
	case CGD_CIPHER_CBC_ENCBLKNO8:
		return "encblkno8";
	case CGD_CIPHER_CBC_ENCBLKNO1:
		return "encblkno1";
	default:
		return "unknown";
	}
}


static void
show(const char *dev) {
	char path[64];
	struct cgd_user cgu;
	int fd;

	fd = opendisk(dev, O_RDONLY, path, sizeof(path), 0);
	if (fd == -1) {
		warn("open: %s", dev);
		return;
	}

	cgu.cgu_unit = -1;
	if (prog_ioctl(fd, CGDIOCGET, &cgu) == -1) {
		close(fd);
		err(1, "CGDIOCGET");
	}

	printf("%s: ", dev);

	if (cgu.cgu_dev == 0) {
		printf("not in use");
		goto out;
	}

	dev = devname(cgu.cgu_dev, S_IFBLK);
	if (dev != NULL)
		printf("%s ", dev);
	else
		printf("dev %llu,%llu ", (unsigned long long)major(cgu.cgu_dev),
		    (unsigned long long)minor(cgu.cgu_dev));

	if (verbose)
		printf("%s ", cgu.cgu_alg);
	if (verbose > 1) {
		printf("keylen %d ", cgu.cgu_keylen);
		printf("blksize %zd ", cgu.cgu_blocksize);
		printf("%s ", iv_method(cgu.cgu_mode));
	}

out:
	putchar('\n');
	close(fd);
}

static int
do_list(int argc, char **argv)
{

	if (argc != 0 && argc != 1)
		usage();

	if (argc) {
		show(argv[0]);
		return 0;
	}

	DIR *dirp;
	struct dirent *dp;
	__BITMAP_TYPE(, uint32_t, 65536) bm;

	__BITMAP_ZERO(&bm);

	if ((dirp = opendir(_PATH_DEV)) == NULL)
		err(1, "opendir: %s", _PATH_DEV);

	while ((dp = readdir(dirp)) != NULL) {
		char *ep;
		if (strncmp(dp->d_name, "rcgd", 4) != 0)
			continue;
		errno = 0;
		int n = (int)strtol(dp->d_name + 4, &ep, 0);
		if (ep == dp->d_name + 4 || errno != 0) {
			warnx("bad name %s", dp->d_name);
			continue;
		}
		*ep = '\0';
		if (__BITMAP_ISSET(n, &bm))
			continue;
		__BITMAP_SET(n, &bm);
		show(dp->d_name + 1);
	}

	closedir(dirp);
	return 0;
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
