/* $NetBSD: booted_dev.c,v 1.2.6.2 2017/12/03 11:36:11 jdolecek Exp $ */

/*
 * Copyright 2000, 2001
 * Broadcom Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and copied only
 * in accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * modified or unmodified copies of this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and
 *    retain this copyright notice and list of conditions as they appear in
 *    the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Broadcom Corporation.  The "Broadcom Corporation" name may not be
 *    used to endorse or promote products derived from this software
 *    without the prior written permission of Broadcom Corporation.
 *
 * 3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR IMPLIED
 *    WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 *    NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL BROADCOM BE LIABLE
 *    FOR ANY DAMAGES WHATSOEVER, AND IN PARTICULAR, BROADCOM SHALL NOT BE
 *    LIABLE FOR DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *    OR OTHERWISE), EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include "stand/sbmips/common/common.h"
#include "stand/sbmips/common/cfe_api.h"

int	booted_dev_fd;
#if defined(PRIMARY_BOOTBLOCK) || defined(UNIFIED_BOOTBLOCK)
char	booted_dev_name[BOOTED_DEV_MAXNAMELEN];
#endif /* defined(PRIMARY_BOOTBLOCK) || defined(UNIFIED_BOOTBLOCK) */

#if defined(PRIMARY_BOOTBLOCK) || defined(UNIFIED_BOOTBLOCK)
int booted_dev_open(void)
{
    if (cfe_getenv("BOOT_DEVICE",booted_dev_name,sizeof(booted_dev_name)) != 0) {
	return 0;
	}

    booted_dev_fd = cfe_open(booted_dev_name);

    return 1;
}
#endif

void booted_dev_close(void)
{
    cfe_close(booted_dev_fd);
}
