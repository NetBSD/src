/* $NetBSD: cgdconfig.c,v 1.6 2003/03/24 03:12:22 elric Exp $ */

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
"@(#) Copyright (c) 2002, 2003\
	The NetBSD Foundation, Inc.  All rights reserved.");
__RCSID("$NetBSD: cgdconfig.c,v 1.6 2003/03/24 03:12:22 elric Exp $");
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
#include <sys/param.h>

#include <dev/cgdvar.h>

#include <ufs/ffs/fs.h>

#include "params.h"
#include "pkcs5_pbkdf2.h"
#include "utils.h"

#define CGDCONFIG_DIR		"/etc/cgd"
#define CGDCONFIG_CFILE		CGDCONFIG_DIR "/cgd.conf"

#define ACTION_CONFIGURE	0x1	/* configure, with paramsfile */
#define ACTION_UNCONFIGURE	0x2	/* unconfigure */
#define ACTION_GENERATE		0x3	/* generate a paramsfile */
#define ACTION_GENERATE_CONVERT	0x4	/* generate a ``dup'' paramsfile */
#define ACTION_CONFIGALL	0x5	/* configure all from config file */
#define ACTION_UNCONFIGALL	0x6	/* unconfigure all from config file */
#define ACTION_CONFIGSTDIN	0x7	/* configure, key from stdin */

/* if nflag is set, do not configure/unconfigure the cgd's */

int	nflag = 0;

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
static bits_t	*getkey(const char *, struct keygen *, int);
static bits_t	*getkey_storedkey(const char *, struct keygen *, int);
static bits_t	*getkey_randomkey(const char *, struct keygen *, int);
static bits_t	*getkey_pkcs5_pbkdf2(const char *, struct keygen *, int);
static int	 opendisk_werror(const char *, char *, int);
static int	 unconfigure_fd(int);
static int	 verify(struct params *, int);
static int	 verify_disklabel(int);
static int	 verify_ffs(int);

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
	fprintf(stderr, "       %s -G [-nv] [-i ivmeth] [-k kgmeth] "
	    "[-o outfile] paramsfile\n", getprogname());
	fprintf(stderr, "       %s -g [-nv] [-i ivmeth] [-k kgmeth] "
	    "[-o outfile] alg [keylen]\n", getprogname());
	fprintf(stderr, "       %s -s [-nv] [-i ivmeth] cgd dev alg "
	    "[keylen]\n", getprogname());
	fprintf(stderr, "       %s -u [-nv] cgd\n", getprogname());
	exit(1);
}

int
main(int argc, char **argv)
{
	struct params *p;
	struct params *tp;
	struct keygen *kg;
	int	action = ACTION_CONFIGURE;
	int	actions = 0;
	int	ch;
	char	cfile[FILENAME_MAX] = "";
	char	outfile[FILENAME_MAX] = "";

	setprogname(*argv);
	p = params_new();
	kg = NULL;

	while ((ch = getopt(argc, argv, "CGUV:b:f:gi:k:no:usv")) != -1)
		switch (ch) {
		case 'C':
			action = ACTION_CONFIGALL;
			actions++;
			break;
		case 'G':
			action = ACTION_GENERATE_CONVERT;
			actions++;
			break;
		case 'U':
			action = ACTION_UNCONFIGALL;
			actions++;
			break;
		case 'V':
			tp = params_verify_method(string_fromcharstar(optarg));
			if (!tp)
				usage();
			p = params_combine(p, tp);
			break;
		case 'b':
			tp = params_bsize(atoi(optarg));
			if (!tp)
				usage();
			p = params_combine(p, tp);
			break;
		case 'f':
			strncpy(cfile, optarg, FILENAME_MAX);
			break;
		case 'g':
			action = ACTION_GENERATE;
			actions++;
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

	switch (action) {
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
	}
	/* NOTREACHED */
}

static bits_t *
getkey(const char *dev, struct keygen *kg, int len)
{
	bits_t	*ret = NULL;
	bits_t	*tmp;

	VPRINTF(3, ("getkey(\"%s\", %p, %d) called\n", dev, kg, len));
	for (; kg; kg=kg->next) {
		switch (kg->kg_method) {
		case KEYGEN_STOREDKEY:
			tmp = getkey_storedkey(dev, kg, len);
			break;
		case KEYGEN_RANDOMKEY:
			tmp = getkey_randomkey(dev, kg, len);
			break;
		case KEYGEN_PKCS5_PBKDF2:
			tmp = getkey_pkcs5_pbkdf2(dev, kg, len);
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
getkey_storedkey(const char *target, struct keygen *kg, int keylen)
{

	return bits_dup(kg->kg_key);
}

/*ARGSUSED*/
static bits_t *
getkey_randomkey(const char *target, struct keygen *kg, int keylen)
{

	return bits_getrandombits(keylen);
}

/*ARGSUSED*/
static bits_t *
getkey_pkcs5_pbkdf2(const char *target, struct keygen *kg, int keylen)
{
	bits_t		*ret;
	char		*passp;
	char		 buf[1024];
	u_int8_t	*tmp;

	snprintf(buf, sizeof(buf), "%s's passphrase:", target);
	passp = getpass(buf);
	if (pkcs5_pbkdf2(&tmp, BITS2BYTES(keylen), passp, strlen(passp),
	    bits_getbuf(kg->kg_salt), BITS2BYTES(bits_len(kg->kg_salt)),
	    kg->kg_iterations)) {
		warnx("failed to generate PKCS#5 PBKDF2 key");
		return NULL;
	}

	ret = bits_new(tmp, keylen);
	free(tmp);
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

	fd = opendisk(*argv, O_RDWR, buf, sizeof(buf), 1);
	if (fd == -1) {
		warn("can't open cgd \"%s\", \"%s\"", *argv, buf);

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

/*ARGSUSED*/
static int
configure(int argc, char **argv, struct params *inparams, int flags)
{
	struct params	*p;
	int		 fd;
	int		 ret;
	char		 pfile[FILENAME_MAX];
	char		 cgdname[PATH_MAX];

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
		if (flags == CONFIG_FLAGS_FROMMAIN) {
			warnx("wrong number of args");
			usage();
		}
		return -1;
		/* NOTREACHED */
	}

	p = params_cget(pfile);
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
	 */

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

		fprintf(stderr, "verification failed, please reenter "
		    "passphrase\n");

		unconfigure_fd(fd);
		close(fd);
	}

	params_free(p);
	close(fd);
	return 0;
bail_err:
	params_free(p);
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

	p->algorithm = string_fromcharstar(argv[2]);
	if (argc > 3)
		p->keylen = atoi(argv[3]);

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
opendisk_werror(const char *cgd, char *buf, int buflen)
{
	int	fd;

	VPRINTF(3, ("opendisk_werror(%s, %s, %d) called.\n", cgd, buf, buflen));

	/* sanity */
	if (!cgd || !buf)
		return -1;

	if (nflag) {
		strncpy(buf, cgd, buflen);
		return 0;
	}

	fd = opendisk(cgd, O_RDWR, buf, buflen, 0);
	if (fd == -1)
		warnx("can't open cgd \"%s\", \"%s\"", cgd, buf);

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
	ci.ci_alg = (char *)string_tocharstar(p->algorithm);
	ci.ci_ivmethod = (char *)string_tocharstar(p->ivmeth);
	ci.ci_key = (char *)bits_getbuf(p->key);
	ci.ci_keylen = p->keylen;
	ci.ci_blocksize = p->bsize;

	VPRINTF(1, ("    with alg %s keylen %d blocksize %d ivmethod %s\n",
	    string_tocharstar(p->algorithm), p->keylen, p->bsize,
	    string_tocharstar(p->ivmeth)));
	VPRINTF(2, ("key: "));
	VERBOSE(2, bits_fprint(stdout, p->key));
	VPRINTF(2, ("\n"));

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

	switch (p->verify_method) {
	case VERIFY_NONE:
		return 0;
	case VERIFY_DISKLABEL:
		return verify_disklabel(fd);
	case VERIFY_FFS:
		return verify_ffs(fd);
	default:
		warnx("unimplemented verification method");
		return -1;
	}
}

static int
verify_disklabel(int fd)
{
	struct	disklabel l;
	int	ret;
	char	buf[SCANSIZE];

	/*
	 * we simply scan the first few blocks for a disklabel, ignoring
	 * any MBR/filecore sorts of logic.  MSDOS and RiscOS can't read
	 * a cgd, anyway, so it is unlikely that there will be non-native
	 * partition information.
	 */

	ret = pread(fd, buf, 8192, 0);
	if (ret == -1) {
		warn("can't read disklabel area");
		return -1;
	}

	/* now scan for the disklabel */

	return disklabel_scan(&l, buf, sizeof(buf));
}

static int
verify_ffs(int fd)
{
	struct	fs *fs;
	int	ret;
	char	buf[SBSIZE];

	ret = pread(fd, buf, SBSIZE, SBOFF);
	fs = (struct fs *)buf;
	if (ret == -1) {
		warn("pread");
		return 0;
	}
	if (fs->fs_magic == FS_MAGIC)
		return 0;
	if (fs->fs_magic == bswap32(FS_MAGIC))
		return 0;
	return 1;
}

static int
generate(struct params *p, int argc, char **argv, const char *outfile)
{
	int	 ret;

	if (argc < 1 || argc > 2)
		usage();

	p->algorithm = string_fromcharstar(argv[0]);
	if (argc > 1)
		p->keylen = atoi(argv[1]);

	ret = params_filldefaults(p);
	if (ret)
		return ret;

	if (!p->keygen) {
		p->keygen = keygen_generate(KEYGEN_PKCS5_PBKDF2);
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
		p->keygen = keygen_generate(KEYGEN_PKCS5_PBKDF2);
		if (!p->keygen)
			return -1;
	}
	params_filldefaults(p);
	keygen_filldefaults(p->keygen, p->keylen);
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
