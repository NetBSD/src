/* $NetBSD: cgdconfig.c,v 1.4 2002/10/28 05:46:01 elric Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
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

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT(
"@(#) Copyright (c) 2002\
	The NetBSD Foundation, Inc.  All rights reserved.");
__RCSID("$NetBSD: cgdconfig.c,v 1.4 2002/10/28 05:46:01 elric Exp $");
#endif

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <sys/ioctl.h>
#include <sys/disklabel.h>
#include <sys/param.h>

#include <dev/cgdvar.h>

#include "params.h"
#include "pkcs5_pbkdf2.h"
#include "utils.h"

#define CGDCONFIG_DIR		"/etc/cgd"
#define CGDCONFIG_CFILE		CGDCONFIG_DIR "/cgd.conf"
#define DEFAULT_SALTLEN		128

#define ACTION_CONFIGURE	0x1	/* configure, with paramsfile */
#define ACTION_UNCONFIGURE	0x2	/* unconfigure */
#define ACTION_GENERATE		0x3	/* generate a paramsfile */
#define ACTION_CONFIGALL	0x4	/* configure all from config file */
#define ACTION_UNCONFIGALL	0x5	/* unconfigure all from config file */
#define ACTION_CONFIGSTDIN	0x6	/* configure, key from stdin */

/* if nflag is set, do not configure/unconfigure the cgd's */

int	nflag = 0;

static int	configure(int, char **, struct params *, int);
static int	configure_stdin(struct params *, int argc, char **);
static int	generate(struct params *, int, char **, const char *);
static int	unconfigure(int, char **, struct params *, int);
static int	do_all(const char *, int, char **,
		       int (*)(int, char **, struct params *, int));

#define CONFIG_FLAGS_FROMALL	1	/* called from configure_all() */
#define CONFIG_FLAGS_FROMMAIN	2	/* called from main() */

static int	 configure_params(int, const char *, const char *,
				  struct params *);
static void	 key_print(FILE *, const u_int8_t *, int);
static char	*getrandbits(int);
static int	 getkey(const char *, struct params *);
static int	 getkeyfrompassphrase(const char *, struct params *);
static int	 getkeyfromfile(FILE *, struct params *);
static int	 opendisk_werror(const char *, char *, int);
static int	 unconfigure_fd(int);
static int	 verify(struct params *, int);

static void	 usage(void);

/* Verbose Framework */
int	verbose = 0;

#define VERBOSE(x,y)	if (verbose >= x) y
#define VPRINTF(x,y)	if (verbose >= x) printf y

static void
usage(void)
{

	fprintf(stderr, "usage: %s [-nv] [-V vmeth] cgd dev [paramsfile]\n",
	    getprogname());
	fprintf(stderr, "       %s -C [-nv] [-f configfile]\n", getprogname());
	fprintf(stderr, "       %s -U [-nv] [-f configfile]\n", getprogname());
	fprintf(stderr, "       %s -g [-nv] [-i ivmeth] [-k kgmeth] "
	    "[-o outfile] [-V vmeth] alg [keylen]\n", getprogname());
	fprintf(stderr, "       %s -s [-nv] [-i ivmeth] cgd dev alg "
	    "[keylen]\n", getprogname());
	fprintf(stderr, "       %s -u [-nv] cgd\n", getprogname());
	exit(1);
}

int
main(int argc, char **argv)
{
	struct params cf;
	int	action = ACTION_CONFIGURE;
	int	actions = 0;
	int	ch;
	int	ret;
	char	cfile[FILENAME_MAX] = "";
	char	outfile[FILENAME_MAX] = "";

	setprogname(*argv);
	params_init(&cf);

	while ((ch = getopt(argc, argv, "CUV:b:f:gi:k:no:usv")) != -1)
		switch (ch) {
		case 'C':
			action = ACTION_CONFIGALL;
			actions++;
			break;
		case 'U':
			action = ACTION_UNCONFIGALL;
			actions++;
			break;
		case 'V':
			ret = params_setverify_method_str(&cf, optarg);
			if (ret)
				usage();
			break;
		case 'b':
			ret = params_setbsize(&cf, atoi(optarg));
			if (ret)
				usage();
			break;
		case 'f':
			strncpy(cfile, optarg, FILENAME_MAX);
			break;
		case 'g':
			action = ACTION_GENERATE;
			actions++;
			break;
		case 'i':
			params_setivmeth(&cf, optarg);
			break;
		case 'k':
			ret = params_setkeygen_method_str(&cf, optarg);
			if (ret)
				usage();
			break;
		case 'n':
			nflag = 1;
			break;
		case 'o':
			strncpy(outfile, optarg, FILENAME_MAX);
			break;
		case 's':
			action = ACTION_CONFIGSTDIN;
			actions++;
			break;

		case 'u':
			action = ACTION_UNCONFIGURE;
			actions++;
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

	/* validate the consistency of the arguments */

	if (actions > 1)
		usage();
	if (action == ACTION_CONFIGURE && params_changed(&cf))
		usage();

	switch (action) {
	case ACTION_CONFIGURE:
		return configure(argc, argv, &cf, CONFIG_FLAGS_FROMMAIN);
	case ACTION_UNCONFIGURE:
		return unconfigure(argc, argv, NULL, CONFIG_FLAGS_FROMMAIN);
	case ACTION_GENERATE:
		return generate(&cf, argc, argv, outfile);
	case ACTION_CONFIGALL:
		return do_all(cfile, argc, argv, configure);
	case ACTION_UNCONFIGALL:
		return do_all(cfile, argc, argv, unconfigure);
	case ACTION_CONFIGSTDIN:
		return configure_stdin(&cf, argc, argv);
	default:
		fprintf(stderr, "undefined action\n");
		return 1;
	}
	/* NOTREACHED */
}

static int
getkey(const char *target, struct params *p)
{

	switch (p->keygen_method) {
	case KEYGEN_RANDOMKEY:
		p->key = getrandbits(p->keylen);
		if (!p->key)
			return -1;
		return 0;
	case KEYGEN_PKCS5_PBKDF2:
		return getkeyfrompassphrase(target, p);
	default:
		fprintf(stderr, "getkey: unknown keygen_method\n");
		return -1;
	}
	/* NOTREACHED */
}

static int
getkeyfromfile(FILE *f, struct params *p)
{
	int	ret;

	/* XXXrcd: data hiding? */
	p->key = malloc(p->keylen);
	if (!p->key)
		return -1;
	ret = fread(p->key, p->keylen, 1, f);
	if (ret < 1) {
		fprintf(stderr, "failed to read key from stdin\n");
		return -1;
	}
	return 0;
}

static int
getkeyfrompassphrase(const char *target, struct params *p)
{
	int	 ret;
	char	*passp;
	char	 buf[1024];

	snprintf(buf, 1024, "%s's passphrase:", target);
	passp = getpass(buf);
	/* XXXrcd: data hiding ? we should be allocating the key here. */
	if (!p->key)
		free(p->key);
	ret = pkcs5_pbkdf2(&p->key, BITS2BYTES(p->keylen), passp,
	    strlen(passp), p->keygen_salt, BITS2BYTES(p->keygen_saltlen),
	    p->keygen_iterations);
	if (p->xor_key)
		memxor(p->key, p->xor_key, BITS2BYTES(p->keylen));
	return ret;
}

/* ARGSUSED */
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

	fd = opendisk(*argv, O_RDWR, buf, sizeof(buf), 1);
	if (fd == -1) {
		fprintf(stderr, "can't open cgd \"%s\", \"%s\": %s\n",
		    *argv, buf, strerror(errno));

		/* this isn't fatal with nflag != 0 */
		if (!nflag)
			return errno;
	}

	VPRINTF(1, ("%s (%s): clearing\n", *argv, buf));

	if (nflag)
		return 0;

	ret = unconfigure_fd(fd);
	close(fd);
	return ret;
}

static int
unconfigure_fd(int fd)
{
	struct	cgd_ioctl ci;
	int	ret;

	ret = ioctl(fd, CGDIOCCLR, &ci);
	if (ret == -1) {
		perror("ioctl");
		return -1;
	}

	return 0;
}

/* ARGSUSED */
static int
configure(int argc, char **argv, struct params *inparams, int flags)
{
	struct params	params;
	int		fd;
	int		ret;
	char		pfile[FILENAME_MAX];
	char		cgdname[PATH_MAX];

	params_init(&params);

	switch (argc) {
	case 2:
		strlcpy(pfile, CGDCONFIG_DIR, FILENAME_MAX);
		strlcat(pfile, "/", FILENAME_MAX);
		strlcat(pfile, basename(argv[1]), FILENAME_MAX);
		break;
	case 3:
		strlcpy(pfile, argv[2], FILENAME_MAX);
		break;
	default:
		/* print usage and exit, only if called from main() */
		if (flags == CONFIG_FLAGS_FROMMAIN)
			usage();
		return -1;
		/* NOTREACHED */
	}

	ret = params_cget(&params, pfile);
	if (ret)
		return ret;
	ret = params_filldefaults(&params);
	if (ret)
		return ret;

	/*
	 * over-ride the verify method with that specified on the
	 * command line
	 */

	if (inparams && inparams->verify_method != VERIFY_UNKNOWN)
		params.verify_method = inparams->verify_method;

	/*
	 * loop over configuring the disk and checking to see if it
	 * verifies properly.  We open and close the disk device each
	 * time, because if the user passes us the block device we
	 * need to flush the buffer cache.
	 */

	for (;;) {
		fd = opendisk_werror(argv[0], cgdname, sizeof(cgdname));
		if (fd == -1)
			return -1;

		ret = getkey(argv[1], &params);
		if (ret)
			goto bail_err;

		ret = configure_params(fd, cgdname, argv[1], &params);
		if (ret)
			goto bail_err;

		ret = verify(&params, fd);
		if (ret == -1)
			goto bail_err;
		if (!ret)
			break;

		fprintf(stderr, "verification failed, please reenter "
		    "passphrase\n");

		unconfigure_fd(fd);
		close(fd);
	}

	params_free(&params);
	close(fd);
	return 0;
bail_err:
	close(fd);
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

	ret = params_setalgorithm(p, argv[2]);
	if (ret)
		return ret;
	if (argc > 3) {
		ret = params_setkeylen(p, atoi(argv[3]));
		if (ret)
			return ret;
	}

	ret = params_filldefaults(p);
	if (ret)
		return ret;

	fd = opendisk_werror(argv[0], cgdname, sizeof(cgdname));
	if (fd == -1)
		return -1;

	ret = getkeyfromfile(stdin, p);
	if (ret)
		return -1;

	return configure_params(fd, cgdname, argv[1], p);
}

static int
opendisk_werror(const char *cgd, char *buf, int buflen)
{
	int	fd;

	/* sanity */
	if (!cgd || !buf)
		return -1;

	if (nflag) {
		strncpy(buf, cgd, buflen);
		return 0;
	}

	fd = opendisk(cgd, O_RDWR, buf, buflen, 0);
	if (fd == -1)
		fprintf(stderr, "can't open cgd \"%s\", \"%s\": %s\n",
		    cgd, buf, strerror(errno));
	return fd;
}

static int
configure_params(int fd, const char *cgd, const char *dev, struct params *p)
{
	struct cgd_ioctl ci;
	int	  ret;

	/* sanity */
	if (!cgd || !dev)
		return -1;

	memset(&ci, 0x0, sizeof(ci));
	ci.ci_disk = (char *)dev;
	ci.ci_alg = p->alg;
	ci.ci_ivmethod = p->ivmeth;
	ci.ci_key = p->key;
	ci.ci_keylen = p->keylen;
	ci.ci_blocksize = p->bsize;

	VPRINTF(1, ("attaching: %s attach to %s\n", cgd, dev));
	VPRINTF(1, ("    with alg %s keylen %d blocksize %d ivmethod %s\n",
	    p->alg, p->keylen, p->bsize, p->ivmeth));
	VERBOSE(2, key_print(stdout, p->key, p->keylen));

	if (nflag)
		return 0;

	ret = ioctl(fd, CGDIOCSET, &ci);
	if (ret == -1) {
		perror("ioctl");
		return errno;
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
	struct	disklabel l;
	int	ret;
	char	buf[SCANSIZE];

	switch (p->verify_method) {
	case VERIFY_NONE:
		return 0;
	case VERIFY_DISKLABEL:
		/*
		 * for now this is the only method, so we just perform it
		 * in this function.
		 */
		break;
	default:
		fprintf(stderr, "verify: unimplemented verification method\n");
		return -1;
	}

	/*
	 * we simply scan the first few blocks for a disklabel, ignoring
	 * any MBR/filecore sorts of logic.  MSDOS and RiscOS can't read
	 * a cgd, anyway, so it is unlikely that there will be non-native
	 * partition information.
	 */

	ret = pread(fd, buf, 8192, 0);
	if (ret == -1) {
		fprintf(stderr, "verify: can't read disklabel area\n");
		return -1;
	}

	/* now scan for the disklabel */

	return disklabel_scan(&l, buf, sizeof(buf));
}

static int
generate(struct params *p, int argc, char **argv, const char *outfile)
{
	FILE	*f;
	int	 ret;
	char	*tmp;

	if (argc < 1 || argc > 2)
		usage();

	ret = params_setalgorithm(p, argv[0]);
	if (ret)
		return ret;
	if (argc > 1) {
		ret = params_setkeylen(p, atoi(argv[1]));
		if (ret)
			return ret;
	}

	ret = params_filldefaults(p);
	if (ret)
		return ret;

	if (p->keygen_method != KEYGEN_RANDOMKEY) {
		tmp = getrandbits(DEFAULT_SALTLEN);
		params_setkeygen_salt(p, tmp, DEFAULT_SALTLEN);
		free(tmp);
		tmp = getrandbits(p->keylen);
		params_setxor_key(p, tmp, p->keylen);
		free(tmp);

		/* XXXrcd: generate key hash, if desired */
	}

	if (*outfile) {
		f = fopen(outfile, "w");
		if (!f) {
			fprintf(stderr, "could not open outfile \"%s\": %s\n",
			    outfile, strerror(errno));
			perror("fopen");
			return -1;
		}
	} else {
		f = stdout;
	}

	ret = params_fput(p, f);
	params_free(p);
	return ret;
}

static int
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
		fprintf(stderr, "could not open config file \"%s\": %s\n",
		    fn, strerror(errno));
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
			fprintf(stderr, "on \"%s\" line %lu\n", fn,
			    (u_long)lineno);
			break;
		}
		words_free(my_argv, my_argc);
	}
	return ret;
}

/*
 * XXX: key_print doesn't work quite exactly properly if the keylength
 *      is not evenly divisible by 8.  If the key is not divisible by
 *      8 then a few extra bits are printed.
 */

static void
key_print(FILE *f, const u_int8_t *key, int len)
{
	int	i;
	int	col;

	len = BITS2BYTES(len);
	fprintf(f, "key: ");
	for (i=0, col=5; i < len; i++, col+=2) {
		fprintf(f, "%02x", key[i]);
		if (col > 70) {
			col = 5 - 2;
			fprintf(f, "\n     ");
		}
	}
	fprintf(f, "\n");
}

static char *
getrandbits(int len)
{
	FILE	*f;
	int	 ret;
	char	*res;

	len = (len + 7) / 8;
	res = malloc(len);
	if (!res)
		return NULL;
	f = fopen("/dev/random", "r");
	if (!f)
		return NULL;
	ret = fread(res, len, 1, f);
	if (ret != 1) {
		free(res);
		return NULL;
	}
	return res;
}
