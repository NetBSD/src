/* $NetBSD: mount_smbfs.c,v 1.10 2009/09/04 18:22:37 pooka Exp $ */

/*
 * Copyright (c) 2000-2002, Boris Popov
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by Boris Popov.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Id: mount_smbfs.c,v 1.17 2002/04/10 04:17:51 bp Exp 
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: mount_smbfs.c,v 1.10 2009/09/04 18:22:37 pooka Exp $");

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/mount.h>

#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <err.h>
#include <sysexits.h>
#include <errno.h>

#include <cflib.h>

#include <netsmb/smb.h>
#include <netsmb/smb_conn.h>
#include <netsmb/smb_lib.h>

#include <fs/smbfs/smbfs.h>

#include "mount_smbfs.h"

#include "mntopts.h"

static void usage(void);

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	{ NULL, 0, 0, 0 }
};

#ifndef MOUNT_NOMAIN
int
main(int argc, char *argv[])
{

	setprogname(argv[0]);
	return mount_smbfs(argc, argv);
}
#endif

struct smb_ctx sctx;
void
mount_smbfs_parseargs(int argc, char *argv[], struct smbfs_args *mdatap,
	int *mntflagsp, char *canon_dev, char *mount_point)
{
	struct smb_ctx *ctx = &sctx;
	struct stat st;
	char *next;
	int opt, error, caseopt;

	if (argc == 2) {
		if (strcmp(argv[1], "-h") == 0) {
			usage();
		} else if (strcmp(argv[1], "-v") == 0) {
			errx(EX_OK, "version %d.%d.%d", SMBFS_VERSION / 100000,
			    (SMBFS_VERSION % 10000) / 1000,
			    (SMBFS_VERSION % 1000) / 100);
		}
	}
	if (argc < 3)
		usage();

	if (smb_lib_init() != 0)
		exit(1);

	*mntflagsp = error = 0;
	memset(mdatap, 0, sizeof(*mdatap));
	mdatap->uid = mdatap->gid = -1;
	caseopt = SMB_CS_NONE;

	if (smb_ctx_init(ctx, argc, argv, SMBL_SHARE, SMBL_SHARE, SMB_ST_DISK) != 0)
		exit(1);
	if (smb_ctx_readrc(ctx) != 0)
		exit(1);
	if (smb_rc)
		rc_close(smb_rc);

	while ((opt = getopt(argc, argv, STDPARAM_OPT"c:d:f:g:l:n:o:u:w:")) != -1) {
		switch (opt) {
		    case STDPARAM_ARGS:
			error = smb_ctx_opt(ctx, opt, optarg);
			if (error)
				exit(1);
			break;
		    case 'u': {
			struct passwd *pwd;

			pwd = isdigit((unsigned char)optarg[0]) ?
			    getpwuid(atoi(optarg)) : getpwnam(optarg);
			if (pwd == NULL)
				errx(EX_NOUSER, "unknown user '%s'", optarg);
			mdatap->uid = pwd->pw_uid;
			break;
		    }
		    case 'g': {
			struct group *grp;

			grp = isdigit((unsigned char)optarg[0]) ?
			    getgrgid(atoi(optarg)) : getgrnam(optarg);
			if (grp == NULL)
				errx(EX_NOUSER, "unknown group '%s'", optarg);
			mdatap->gid = grp->gr_gid;
			break;
		    }
		    case 'd':
			errno = 0;
			mdatap->dir_mode = strtol(optarg, &next, 8);
			if (errno || *next != 0)
				errx(EX_DATAERR, "invalid value for directory mode");
			break;
		    case 'f':
			errno = 0;
			mdatap->file_mode = strtol(optarg, &next, 8);
			if (errno || *next != 0)
				errx(EX_DATAERR, "invalid value for file mode");
			break;
		    case '?':
			usage();
			/*NOTREACHED*/
		    case 'n': {
			char *inp, *nsp;

			nsp = inp = optarg;
			while ((nsp = strsep(&inp, ",;:")) != NULL) {
				if (strcasecmp(nsp, "LONG") == 0)
					mdatap->flags |= SMBFS_MOUNT_NO_LONG;
				else
					errx(EX_DATAERR, "unknown suboption '%s'", nsp);
			}
			break;
		    };
		    case 'o':
			getmntopts(optarg, mopts, mntflagsp, 0);
			break;
		    case 'c':
			switch (optarg[0]) {
			    case 'l':
				caseopt |= SMB_CS_LOWER;
				break;
			    case 'u':
				caseopt |= SMB_CS_UPPER;
				break;
			    default:
		    		errx(EX_DATAERR, "invalid suboption '%c' for -c",
				    optarg[0]);
			}
			break;
		    default:
			usage();
		}
	}

	if (optind == argc - 2)
		optind++;

	if (optind != argc - 1)
		usage();
	realpath(argv[optind], mount_point);

	if (stat(mount_point, &st) == -1)
		err(EX_OSERR, "could not find mount point %s", mount_point);
	if (!S_ISDIR(st.st_mode)) {
		errno = ENOTDIR;
		err(EX_OSERR, "can't mount on %s", mount_point);
	}
/*
	if (smb_getextattr(mount_point, &einfo) == 0)
		errx(EX_OSERR, "can't mount on %s twice", mount_point);
*/
	if (mdatap->uid == (uid_t)-1)
		mdatap->uid = st.st_uid;
	if (mdatap->gid == (gid_t)-1)
		mdatap->gid = st.st_gid;
	if (mdatap->file_mode == 0 )
		mdatap->file_mode = st.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
	if (mdatap->dir_mode == 0) {
		mdatap->dir_mode = mdatap->file_mode;
		if (mdatap->dir_mode & S_IRUSR)
			mdatap->dir_mode |= S_IXUSR;
		if (mdatap->dir_mode & S_IRGRP)
			mdatap->dir_mode |= S_IXGRP;
		if (mdatap->dir_mode & S_IROTH)
			mdatap->dir_mode |= S_IXOTH;
	}
	/*
	 * For now, let connection be private for this mount
	 */
	ctx->ct_ssn.ioc_opt |= SMBVOPT_PRIVATE;
	if (getuid() == 0)
		ctx->ct_ssn.ioc_owner = ctx->ct_sh.ioc_owner = 0; /* root */
	else
		ctx->ct_ssn.ioc_owner = ctx->ct_sh.ioc_owner = mdatap->uid;
	ctx->ct_ssn.ioc_group = ctx->ct_sh.ioc_group = mdatap->gid;
	opt = 0;
	if (mdatap->dir_mode & S_IXGRP)
		opt |= SMBM_EXECGRP;
	if (mdatap->dir_mode & S_IXOTH)
		opt |= SMBM_EXECOTH;
	ctx->ct_ssn.ioc_rights |= opt;
	ctx->ct_sh.ioc_rights |= opt;
	error = smb_ctx_resolve(ctx);
	if (error)
		err(1, "resolve %d", error);
	error = smb_ctx_lookup(ctx, SMBL_SHARE, SMBLK_CREATE);
	if (error) {
		err(1, "lookup %d", error);
	}
	mdatap->version = SMBFS_VERSION;
	mdatap->dev_fd = ctx->ct_fd;
	mdatap->caseopt = caseopt;
}

int
mount_smbfs(int argc, char *argv[])
{
	char canon_dev[MAXPATHLEN], canon_dir[MAXPATHLEN];
	struct smbfs_args mdata;
	int mntflags, error;

	mount_smbfs_parseargs(argc, argv, &mdata, &mntflags,
	    canon_dev, canon_dir);

	error = mount(SMBFS_VFSNAME, canon_dir, mntflags,
	    &mdata, sizeof mdata);
	smb_ctx_done(&sctx); /* XXX */

	if (error) {
		smb_error("mount error for %s: %s", error, canon_dir,
			strerror(errno));
		exit(1);
	}
	return 0;
}

static void
usage(void)
{
	fprintf(stderr, "%s\n%s\n%s\n%s\n",
	"usage: mount_smbfs [-E cs1:cs2] [-I host] [-L locale] [-M crights:srights]",
	"                   [-N] [-O cowner:cgroup/sowner:sgroup] [-R retrycount]",
	"                   [-T timeout] [-W workgroup] [-c case] [-d mode] [-f mode]",
	"                   [-g gid] [-n opt] [-u uid] //user@server/share node");

	exit (1);
}
