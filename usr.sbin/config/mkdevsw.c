/*	$NetBSD: mkdevsw.c,v 1.4 2003/05/17 18:53:01 itojun Exp $	*/

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by MAEKAWA Masahide (gehenna@NetBSD.org).
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
 *	  This product includes software developed by the NetBSD
 *	  Foundation, Inc. and its contributors.
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

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "defs.h"

static int emitconv(FILE *);
static int emitdev(FILE *);
static int emitdevm(FILE *);
static int emitheader(FILE *);

int
mkdevsw(void)
{
	FILE *fp;

	if ((fp = fopen("devsw.c.tmp", "w")) == NULL) {
		(void)fprintf(stderr, "config: cannot write devsw.c: %s\n",
			      strerror(errno));
		return (1);
	}

	if (emitheader(fp) || emitdevm(fp) || emitconv(fp) || emitdev(fp)) {
		(void)fprintf(stderr, "config: error writing devsw.c: %s\n",
			      strerror(errno));
		(void)fclose(fp);
		return (1);
	}

	(void)fclose(fp);

	if (moveifchanged("devsw.c.tmp", "devsw.c") != 0) {
		(void)fprintf(stderr, "config: error renaming devsw.c: %s\n",
			      strerror(errno));
		return (1);
	}

	return (0);
}

static int
emitheader(FILE *fp)
{
	if (fprintf(fp, "/*\n * MACHINE GENERATED: DO NOT EDIT\n *\n"
		    " * devsw.c, from \"%s\"\n */\n\n", conffile) < 0) {
		return (1);
	}

	if (fputs("#include <sys/param.h>\n"
		  "#include <sys/conf.h>\n"
		  "\n#define\tDEVSW_ARRAY_SIZE(x)\t"
		  "(sizeof((x))/sizeof((x)[0]))\n", fp) < 0) {
		return (1);
	}

	return (0);
}

/*
 * Emit device switch table for character/block device.
 */
static int
emitdevm(FILE *fp)
{
	struct devm *dm;
	char mstr[16];
	int i;

	if (fputs("\n/* device switch table for block device */\n", fp) < 0)
		return (1);

	for (i = 0 ; i <= maxbdevm ; i++) {
		(void)snprintf(mstr, sizeof(mstr), "%d", i);
		if ((dm = ht_lookup(bdevmtab, intern(mstr))) == NULL)
			continue;

		if (fprintf(fp, "extern const struct bdevsw %s_bdevsw;\n",
			    dm->dm_name) < 0)
			return (1);
	}

	if (fputs("\nconst struct bdevsw *bdevsw0[] = {\n", fp) < 0)
		return (1);

	for (i = 0 ; i <= maxbdevm ; i++) {
		(void)snprintf(mstr, sizeof(mstr), "%d", i);
		if ((dm = ht_lookup(bdevmtab, intern(mstr))) == NULL) {
			if (fprintf(fp, "\tNULL,\n") < 0)
				return (1);
		} else {
			if (fprintf(fp, "\t&%s_bdevsw,\n", dm->dm_name) < 0)
				return (1);
		}
	}

	if (fputs("};\n\nconst struct bdevsw **bdevsw = bdevsw0;\n", fp) < 0)
		return (1);

	if (fputs("const int sys_bdevsws = DEVSW_ARRAY_SIZE(bdevsw0);\n"
		  "int max_bdevsws = DEVSW_ARRAY_SIZE(bdevsw0);\n", fp) < 0)
		return (1);

	if (fputs("\n/* device switch table for character device */\n", fp) < 0)
		return (1);

	for (i = 0 ; i <= maxcdevm ; i++) {
		(void)snprintf(mstr, sizeof(mstr), "%d", i);
		if ((dm = ht_lookup(cdevmtab, intern(mstr))) == NULL)
			continue;

		if (fprintf(fp, "extern const struct cdevsw %s_cdevsw;\n",
			    dm->dm_name) < 0)
			return (1);
	}

	if (fputs("\nconst struct cdevsw *cdevsw0[] = {\n", fp) < 0)
		return (1);

	for (i = 0 ; i <= maxcdevm ; i++) {
		(void)snprintf(mstr, sizeof(mstr), "%d", i);
		if ((dm = ht_lookup(cdevmtab, intern(mstr))) == NULL) {
			if (fprintf(fp, "\tNULL,\n") < 0)
				return (1);
		} else {
			if (fprintf(fp, "\t&%s_cdevsw,\n", dm->dm_name) < 0)
				return (1);
		}
	}

	if (fputs("};\n\nconst struct cdevsw **cdevsw = cdevsw0;\n", fp) < 0)
		return (1);

	if (fputs("const int sys_cdevsws = DEVSW_ARRAY_SIZE(cdevsw0);\n"
		  "int max_cdevsws = DEVSW_ARRAY_SIZE(cdevsw0);\n", fp) < 0)
		return (1);

	return (0);
}

/*
 * Emit device major conversion table.
 */
static int
emitconv(FILE *fp)
{
	struct devm *dm;

	if (fputs("\n/* device conversion table */\n"
		  "struct devsw_conv devsw_conv0[] = {\n", fp) < 0)
		return (-1);
	TAILQ_FOREACH(dm, &alldevms, dm_next) {
		if (fprintf(fp, "\t{ \"%s\", %d, %d },\n", dm->dm_name,
			    dm->dm_bmajor, dm->dm_cmajor) < 0)
			return (1);
	}
	if (fputs("};\n\n"
		  "struct devsw_conv *devsw_conv = devsw_conv0;\n"
		  "int max_devsw_convs = DEVSW_ARRAY_SIZE(devsw_conv0);\n",
		  fp) < 0)
		return (1);

	return (0);
}

/*
 * Emit specific device major informations.
 */
static int
emitdev(FILE *fp)
{
	struct devm *dm;
	char mstr[16];

	if (fputs("\n", fp) < 0)
		return (1);

	(void)strlcpy(mstr, "swap", sizeof(mstr));
	if ((dm = ht_lookup(bdevmtab, intern(mstr))) == NULL)
		panic("swap device is not configured");
	if (fprintf(fp, "const dev_t swapdev = makedev(%d, 0);\n",
		    dm->dm_bmajor) < 0)
		return (1);

	(void)strlcpy(mstr, "mem", sizeof(mstr));
	if ((dm = ht_lookup(cdevmtab, intern(mstr))) == NULL)
		panic("memory device is not configured");
	if (fprintf(fp, "const dev_t zerodev = makedev(%d, DEV_ZERO);\n",
		    dm->dm_cmajor) < 0)
		return (1);

	if (fputs("\n/* mem_no is only used in iskmemdev() */\n", fp) < 0)
		return (1);
	if (fprintf(fp, "const int mem_no = %d;\n", dm->dm_cmajor) < 0)
		return (1);

	return (0);
}
