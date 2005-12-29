/*	$NetBSD: local.h,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

struct ipl_args{
	int a0, v0, v1;
};
extern struct ipl_args ipl_args;

#define	PD_CACHE_SIZE		(8 * 1024)
#define	SD_CACHE_SIZE		(1024 * 1024)
#define	SD_CACHE_LINESIZE	64

/* XXX: hack to use mdsetimage(8) to put kernel into secondary boot */
#define	kernel_binary		md_root_image
#define	kernel_binary_size	md_root_size

enum fstype {
	FSTYPE_UFS	= 1,
	FSTYPE_BFS	= 2,
	FSTYPE_USTARFS	= 3,
};

struct device_capability {
	boolean_t active;
	int booted_device;
	int booted_unit;
	int active_device;
	boolean_t disk_enabled;
	boolean_t network_enabled;
	boolean_t fd_enabled;
};
extern struct device_capability DEVICE_CAPABILITY;

boolean_t find_partition_start(int,  int *);
int fstype(int);
void delay(int);
boolean_t device_attach(int, int, int);
void data_attach(void *, size_t);
boolean_t ustarfs_load(const char *, void **, size_t *);
boolean_t prompt_yesno(int);
void prompt(void);
boolean_t read_vtoc(void);
