/* $NetBSD: pass6.c,v 1.5 2023/07/04 20:40:53 riastradh Exp $ */

/*-
  * Copyright (c) 2010 Manuel Bouyer
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

#include <sys/param.h>
#include <sys/time.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>
#include <ufs/ufs/ufs_bswap.h>

#include <err.h>
#include <string.h>
#include <stdlib.h>
#include <ufs/ufs/quota2.h>

#include "fsutil.h"
#include "fsck.h"
#include "extern.h"

const char *utname[] = {"USER", "GROUP"};

void
pass6(void)
{
	int ret1, ret2;
	int i;
	if ((sblock->fs_flags & FS_DOQUOTA2) == 0)
		return;

	for (i = 0; i < MAXQUOTAS; i++) {
		if ((sblock->fs_quota_flags & FS_Q2_DO_TYPE(i)) == 0 &&
		    sblock->fs_quotafile[i] != 0) {
			if (preen || reply(
				    i == 0 ? "CLEAR USER QUOTA INODE" :
				    "CLEAR GROUP QUOTA INODE") != 0) {
				if (preen)
					printf("%s QUOTA INODE CLEARED\n",
				    utname[i]);
				freeino(sblock->fs_quotafile[i]);
				sblock->fs_quotafile[i] = 0;
				sbdirty();
			} else
				markclean = 0;
		}
	}
	if ((sblock->fs_quota_flags &
	    (FS_Q2_DO_TYPE(USRQUOTA) | FS_Q2_DO_TYPE(GRPQUOTA))) == 0) {
		if (preen || reply("CLEAR SUPERBLOCK QUOTA FLAG") != 0) {
			if (preen)
				printf("SUPERBLOCK QUOTA FLAG CLEARED\n");
			sblock->fs_flags &= ~FS_DOQUOTA2;
			sbdirty();
		} else
			markclean = 0;
	}

	ret1 = quota2_check_inode(USRQUOTA);
	ret2 = quota2_check_inode(GRPQUOTA);

	if (ret1 == 0)
		quota2_check_usage(USRQUOTA);
	if (ret2 == 0)
		quota2_check_usage(GRPQUOTA);
}
