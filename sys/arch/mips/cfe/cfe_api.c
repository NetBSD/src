/* $NetBSD: cfe_api.c,v 1.1.10.2 2002/06/23 17:37:59 jdolecek Exp $ */

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
    *  Device Function stubs			File: cfe_api.c
    *
    *  This module contains device function stubs (small routines to
    *  call the standard "iocb" interface entry point to CFE).
    *  There should be one routine here per iocb function call.
    *
    *  Author:  Mitch Lichtenberg (mpl@broadcom.com)
    *
    ********************************************************************* */


#include <sys/types.h>

#include <mips/cfe/cfe_xiocb.h>
#include <mips/cfe/cfe_api.h>

typedef long intptr_t;
typedef u_long uintptr_t;

/*
 * Declare the dispatch function with args of "intptr_t".
 * This makes sure whatever model we're compiling in
 * puts the pointers in a single register.  For example,
 * combining -mlong64 and -mips1 or -mips2 would lead to
 * trouble, since the handle and IOCB pointer will be
 * passed in two registers each, and CFE expects one.
 */

static int (*cfe_dispfunc)(intptr_t handle,intptr_t xiocb) = 0;
static cfe_xuint_t cfe_handle = 0;
int cfe_iocb_dispatch(cfe_xiocb_t *xiocb);

int cfe_init(cfe_xuint_t handle)
{
    u_int *sealloc = (u_int *) (intptr_t) (int)  CFE_APISEAL;
    if (*sealloc != CFE_EPTSEAL) return -1;
    cfe_dispfunc = (void *) (cfe_xptr_t) (int) CFE_APIENTRY;
    if (handle) cfe_handle = handle;
    return 0;
}

int cfe_iocb_dispatch(cfe_xiocb_t *xiocb)
{
    if (!cfe_dispfunc) return -1;
    return (*cfe_dispfunc)((intptr_t) cfe_handle,(intptr_t) xiocb);
}

static int cfe_strlen(char *name)
{
    int count = 0;

    while (*name) {
	count++;
	name++;
	}

    return count;
}

int cfe_open(char *name)
{
    cfe_xiocb_t xiocb;

    xiocb.xiocb_fcode = CFE_CMD_DEV_OPEN;
    xiocb.xiocb_status = 0;
    xiocb.xiocb_handle = 0;
    xiocb.xiocb_flags = 0;
    xiocb.xiocb_psize = sizeof(xiocb_buffer_t);
    xiocb.plist.xiocb_buffer.buf_offset = 0;
    xiocb.plist.xiocb_buffer.buf_ptr = (cfe_xptr_t) (intptr_t) name;
    xiocb.plist.xiocb_buffer.buf_length = cfe_strlen(name);

    cfe_iocb_dispatch(&xiocb);

    return (xiocb.xiocb_status < 0) ? xiocb.xiocb_status : xiocb.xiocb_handle;
}

int cfe_close(int handle)
{
    cfe_xiocb_t xiocb;

    xiocb.xiocb_fcode = CFE_CMD_DEV_CLOSE;
    xiocb.xiocb_status = 0;
    xiocb.xiocb_handle = handle;
    xiocb.xiocb_flags = 0;
    xiocb.xiocb_psize = 0;

    cfe_iocb_dispatch(&xiocb);

    return (xiocb.xiocb_status);

}

int cfe_readblk(int handle,cfe_xint_t offset,u_char *buffer,int length)
{
    cfe_xiocb_t xiocb;

    xiocb.xiocb_fcode = CFE_CMD_DEV_READ;
    xiocb.xiocb_status = 0;
    xiocb.xiocb_handle = handle;
    xiocb.xiocb_flags = 0;
    xiocb.xiocb_psize = sizeof(xiocb_buffer_t);
    xiocb.plist.xiocb_buffer.buf_offset = offset;
    xiocb.plist.xiocb_buffer.buf_ptr = (cfe_xptr_t) (intptr_t) buffer;
    xiocb.plist.xiocb_buffer.buf_length = length;

    cfe_iocb_dispatch(&xiocb);

    return (xiocb.xiocb_status < 0) ? xiocb.xiocb_status : xiocb.plist.xiocb_buffer.buf_retlen;
}

int cfe_read(int handle,u_char *buffer,int length)
{
    return cfe_readblk(handle,0,buffer,length);
}


int cfe_writeblk(int handle,cfe_xint_t offset,u_char *buffer,int length)
{
    cfe_xiocb_t xiocb;

    xiocb.xiocb_fcode = CFE_CMD_DEV_WRITE;
    xiocb.xiocb_status = 0;
    xiocb.xiocb_handle = handle;
    xiocb.xiocb_flags = 0;
    xiocb.xiocb_psize = sizeof(xiocb_buffer_t);
    xiocb.plist.xiocb_buffer.buf_offset = offset;
    xiocb.plist.xiocb_buffer.buf_ptr = (cfe_xptr_t) (intptr_t) buffer;
    xiocb.plist.xiocb_buffer.buf_length = length;

    cfe_iocb_dispatch(&xiocb);

    return (xiocb.xiocb_status < 0) ? xiocb.xiocb_status : xiocb.plist.xiocb_buffer.buf_retlen;
}

int cfe_write(int handle,u_char *buffer,int length)
{
    return cfe_writeblk(handle,0,buffer,length);
}


int cfe_ioctl(int handle,u_int ioctlnum,u_char *buffer,int length,int *retlen)
{
    cfe_xiocb_t xiocb;

    xiocb.xiocb_fcode = CFE_CMD_DEV_IOCTL;
    xiocb.xiocb_status = 0;
    xiocb.xiocb_handle = handle;
    xiocb.xiocb_flags = 0;
    xiocb.xiocb_psize = sizeof(xiocb_buffer_t);
    xiocb.plist.xiocb_buffer.buf_ioctlcmd = (cfe_xint_t) ioctlnum;
    xiocb.plist.xiocb_buffer.buf_ptr = (cfe_xptr_t) (intptr_t) buffer;
    xiocb.plist.xiocb_buffer.buf_length = length;

    cfe_iocb_dispatch(&xiocb);

    if (retlen) *retlen = xiocb.plist.xiocb_buffer.buf_retlen;
    return xiocb.xiocb_status;
}

int cfe_inpstat(int handle)
{
    cfe_xiocb_t xiocb;

    xiocb.xiocb_fcode = CFE_CMD_DEV_INPSTAT;
    xiocb.xiocb_status = 0;
    xiocb.xiocb_handle = handle;
    xiocb.xiocb_flags = 0;
    xiocb.xiocb_psize = sizeof(xiocb_inpstat_t);
    xiocb.plist.xiocb_inpstat.inp_status = 0;

    cfe_iocb_dispatch(&xiocb);

    if (xiocb.xiocb_status < 0) return xiocb.xiocb_status;

    return xiocb.plist.xiocb_inpstat.inp_status;

}

long long cfe_getticks(void)
{
    cfe_xiocb_t xiocb;

    xiocb.xiocb_fcode = CFE_CMD_FW_GETTIME;
    xiocb.xiocb_status = 0;
    xiocb.xiocb_handle = 0;
    xiocb.xiocb_flags = 0;
    xiocb.xiocb_psize = sizeof(xiocb_time_t);
    xiocb.plist.xiocb_time.ticks = 0;

    cfe_iocb_dispatch(&xiocb);

    return xiocb.plist.xiocb_time.ticks;

}

int cfe_getenv(char *name,char *dest,int destlen)
{
    cfe_xiocb_t xiocb;

    *dest = 0;

    xiocb.xiocb_fcode = CFE_CMD_ENV_GET;
    xiocb.xiocb_status = 0;
    xiocb.xiocb_handle = 0;
    xiocb.xiocb_flags = 0;
    xiocb.xiocb_psize = sizeof(xiocb_envbuf_t);
    xiocb.plist.xiocb_envbuf.enum_idx = 0;
    xiocb.plist.xiocb_envbuf.name_ptr = (cfe_xptr_t) (intptr_t) name;
    xiocb.plist.xiocb_envbuf.name_length = cfe_strlen(name);
    xiocb.plist.xiocb_envbuf.val_ptr = (cfe_xptr_t) (intptr_t) dest;
    xiocb.plist.xiocb_envbuf.val_length = destlen;

    cfe_iocb_dispatch(&xiocb);

    return xiocb.xiocb_status;
}

int cfe_setenv(char *name,char *val)
{
    cfe_xiocb_t xiocb;

    xiocb.xiocb_fcode = CFE_CMD_ENV_SET;
    xiocb.xiocb_status = 0;
    xiocb.xiocb_handle = 0;
    xiocb.xiocb_flags = 0;
    xiocb.xiocb_psize = sizeof(xiocb_envbuf_t);
    xiocb.plist.xiocb_envbuf.enum_idx = 0;
    xiocb.plist.xiocb_envbuf.name_ptr = (cfe_xptr_t) (intptr_t) name;
    xiocb.plist.xiocb_envbuf.name_length = cfe_strlen(name);
    xiocb.plist.xiocb_envbuf.val_ptr = (cfe_xptr_t) (intptr_t) val;
    xiocb.plist.xiocb_envbuf.val_length = cfe_strlen(val);

    cfe_iocb_dispatch(&xiocb);

    return xiocb.xiocb_status;
}

int cfe_enumenv(int idx,char *name,int namelen,char *val,int vallen)
{
    cfe_xiocb_t xiocb;

    xiocb.xiocb_fcode = CFE_CMD_ENV_SET;
    xiocb.xiocb_status = 0;
    xiocb.xiocb_handle = 0;
    xiocb.xiocb_flags = 0;
    xiocb.xiocb_psize = sizeof(xiocb_envbuf_t);
    xiocb.plist.xiocb_envbuf.enum_idx = idx;
    xiocb.plist.xiocb_envbuf.name_ptr = (cfe_xptr_t) (intptr_t) name;
    xiocb.plist.xiocb_envbuf.name_length = namelen;
    xiocb.plist.xiocb_envbuf.val_ptr = (cfe_xptr_t) (intptr_t) val;
    xiocb.plist.xiocb_envbuf.val_length = vallen;

    cfe_iocb_dispatch(&xiocb);

    return xiocb.xiocb_status;
}

int cfe_exit(int warm,int status)
{
    cfe_xiocb_t xiocb;

    xiocb.xiocb_fcode = CFE_CMD_FW_RESTART;
    xiocb.xiocb_status = 0;
    xiocb.xiocb_handle = 0;
    xiocb.xiocb_flags = warm ? CFE_FLG_WARMSTART : 0;
    xiocb.xiocb_psize = sizeof(xiocb_exitstat_t);
    xiocb.plist.xiocb_exitstat.status = (cfe_xint_t) status;

    cfe_iocb_dispatch(&xiocb);

    return (xiocb.xiocb_status);

}

int cfe_flushcache(int flg)
{
    cfe_xiocb_t xiocb;

    xiocb.xiocb_fcode = CFE_CMD_FW_FLUSHCACHE;
    xiocb.xiocb_status = 0;
    xiocb.xiocb_handle = 0;
    xiocb.xiocb_flags = flg;
    xiocb.xiocb_psize = 0;

    cfe_iocb_dispatch(&xiocb);

    return xiocb.xiocb_status;
}

int cfe_getstdhandle(int flg)
{
    cfe_xiocb_t xiocb;

    xiocb.xiocb_fcode = CFE_CMD_DEV_GETHANDLE;
    xiocb.xiocb_status = 0;
    xiocb.xiocb_handle = 0;
    xiocb.xiocb_flags = flg;
    xiocb.xiocb_psize = 0;

    cfe_iocb_dispatch(&xiocb);

    return (xiocb.xiocb_status < 0) ? xiocb.xiocb_status : xiocb.xiocb_handle;

}

int cfe_getdevinfo(char *name)
{
    cfe_xiocb_t xiocb;

    xiocb.xiocb_fcode = CFE_CMD_DEV_GETINFO;
    xiocb.xiocb_status = 0;
    xiocb.xiocb_handle = 0;
    xiocb.xiocb_flags = 0;
    xiocb.xiocb_psize = sizeof(xiocb_buffer_t);
    xiocb.plist.xiocb_buffer.buf_offset = 0;
    xiocb.plist.xiocb_buffer.buf_ptr = (cfe_xptr_t) (intptr_t) name;
    xiocb.plist.xiocb_buffer.buf_length = cfe_strlen(name);

    cfe_iocb_dispatch(&xiocb);

    return (xiocb.xiocb_status < 0) ? xiocb.xiocb_status : (int)xiocb.plist.xiocb_buffer.buf_devflags;
}

int cfe_getmeminfo(int idx,cfe_xuint_t *start,cfe_xuint_t *length,cfe_xuint_t *type)
{
    cfe_xiocb_t xiocb;

    xiocb.xiocb_fcode = CFE_CMD_FW_MEMENUM;
    xiocb.xiocb_status = 0;
    xiocb.xiocb_handle = 0;
    xiocb.xiocb_flags = 0;
    xiocb.xiocb_psize = sizeof(xiocb_meminfo_t);
    xiocb.plist.xiocb_meminfo.mi_idx = idx;

    cfe_iocb_dispatch(&xiocb);

    if (xiocb.xiocb_status < 0) return xiocb.xiocb_status;

    *start = xiocb.plist.xiocb_meminfo.mi_addr;
    *length = xiocb.plist.xiocb_meminfo.mi_size;
    *type = xiocb.plist.xiocb_meminfo.mi_type;

    return 0;
}

int cfe_getfwinfo(xiocb_fwinfo_t *info)
{
    cfe_xiocb_t xiocb;

    xiocb.xiocb_fcode = CFE_CMD_FW_GETINFO;
    xiocb.xiocb_status = 0;
    xiocb.xiocb_handle = 0;
    xiocb.xiocb_flags = 0;
    xiocb.xiocb_psize = sizeof(xiocb_fwinfo_t);

    cfe_iocb_dispatch(&xiocb);

    if (xiocb.xiocb_status < 0) return xiocb.xiocb_status;

    info->fwi_version = xiocb.plist.xiocb_fwinfo.fwi_version;
    info->fwi_totalmem = xiocb.plist.xiocb_fwinfo.fwi_totalmem;
    info->fwi_flags = xiocb.plist.xiocb_fwinfo.fwi_flags;
    info->fwi_boardid = xiocb.plist.xiocb_fwinfo.fwi_boardid;
    info->fwi_bootarea_va = xiocb.plist.xiocb_fwinfo.fwi_bootarea_va;
    info->fwi_bootarea_pa = xiocb.plist.xiocb_fwinfo.fwi_bootarea_pa;
    info->fwi_bootarea_size = xiocb.plist.xiocb_fwinfo.fwi_bootarea_size;
    info->fwi_reserved1 = xiocb.plist.xiocb_fwinfo.fwi_reserved1;
    info->fwi_reserved2 = xiocb.plist.xiocb_fwinfo.fwi_reserved2;
    info->fwi_reserved3 = xiocb.plist.xiocb_fwinfo.fwi_reserved3;

    return 0;
}
