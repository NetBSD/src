/* $NetBSD: cfe_api.h,v 1.1.10.2 2002/06/23 17:37:59 jdolecek Exp $ */

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
 *    Broadcom Corporation. Neither the "Broadcom Corporation" name nor any
 *    trademark or logo of Broadcom Corporation may be used to endorse or
 *    promote products derived from this software without the prior written
 *    permission of Broadcom Corporation.
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

/*  *********************************************************************
    *  Broadcom Common Firmware Environment (CFE)
    *
    *  Device function prototypes		File: cfe_api.h
    *
    *  This module contains prototypes for cfe_devfuncs.c, a set
    *  of wrapper routines to the IOCB interface.  This file,
    *  along with cfe_api.c, can be incorporated into programs
    *  that need to call CFE.
    *
    *  Author:  Mitch Lichtenberg (mpl@broadcom.com)
    *
    ********************************************************************* */


#define	CFE_EPTSEAL 0x43464531
#define	CFE_APIENTRY 0xBFC00500
#define	CFE_APISEAL  0xBFC00508

#ifndef __ASSEMBLER__
int cfe_init(cfe_xuint_t handle);
int cfe_open(char *name);
int cfe_close(int handle);
int cfe_readblk(int handle,cfe_xint_t offset,u_char *buffer,int length);
int cfe_read(int handle,u_char *buffer,int length);
int cfe_writeblk(int handle,cfe_xint_t offset,u_char *buffer,int length);
int cfe_write(int handle,u_char *buffer,int length);
int cfe_ioctl(int handle,u_int ioctlnum,u_char *buffer,int length,int *retlen);
int cfe_inpstat(int handle);
int cfe_enumenv(int idx,char *name,int namelen,char *val,int vallen);
int cfe_setenv(char *name,char *val);
int cfe_getenv(char *name,char *dest,int destlen);
long long cfe_getticks(void);
int cfe_exit(int warm,int status);
int cfe_flushcache(int flg);
int cfe_getstdhandle(int flg);
int cfe_getdevinfo(char *name);
int cfe_getmeminfo(int idx,cfe_xuint_t *start,cfe_xuint_t *length,cfe_xuint_t *type);
int cfe_getfwinfo(cfe_fwinfo_t *info);
#endif
