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
#include "config.h"


#include "osd.h"
#include "util.h"
#include "compat.h"
#include "osd_ops.h"

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

static int 
extract_attribute(uint32_t page, uint32_t n, uint16_t len,
		  uint8_t * data, unsigned length, void *val)
{
	int             i = 0;

	for (i = 0; i < length;) {
		if (ISCSI_NTOHL(*(uint32_t *) (data + i)) != page) {
			iscsi_trace_error(__FILE__, __LINE__, "page mismatch: got 0x%x, expected 0x%x\n", ISCSI_NTOHL(*(uint32_t *) (data + i)), page);
			return -1;
		}
		i += 4;
		if (ISCSI_NTOHL(*(uint32_t *) (data + i)) != n) {
			iscsi_trace_error(__FILE__, __LINE__, "index mismatch\n");
			return -1;
		}
		i += 4;
		if (ISCSI_NTOHS(*(uint16_t *) (data + i)) != len) {
			iscsi_trace_error(__FILE__, __LINE__, "len mismatch\n");
			return -1;
		}
		i += 2;
		iscsi_trace(TRACE_DEBUG, __FILE__, __LINE__, "page 0x%x, index %u, len %u\n", page, n, len);
		memcpy(val, data + i, len);
		i += len;
	}
	iscsi_trace(TRACE_DEBUG, __FILE__, __LINE__, "parsed %i bytes\n", i);
	return i;
}


int 
osd_create_group(void *dev,
	  int (*osd_exec) (void *dev, osd_args_t * args, OSD_OPS_MEM * mem),
		 uint32_t * GroupID)
{
	osd_args_t      args;
#if 0
	uint8_t         get_list[8];
#endif
	uint8_t         get_data[14];
	OSD_OPS_MEM     mem;

	mem.recv_data = get_data;
	mem.recv_len = 14;
	mem.recv_sg = 0;

	memset(&args, 0, sizeof(osd_args_t));
	args.opcode = 0x7F;
	args.service_action = OSD_CREATE_GROUP;

#if 0
	args.length = 8;
	args.get_attributes_list_length = 8;
	*((unsigned long *) get_list) = ISCSI_HTONL(0x40000001);
	*((unsigned long *) (get_list + 4)) = ISCSI_HTONL(0x1);
	mem.send_data = get_list;
	mem.send_len = 8;
	mem.send_sg = 0;
#else
	args.get_attributes_page = 0x40000001;
	mem.send_data = NULL;
	mem.send_len = 0;
	mem.send_sg = 0;
#endif

	args.get_attributes_allocation_length = 14;
	if (osd_exec(dev, &args, &mem) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "osd_exec() failed\n");
		return -1;
	}
	if (extract_attribute(0x40000001, 0x1, 4, get_data, 14, GroupID) == -1) {
		iscsi_trace_error(__FILE__, __LINE__, "extract_attributes() failed\n");
		return -1;
	}
	*GroupID = ISCSI_NTOHL(*GroupID);
	iscsi_trace(TRACE_OSD, __FILE__, __LINE__, "osd_create_group() OK --> GroupID 0x%x\n", *GroupID);

	return 0;
}

int 
osd_create(void *dev, uint32_t GroupID,
	   int (*osd_exec) (void *dev, osd_args_t * args, OSD_OPS_MEM * mem),
	   uint64_t * UserID)
{
	osd_args_t      args;
#if 0
	uint8_t         get_list[8];
#endif
	uint8_t         get_data[18];
	OSD_OPS_MEM     mem;

	mem.recv_data = get_data;
	mem.recv_len = 18;
	mem.recv_sg = 0;

	memset(&args, 0, sizeof(osd_args_t));
	args.opcode = 0x7F;
	args.service_action = OSD_CREATE;
	args.GroupID = GroupID;

#if 0
	args.length = 8;
	args.get_attributes_list_length = 8;
	*((unsigned long *) get_list) = ISCSI_HTONL(0x00000001);
	*((unsigned long *) (get_list + 4)) = ISCSI_HTONL(0x2);
	mem.send_data = get_list;
	mem.send_len = 8;
	mem.send_sg = 0;
#else
	args.get_attributes_page = 0x000000001;
	mem.send_data = NULL;
	mem.send_len = 0;
	mem.send_sg = 0;
#endif

	args.get_attributes_allocation_length = 18;
	if (osd_exec(dev, &args, &mem) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "osd_exec() failed\n");
		return -1;
	}
	if (extract_attribute(0x00000001, 0x2, 8, get_data, 18, UserID) == -1) {
		iscsi_trace_error(__FILE__, __LINE__, "extract_attributes() failed\n");
		return -1;
	}
	*UserID = ISCSI_NTOHLL(*UserID);
	iscsi_trace(TRACE_OSD, __FILE__, __LINE__, "osd_create(GroupID 0x%x) OK --> UserID 0x%llx\n", GroupID, *UserID);

	return 0;
}

int 
osd_remove_group(void *dev, uint32_t GroupID,
	  int (*osd_exec) (void *dev, osd_args_t * args, OSD_OPS_MEM * mem))
{
	osd_args_t      args;
	OSD_OPS_MEM     mem;

	mem.send_data = NULL;
	mem.send_len = 0;
	mem.send_sg = 0;
	mem.recv_data = NULL;
	mem.recv_len = 0;
	mem.recv_sg = 0;

	memset(&args, 0, sizeof(osd_args_t));
	args.opcode = 0x7F;
	args.service_action = OSD_REMOVE_GROUP;
	args.GroupID = GroupID;
	if (osd_exec(dev, &args, &mem) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "osd_exec() failed\n");
		return -1;
	}
	iscsi_trace(TRACE_OSD, __FILE__, __LINE__, "osd_remove_group(Group ID 0x%x) OK\n", GroupID);

	return 0;
}

int 
osd_remove(void *dev, uint32_t GroupID, uint64_t UserID,
	   int (*osd_exec) (void *dev, osd_args_t * args, OSD_OPS_MEM * mem))
{
	osd_args_t      args;
	OSD_OPS_MEM     mem;

	mem.send_data = NULL;
	mem.send_len = 0;
	mem.send_sg = 0;
	mem.recv_data = NULL;
	mem.recv_len = 0;
	mem.recv_sg = 0;

	memset(&args, 0, sizeof(osd_args_t));
	args.opcode = 0x7F;
	args.service_action = OSD_REMOVE;
	args.UserID = UserID;
	args.GroupID = GroupID;
	if (osd_exec(dev, &args, &mem) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "osd_exec() failed\n");
		return -1;
	}
	iscsi_trace(TRACE_OSD, __FILE__, __LINE__, "osd_remove(GroupID 0x%x, UserID 0x%llx) OK\n", GroupID, UserID);

	return 0;
}

int 
osd_write(void *dev,
	  uint32_t GroupID, uint64_t UserID, uint64_t offset, uint64_t len, const void *send_data, int sg_len,
	  int (*osd_exec) (void *dev, osd_args_t * args, OSD_OPS_MEM * mem))
{
	osd_args_t      args;
	OSD_OPS_MEM     mem;

	iscsi_trace(TRACE_OSD, __FILE__, __LINE__, "osd_write(GroupID 0x%x, UserID 0x%llx, Offset %llu, Len %llu)\n", GroupID, UserID, offset, len);
	mem.send_data = send_data;
	mem.send_len = len;
	mem.send_sg = sg_len;
	mem.recv_data = NULL;
	mem.recv_len = 0;
	mem.recv_sg = 0;
	memset(&args, 0, sizeof(osd_args_t));
	args.opcode = 0x7F;
	args.service_action = OSD_WRITE;
	args.GroupID = GroupID;
	args.UserID = UserID;
	args.offset = offset;
	args.length = len;
	if (osd_exec(dev, &args, &mem) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "osd_exec() failed\n");
		return -1;
	}
	return 0;
}

int 
osd_read(void *dev,
	 uint32_t GroupID, uint64_t UserID, uint64_t offset, uint64_t len, void *recv_data, int sg_len,
	 int (*osd_exec) (void *dev, osd_args_t * args, OSD_OPS_MEM * mem))
{
	osd_args_t      args;
	OSD_OPS_MEM     mem;

	iscsi_trace(TRACE_OSD, __FILE__, __LINE__, "osd_read(GroupID 0x%x, UserID 0x%llx, Offset %llu, Len %llu)\n", GroupID, UserID, offset, len);
	mem.send_data = NULL;
	mem.send_len = 0;
	mem.send_sg = 0;
	mem.recv_data = recv_data;
	mem.recv_len = len;
	mem.recv_sg = sg_len;
	memset(&args, 0, sizeof(osd_args_t));
	args.opcode = 0x7F;
	args.service_action = OSD_READ;
	args.GroupID = GroupID;
	args.UserID = UserID;
	args.offset = offset;
	args.length = len;
	if (osd_exec(dev, &args, &mem) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "osd_exec() failed\n");
		return -1;
	}
	return 0;
}

int 
osd_set_one_attr(void *dev,
		 uint32_t GroupID, uint64_t UserID, uint32_t page, uint32_t n, uint32_t len, void *value,
	  int (*osd_exec) (void *dev, osd_args_t * args, OSD_OPS_MEM * mem))
{
	osd_args_t      args;
	OSD_OPS_MEM     mem;
	uint8_t         list[10];
#if 0
	struct iovec    sg[2];
#else
	uint8_t        *buffer = NULL;
#endif

	iscsi_trace(TRACE_OSD, __FILE__, __LINE__, "osd_set_one_attr(GroupID 0x%x, UserID 0x%llx, Page 0x%x, Index %u, Len %u)\n",
	      GroupID, UserID, page, n, len);
	memset(&args, 0, sizeof(osd_args_t));
	args.opcode = 0x7F;
	args.service_action = OSD_SET_ATTR;
	args.GroupID = GroupID;
	args.UserID = UserID;
	args.length = 10 + len;
	args.set_attributes_list_length = 10 + len;
	*((uint32_t *) (list + 0)) = ISCSI_HTONL(page);
	*((uint32_t *) (list + 4)) = ISCSI_HTONL(n);
	*((uint16_t *) (list + 8)) = ISCSI_HTONS(len);
#if 0
	sg[0].iov_base = list;
	sg[0].iov_len = 10;
	sg[1].iov_base = value;
	sg[1].iov_len = len;
	mem.send_data = sg;
	mem.send_len = 10 + len;
	mem.send_sg = 2;
#else
	if ((buffer = iscsi_malloc_atomic(10 + len)) == NULL) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_malloc() failed\n");
		return -1;
	}
	memcpy(buffer, list, 10);
	memcpy(buffer + 10, value, len);
	mem.send_data = buffer;
	mem.send_len = 10 + len;
	mem.send_sg = 0;
#endif
	mem.recv_data = NULL;
	mem.recv_len = 0;
	mem.recv_sg = 0;

	if (osd_exec(dev, &args, &mem) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "osd_exec() failed\n");
		return -1;
	}
	if (buffer)
		iscsi_free_atomic(buffer);

	return 0;
}

int 
osd_get_one_attr(void *dev,
		 uint32_t GroupID, uint64_t UserID, uint32_t page, uint32_t n, uint32_t alloc_len,
	  int (*osd_exec) (void *dev, osd_args_t * args, OSD_OPS_MEM * mem),
		 uint16_t * len, void *value)
{
	osd_args_t      args;
	OSD_OPS_MEM     mem;
	uint8_t         list_out[8];
#if 0
	uint8_t         list_in[10];
	struct iovec    sg[2];
#else
	uint8_t        *buffer;
#endif

	iscsi_trace(TRACE_OSD, __FILE__, __LINE__, "osd_get_one_attr(GroupID 0x%x, UserID 0x%llx, Page 0x%x, Index %u, Alloc Len %u)\n",
	      GroupID, UserID, page, n, alloc_len);
	memset(&args, 0, sizeof(osd_args_t));
	args.opcode = 0x7F;
	args.service_action = OSD_GET_ATTR;
	args.GroupID = GroupID;
	args.UserID = UserID;
	if (n) {
		args.length = 8;
		*((uint32_t *) (list_out + 0)) = ISCSI_HTONL(page);
		*((uint32_t *) (list_out + 4)) = ISCSI_HTONL(n);
		args.get_attributes_list_length = 8;
		mem.send_data = list_out;
		mem.send_len = 8;
		mem.send_sg = 0;
	} else {
		iscsi_trace(TRACE_OSD, __FILE__, __LINE__, "requesting entire page or reference page\n");
		mem.send_data = NULL;
		mem.send_len = 0;
		mem.send_sg = 0;
		args.get_attributes_page = page;
	}
#if 0
	sg[0].iov_base = list_in;
	sg[0].iov_len = 10;
	sg[1].iov_base = value;
	sg[1].iov_len = alloc_len;
	mem.recv_data = sg;
	mem.recv_len = 10 + alloc_len;
	mem.recv_sg = 2;
#else
	if ((buffer = iscsi_malloc_atomic(10 + alloc_len)) == NULL) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_malloc() failed\n");
		return -1;
	}
	mem.recv_data = buffer;
	mem.recv_len = 10 + alloc_len;
	mem.recv_sg = 0;
#endif
	args.get_attributes_allocation_length = 10 + alloc_len;
	if (osd_exec(dev, &args, &mem) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "osd_exec() failed\n");
		return -1;
	}
	memcpy(value, buffer + 10, alloc_len);
	if (buffer)
		iscsi_free_atomic(buffer);
	return 0;
}
