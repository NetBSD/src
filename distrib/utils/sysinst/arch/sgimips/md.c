/*	$NetBSD: md.c,v 1.9 2003/07/11 14:48:42 dsl Exp $	*/

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Written by Philip A. Nelson for Piermont Information Systems Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Piermont Information Systems Inc.
 * 4. The name of Piermont Information Systems Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PIERMONT INFORMATION SYSTEMS INC. ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* md.c -- sgimips machine specific routines */

#include <sys/types.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <stdio.h>
#include <curses.h>
#include <unistd.h>
#include <fcntl.h>
#include <util.h>
#include <errno.h>
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

int
md_get_info(void)
{

	disktype = "SCSI";
	return 1;
}

int
md_pre_disklabel(void)
{
	return 0;
}

int
md_post_disklabel(void)
{
	return run_prog(RUN_DISPLAY, MSG_cmdfail,
	    "%s %s", "/usr/mdec/sgivol -f -w boot /usr/mdec/boot", diskdev);
}

int
md_post_newfs(void)
{
	return 0;
}

int
md_copy_filesystem(void)
{
	return 0;
}

int
md_make_bsd_partitions(void)
{
	return make_bsd_partitions();
}

/*    
 * any additional partition validation
 */   
int     
md_check_partitions(void)
{
	return 1;
}


/* update support */
int 
md_update(void)
{
	return 1;
}

void
md_cleanup_install(void)
{
}

int
md_pre_update()
{
	return 1;
}

void
md_init()
{
}

void
md_set_sizemultname()
{

	set_sizemultname_meg();
}
