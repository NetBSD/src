/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By downloading, copying, installing or
 * using the software you agree to this license. If you do not agree to this license, do not download, install,
 * copy or use the software. 
 *
 * Intel License Agreement 
 *
 * Copyright (c) 2002, Intel Corporation
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met: 
 *
 * -Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *  following disclaimer. 
 *
 * -Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 *  following disclaimer in the documentation and/or other materials provided with the distribution. 
 *
 * -The name of Intel Corporation may not be used to endorse or promote products derived from this software
 *  without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE. 
 */

/*
 * Transport-independent Methods
 */


#ifndef _OSD_OPS_H_
#define _OSD_OPS_H_

#include "osd.h"

typedef struct osd_ops_mem {
	const void		*send_data;  
	uint64_t	send_len; 
	int		send_sg;
	void		*recv_data; 
	uint64_t	recv_len; 
	int		recv_sg;
} OSD_OPS_MEM;

int osd_create_group(void *, int (*)(void *, osd_args_t *, OSD_OPS_MEM *), uint32_t *);
int osd_remove_group(void *, uint32_t , int (*)(void *, osd_args_t *, OSD_OPS_MEM *));
int osd_create(void *, uint32_t , int (*)(void *, osd_args_t *, OSD_OPS_MEM *), uint64_t *);
int osd_remove(void *, uint32_t , uint64_t , int (*)(void *, osd_args_t *, OSD_OPS_MEM *));
int osd_write(void *, uint32_t , uint64_t , uint64_t , uint64_t , const void *, int , int (*)(void *, osd_args_t *, OSD_OPS_MEM *));
int osd_read(void *, uint32_t , uint64_t , uint64_t , uint64_t , void *, int , int (*)(void *, osd_args_t *, OSD_OPS_MEM *));
int osd_set_one_attr(void *, uint32_t , uint64_t , uint32_t , uint32_t , uint32_t , void *, int (*)(void *, osd_args_t *, OSD_OPS_MEM *));
int osd_get_one_attr(void *, uint32_t , uint64_t , uint32_t , uint32_t , uint32_t , int (*)(void *, osd_args_t *, OSD_OPS_MEM *), uint16_t *, void *);

#endif
