/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By downloading, copying, installing or
 * using the software you agree to this license. If you do not agree to this license, do not download, install,
 * copy or use the software.
 *
 * Intel License Agreement
 *
 * Copyright (c) 2000, Intel Corporation
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
#include "config.h"
#include "compat.h"

#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_UTIME_H
#include <utime.h>
#endif

#include <unistd.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#include "scsi_cmd_codes.h"

#include "iscsi.h"
#include "initiator.h"
#include "tests.h"
#include "osd.h"
#include "osd_ops.h"


#define toSeconds(t) (t.tv_sec + (t.tv_usec/1000000.0))

typedef struct osd_device_t {
	int             target;
	int             lun;
}               OSD_DEVICE_T;

int             osd_command(void *dev, osd_args_t * args, OSD_OPS_MEM * mem);

/*
 * SCSI Command Tests
 */
int 
nop_out(uint64_t target, int lun, int length, int ping, const char *data)
{
	initiator_cmd_t cmd;
	iscsi_nop_out_args_t nop_cmd;

	cmd.type = ISCSI_NOP_OUT;
	cmd.ptr = &nop_cmd;
	cmd.isid = target;
	cmd.targetname[0] = 0x0;
	(void) memset(&nop_cmd, 0x0, sizeof(iscsi_nop_out_args_t));
	RETURN_GREATER("length", length, 4096, NO_CLEANUP, -1);
	nop_cmd.length = length;
	nop_cmd.data = (const uint8_t *) data;
	nop_cmd.lun = lun;
	if (!ping) {
		nop_cmd.tag = 0xffffffff;
	}
	if (initiator_command(&cmd) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "initiator_command() failed\n");
		return -1;
	}
	return 0;
}

static int 
inquiry(uint64_t target, uint32_t lun, uint32_t *device_type)
{
	uint8_t   data[36], cdb[16];
	initiator_cmd_t cmd;
	iscsi_scsi_cmd_args_t args;

	memset(cdb, 0, 16);
	cdb[0] = INQUIRY;
	cdb[1] = lun << 5;
	cdb[4] = 35;
	memset(&args, 0, sizeof(iscsi_scsi_cmd_args_t));
	args.input = 1;
	args.trans_len = 36;
	args.cdb = cdb;
	args.lun = lun;
	args.recv_data = data;
	memset(&cmd, 0, sizeof(initiator_cmd_t));
	cmd.isid = target;
	cmd.type = ISCSI_SCSI_CMD;
	cmd.ptr = &args;

	if (initiator_command(&cmd) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "initiator_command() failed\n");
		return -1;
	}
	if (args.status) {
		iscsi_trace_error(__FILE__, __LINE__, "INQUIRY failed (status %#x)\n", args.status);
		return -1;
	}
	*device_type = data[0] & 0x1f;
	iscsi_trace(TRACE_SCSI_DEBUG, __FILE__, __LINE__, "Device Type %#x\n", *device_type);

	return 0;
}


int 
read_capacity(uint64_t target, uint32_t lun, uint32_t *max_lba, uint32_t *block_len)
{
	uint8_t   data[8], cdb[16];
	initiator_cmd_t cmd;
	iscsi_scsi_cmd_args_t args;

	memset(cdb, 0, 16);
	cdb[0] = READ_CAPACITY;
	cdb[1] = lun << 5;

	memset(&args, 0, sizeof(iscsi_scsi_cmd_args_t));
	args.recv_data = data;
	args.input = 1;
	args.lun = lun;
	args.trans_len = 8;
	args.cdb = cdb;

	cmd.isid = target;
	cmd.type = ISCSI_SCSI_CMD;
	cmd.ptr = &args;

	if (initiator_command(&cmd) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "initiator_command() failed\n");
		return -1;
	}
	if (args.status) {
		iscsi_trace_error(__FILE__, __LINE__, "READ_CAPACITY failed (status %#x)\n", args.status);
		return -1;
	}
	*max_lba = ISCSI_NTOHL(*((uint32_t *) (data)));
	*block_len = ISCSI_NTOHL(*((uint32_t *) (data + 4)));
	iscsi_trace(TRACE_SCSI_DEBUG, __FILE__, __LINE__, "Max LBA (lun %u):   %u\n", lun, *max_lba);
	iscsi_trace(TRACE_SCSI_DEBUG, __FILE__, __LINE__, "Block Len (lun %u): %u\n", lun, *block_len);
	if (*max_lba == 0) {
		iscsi_trace_error(__FILE__, __LINE__, "Device returned Maximum LBA of zero\n");
		return -1;
	}
	if (*block_len % 2) {
		iscsi_trace_error(__FILE__, __LINE__, "Device returned strange block len: %u\n", *block_len);
		return -1;
	}
	return 0;
}

/*
 * write_read_test() writes a pattern to the first and last block of the
 * target:lun
 */
/*
 * specified and then reads back this pattern.  <type> is either 6 or 10 ans
 * specifies
 */
/* WRITE_6/READ_6 and WRITE_10/READ_10, respectively.  */

int 
write_read_test(uint64_t target, uint32_t lun, int type)
{
	iscsi_scsi_cmd_args_t	args;
	initiator_cmd_t		cmd;
	uint32_t        	max_lba;
	uint32_t        	block_len;
	uint32_t		i;
	uint16_t		len = 1;
	uint8_t   		data[4096];
	uint8_t   		cdb[16];
	int             	j;

	if ((type != 6) && (type != 10)) {
		iscsi_trace_error(__FILE__, __LINE__, "bad type, select 6 or 10\n");
		return -1;
	}
	/* determine last block on device */

	if (read_capacity(target, lun, &max_lba, &block_len) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "read_capacity() failed\n");
		return -1;
	}
	/* write pattern into first and last block on device */

	for (i = 0; i <= max_lba; i += max_lba) {
		for (j = 0; j < block_len; j++)
			data[j] = (uint8_t) (i + j);
		memset(cdb, 0, 16);
		if (type == 10) {
			cdb[0] = WRITE_10;
			cdb[1] = lun << 5;
			lba2cdb(cdb, &i, &len);
		} else {
			*((uint32_t *) (cdb + 0)) = ISCSI_HTONL(i);
			cdb[0] = WRITE_6;
			cdb[1] = (cdb[1] & 0x1f) | lun << 5;
			cdb[4] = len;
		}
		memset(&args, 0, sizeof(iscsi_scsi_cmd_args_t));
		args.send_data = data;
		args.output = 1;
		args.length = block_len;
		args.lun = lun;
		args.trans_len = block_len;
		args.cdb = cdb;

		cmd.isid = target;
		cmd.ptr = &args;
		cmd.type = ISCSI_SCSI_CMD;
		if (initiator_command(&cmd) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "initiator_command() failed\n");
			return -1;
		}
		if (args.status) {
			iscsi_trace_error(__FILE__, __LINE__, "initiator_command() failed\n");
			return -1;
		}
	}

	/* read pattern back from first and last block */

	for (i = 0; i <= max_lba; i += max_lba) {
		memset(cdb, 0, 16);
		if (type == 10) {
			cdb[0] = READ_10;
			cdb[1] = lun << 5;
			lba2cdb(cdb, &i, &len);
		} else {
			*((uint32_t *) (cdb + 0)) = ISCSI_HTONL(i);
			cdb[0] = READ_6;
			cdb[1] = (cdb[1] & 0x1f) | lun << 5;
			cdb[4] = len;
		}
		memset(&args, 0, sizeof(iscsi_scsi_cmd_args_t));
		args.recv_data = data;
		args.input = 1;
		args.lun = lun;
		args.trans_len = block_len;
		args.cdb = cdb;
		cmd.isid = target;
		cmd.type = ISCSI_SCSI_CMD;
		cmd.ptr = &args;
		if (initiator_command(&cmd) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "initiator_command() failed\n");
			return -1;
		}
		if (args.status) {
			iscsi_trace_error(__FILE__, __LINE__, "initiator_command() failed\n");
			return -1;
		}
		for (j = 0; j < block_len; j++) {
			if (data[j] != (uint8_t) (i + j)) {
				iscsi_trace_error(__FILE__, __LINE__, "Bad byte. data[%d] = %u, expected %u.\n",
				       j, data[j], (uint8_t) (i + j));
				return -1;
			}
		}
	}
	return 0;
}

/*
 * WRITE_10|READ_10
 */

int 
read_or_write(uint64_t target, uint32_t lun, uint32_t lba, uint32_t len,
	      uint32_t block_len, uint8_t *data, int sg_len, int writing)
{
	initiator_cmd_t cmd;
	iscsi_scsi_cmd_args_t args;
	uint8_t   cdb[16];

	/* Build CDB */

	cdb[0] = writing ? WRITE_10 : READ_10;
	cdb[1] = lun << 5;
	cdb[2] = ((uint8_t *) &lba)[3];
	cdb[3] = ((uint8_t *) &lba)[2];
	cdb[4] = ((uint8_t *) &lba)[1];
	cdb[5] = ((uint8_t *) &lba)[0];
	cdb[7] = ((uint8_t *) &len)[1];
	cdb[8] = ((uint8_t *) &len)[0];

	/* Build iSCSI command */

	memset(&args, 0, sizeof(iscsi_scsi_cmd_args_t));
	args.lun = lun;
	args.output = writing ? 1 : 0;
	args.input = writing ? 0 : 1;
	args.trans_len = len * block_len;
	args.send_data = writing ? data : NULL;
	args.send_sg_len = writing ? sg_len : 0;
	args.recv_data = writing ? NULL : data;
	args.recv_sg_len = writing ? 0 : sg_len;
	args.cdb = cdb;
	cmd.isid = target;
	cmd.ptr = &args;
	cmd.type = ISCSI_SCSI_CMD;

	/* Execute iSCSI command */

	if (initiator_command(&cmd) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "initiator_command() failed\n");
		return -1;
	}
	if (args.status) {
		iscsi_trace_error(__FILE__, __LINE__, "scsi_command() failed (status %#x)\n", args.status);
		return -1;
	}
	return 0;
}

int 
throughput_test(uint32_t target, uint32_t lun, uint32_t length, uint32_t request, uint32_t verbose, int writing, int sg_factor)
{
	uint32_t        max_lba, block_len;
	uint8_t **data;
	uint32_t        iters;
	uint32_t        num_blocks;
	uint32_t        block_offset;
	struct iovec   *sg;
	int             i, j;
	struct timeval  t_start, t_stop;
	double          seconds;

	/* Get device block len & capacity */

	if (read_capacity(target, lun, &max_lba, &block_len) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "read_capacity() failed\n");
		return -1;
	}
	if (request % block_len) {
		iscsi_trace_error(__FILE__, __LINE__, "request must be a multiple of %u\n", block_len);
		return -1;
	}
	if (!sg_factor) {
		iscsi_trace_error(__FILE__, __LINE__, "sg_factor must be at least 1\n");
		return -1;
	}
	if (request % sg_factor) {
		iscsi_trace_error(__FILE__, __LINE__, "request must be a multiple of sg_factor\n");
		return -1;
	}
	if (length % request) {
		iscsi_trace_error(__FILE__, __LINE__, "length must be a multiple of request\n");
		return -1;
	}
	if (length > ((max_lba + 1) * block_len)) {
		iscsi_trace_error(__FILE__, __LINE__, "attempt to read past device (max length %u)\n", max_lba * block_len);
		return -1;
	}
	if ((sg = iscsi_malloc(sg_factor * sizeof(struct iovec))) == NULL) {
		iscsi_trace_error(__FILE__, __LINE__, "out of memory\n");
		return -1;
	}
	if ((data = iscsi_malloc(sg_factor * sizeof(uint8_t *))) == NULL) {
		iscsi_trace_error(__FILE__, __LINE__, "out of memory\n");
		return -1;
	}
	for (i = 0; i < sg_factor; i++) {
		if ((data[i] = iscsi_malloc(request / sg_factor)) == NULL) {
			iscsi_trace_error(__FILE__, __LINE__, "out of memory\n");
			return -1;
		}
	}

	gettimeofday(&t_start, 0);

	iters = length / request;
	num_blocks = request / block_len;
	for (i = 0; i < iters; i++) {
		block_offset = i * num_blocks;

		/* The iSCSI initiator may modify our sg list */

		for (j = 0; j < sg_factor; j++) {
			sg[j].iov_base = data[j];
			sg[j].iov_len = request / sg_factor;
		}
		if (read_or_write(target, lun, block_offset, num_blocks, block_len, (uint8_t *) sg, sg_factor, writing) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "read_10() failed\n");
			goto done;
		}
		if (verbose && !((i + 1) % verbose)) {
			printf("%u total bytes %s: this request (lba %u, len %u)\n",
			      (i + 1) * request, writing ? "written" : "read", block_offset, num_blocks);
		}
	}

	gettimeofday(&t_stop, 0);
	seconds = toSeconds(t_stop) - toSeconds(t_start);

	/* Output results */

	printf("%u MB %s in %.2f seconds --> %.2f MB/sec\n", length / 1048576, writing ? "written" : "read", (double) seconds,
	      (double) (length / seconds) / 1048576);

done:	for (i = 0; i < sg_factor; i++) {
		if (data[i])
			iscsi_free(data[i]);
	}
	iscsi_free(data);
	iscsi_free(sg);

	return 0;
}

int 
integrity_test(uint32_t target, uint32_t lun, uint32_t length, int sg_factor)
{
	uint32_t        max_lba, block_len;
	uint8_t **data;
	struct iovec   *sg;
	int             i, j;

	/* Get device block len & capacity; and check args */

	if (read_capacity(target, lun, &max_lba, &block_len) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "read_capacity() failed\n");
		return -1;
	}
	if (length % block_len) {
		iscsi_trace_error(__FILE__, __LINE__, "length must be a multiple of block len %u\n", block_len);
		return -1;
	}
	if (!sg_factor) {
		iscsi_trace_error(__FILE__, __LINE__, "sg_factor must be at least 1\n");
		return -1;
	}
	if (length % sg_factor) {
		iscsi_trace_error(__FILE__, __LINE__, "length must be a multiple of sg_factor\n");
		return -1;
	}
	if (length > ((max_lba + 1) * block_len)) {
		iscsi_trace_error(__FILE__, __LINE__, "attempt to read past device (max length %u)\n", max_lba * block_len);
		return -1;
	}
	/* Allocate sg and data buffers; fill data buffers with pattern */

	if ((sg = iscsi_malloc(sg_factor * sizeof(struct iovec))) == NULL) {
		iscsi_trace_error(__FILE__, __LINE__, "out of memory\n");
		return -1;
	}
	if ((data = iscsi_malloc(sg_factor * sizeof(uint8_t *))) == NULL) {
		iscsi_trace_error(__FILE__, __LINE__, "out of memory\n");
		return -1;
	}
	for (i = 0; i < sg_factor; i++) {
		if ((data[i] = iscsi_malloc(length / sg_factor)) == NULL) {
			iscsi_trace_error(__FILE__, __LINE__, "out of memory\n");
			return -1;
		}
		for (j = 0; j < (length / sg_factor); j++) {
			/* data[i][j] = i+j; */
			data[i][j] = i + 1;
		}
	}

	/* Write blocks */

	for (j = 0; j < sg_factor; j++) {
		sg[j].iov_base = data[j];
		sg[j].iov_len = length / sg_factor;
	}
	if (read_or_write(target, lun, 0, length / block_len, block_len, (uint8_t *) sg, sg_factor, 1) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "read_or_write() failed\n");
		goto done;
	}
	for (i = 0; i < sg_factor; i++) {
		for (j = 0; j < (length / sg_factor); j++) {
			/* if (data[i][j] != (uint8_t)(i+j)) { */
			if (data[i][j] != (uint8_t) (i + 1)) {
				iscsi_trace_error(__FILE__, __LINE__, "Bad byte data[%d][%d]: got %u, expected %u\n", i, j, data[i][j], (uint8_t) (i + j));
				goto done;
			}
		}
	}

	/* Read blocks */

	for (j = 0; j < sg_factor; j++) {
		memset(data[j], 0, length / sg_factor);
		sg[j].iov_base = data[j];
		sg[j].iov_len = length / sg_factor;
	}
	if (read_or_write(target, lun, 0, length / block_len, block_len, (uint8_t *) sg, sg_factor, 0) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "read_or_write() failed\n");
		goto done;
	}
	for (i = 0; i < sg_factor; i++) {
		for (j = 0; j < (length / sg_factor); j++) {
			/* if (data[i][j] != (uint8_t)(i+j)) { */
			if (data[i][j] != (uint8_t) (i + 1)) {
				iscsi_trace_error(__FILE__, __LINE__, "Bad byte data[%d][%d]: got %u, expected %u\n", i, j, data[i][j], (uint8_t) (i + j));
				goto done;
			}
		}
	}

done:
	for (i = 0; i < sg_factor; i++) {
		if (data[i])
			iscsi_free(data[i]);
	}
	iscsi_free(data);
	iscsi_free(sg);

	return 0;
}

int 
nop_test(uint32_t target, uint32_t lun, uint32_t iters)
{
	struct timeval  t_start, t_stop;
	char           *data;
	int             i, j, k;

	if ((data = iscsi_malloc(4096)) == NULL) {
		iscsi_trace_error(__FILE__, __LINE__, "iscsi_malloc() failed\n");
		return -1;
	}
	/* Fill with some pattern */

	for (i = 0; i < 4096; i++)
		data[i] = (uint8_t) i;

	for (k = 0; k <= 1; k++) {	/* 0 = no ping, 1= ping */
		for (j = 0; j <= 4096; j *= 4) {	/* payload between 0 and
							 * 4K  */
			gettimeofday(&t_start, 0);
			for (i = 0; i < iters; i++) {
				if (nop_out(target, lun, j, k, data) != 0) {
					iscsi_trace_error(__FILE__, __LINE__, "nop_out() failed\n");
					return -1;
				}
			}
			gettimeofday(&t_stop, 0);

			/* Output results */

			printf("NOP_OUT (%4i bytes, ping = %d): %u iters in %.2f sec --> %.2f usec\n", j, k,
			      iters, toSeconds(t_stop) - toSeconds(t_start),
			      ((toSeconds(t_stop) - toSeconds(t_start)) * 1e6) / iters);
			if (!j)
				j = 1;
		}
	}
	iscsi_free(data);

	return 0;
}

static const char *
humanise(uint8_t op)
{
	switch(op) {
	case TEST_UNIT_READY:
		return "TEST_UNIT_READY";
	case INQUIRY:
		return "INQUIRY";
	case READ_10:
		return "READ_10";
	case WRITE_10:
		return "WRITE_10";
	case READ_CAPACITY:
		return "READ CAPACITY";
	default:
		return "unknown";
	}
}

/* latency_test() performs <op> for a number of iterations and outputs */
/* the average latency.  <op> can be any of WRITE_10, READ_10,  */
/* TEST_UNIT_READY, READ_CAPACITY or INQUIRY. */

int 
latency_test(uint64_t target, uint32_t lun, uint8_t op, uint32_t iters)
{
	uint32_t        length, trans_len;
	uint32_t        max_lba, block_len;
	int             input, output;
	uint8_t  *data, cdb[16];
	initiator_cmd_t cmd;
	iscsi_scsi_cmd_args_t args;
	struct timeval  t_start, t_stop;
	int             i, rc = -1;
	uint32_t        lba = 0;
	uint16_t  len = 1;

	/* Get device block len */

	if ((op == WRITE_10) || (op == READ_10)) {
		if (read_capacity(target, lun, &max_lba, &block_len) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "read_capacity() failed\n");
			return -1;
		}
		if ((data = iscsi_malloc(block_len)) == NULL) {
			iscsi_trace_error(__FILE__, __LINE__, "out of memory\n");
			return -1;
		}
	} else {
		if ((data = iscsi_malloc(1024)) == NULL) {
			iscsi_trace_error(__FILE__, __LINE__, "out of memory\n");
			return -1;
		}
	}


	/* Build CDB: */

	memset(cdb, 0, 16);
	input = output = length = trans_len = 0;
	cdb[1] = lun << 5;
	switch (op) {
	case TEST_UNIT_READY:
		cdb[0] = TEST_UNIT_READY;
		break;
	case INQUIRY:
		cdb[0] = INQUIRY;
		cdb[4] = 35;
		input = 1;
		trans_len = 36;
		break;
	case READ_CAPACITY:
		cdb[0] = READ_CAPACITY;
		input = 1;
		trans_len = 8;
		break;
	case READ_10:
		input = 1;
		trans_len = block_len;
		cdb[0] = READ_10;
		cdb[1] = lun << 5;
		lba2cdb(cdb, &lba, &len);
		break;
	case WRITE_10:
		output = 1;
		trans_len = block_len;
		cdb[0] = WRITE_10;
		cdb[1] = lun << 5;
		lba2cdb(cdb, &lba, &len);
		break;
	default:
		iscsi_trace_error(__FILE__, __LINE__, "op %#x not implemented\n", op);
		return -1;
	}

	/* Build iSCSI command */

	memset(&args, 0, sizeof(iscsi_scsi_cmd_args_t));
	args.lun = lun;
	args.cdb = cdb;
	args.input = input;
	args.output = output;
	args.trans_len = trans_len;
	args.send_data = output ? data : NULL;
	args.recv_data = input ? data : NULL;

	cmd.isid = target;
	cmd.type = ISCSI_SCSI_CMD;
	cmd.ptr = &args;

	/* Run test */

	gettimeofday(&t_start, 0);
	for (i = 0; i < iters; i++) {
		if (initiator_command(&cmd) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "initiator_command() failed\n");
			goto done;
		}
		if (args.status) {
			iscsi_trace_error(__FILE__, __LINE__, "scsi_command() failed (status %#x)\n", args.status);
			goto done;
		}
	}
	gettimeofday(&t_stop, 0);

	/* Output results */

	printf("SCSI op 0x%2x (%s): %u iters in %.2f sec --> %.2f usec\n",
	      op, humanise(op), iters, toSeconds(t_stop) - toSeconds(t_start),
	      ((toSeconds(t_stop) - toSeconds(t_start)) * 1e6) / iters);

	rc = 0;
done:
	iscsi_free(data);
	return rc;
}

/*
 * scatter_gather() tests scatter/gather performance for WRITE_10 and
 * READ_10.  Instead of specifying a data buffer in args.send_data and
 * arg.recv_data, we specify a scatter/gather list.  */

int 
scatter_gather_test(uint64_t target, uint32_t lun, uint8_t op)
{
	uint32_t        length, trans_len;
	uint8_t   cdb[16];
	uint32_t        lba = 0;
	struct iovec   *sg;
	uint8_t **buff;
	uint32_t        block_len, max_lba;
	uint16_t  len;
	int             i, n;
	int             input, output;
	initiator_cmd_t cmd;
	iscsi_scsi_cmd_args_t args;
	int             xfer_size = 100 * 1048576;
	int             xfer_chunk = 1048576;
	int             rc = -1;
	struct timeval  t_start, t_stop;

	/* Number of iterations (xfer_chunk bytes read/written per iteration) */

	if (xfer_size % xfer_chunk) {
		iscsi_trace_error(__FILE__, __LINE__, "xfer_size (%d) is not a multiple of xfer_chunk (%d)\n", xfer_size, xfer_chunk);
		return -1;
	}
	n = xfer_size / xfer_chunk;

	/* Number of blocks per iteration */

	if (read_capacity(target, lun, &max_lba, &block_len) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "read_capacity() failed\n");
		return -1;
	}
	if (xfer_chunk % block_len) {
		iscsi_trace_error(__FILE__, __LINE__, "xfer_chunk (%d) is not a multiple of block_len (%d)\n", xfer_chunk, block_len);
		return -1;
	}
	len = xfer_chunk / block_len;

	/* Build CDB */

	memset(cdb, 0, 16);
	cdb[1] = lun << 5;
	trans_len = block_len * len;
	length = input = output = 0;
	switch (op) {
	case WRITE_10:
		cdb[0] = WRITE_10;
		output = 1;
		length = trans_len;
		break;
	case READ_10:
		cdb[0] = READ_10;
		input = 1;
		break;
	default:
		iscsi_trace_error(__FILE__, __LINE__, "scatter/gather test not implemented for SCSI op %#x\n", op);
		return -1;
	}

	/* Allocate buffers for scatter/gather */

	if ((buff = iscsi_malloc(len * sizeof(uint8_t *))) == NULL) {
		iscsi_trace_error(__FILE__, __LINE__, "out of memory\n");
		return -1;
	}
	for (i = 0; i < len; i++) {
		buff[i] = iscsi_malloc(block_len);
		if (buff[i] == NULL) {
			iscsi_trace_error(__FILE__, __LINE__, "out of memory\n");
		}
	}
	if ((sg = iscsi_malloc(len * sizeof(struct iovec))) == NULL) {
		iscsi_trace_error(__FILE__, __LINE__, "out of memory\n");
		return -1;
	}
	for (i = 0; i < len; i++) {
		sg[i].iov_base = buff[i];
		sg[i].iov_len = block_len;
	}
	lba2cdb(cdb, &lba, &len);

	gettimeofday(&t_start, 0);

	/* Begin I/O  */

	for (i = 0; i < n; i++) {

#if 0
		for (j = 0; j < len; j++) {
			sg[j].iov_base = buff[j];
			sg[j].iov_len = block_len;
		}
#endif

		memset(&args, 0, sizeof(iscsi_scsi_cmd_args_t));
		args.send_data = output ? ((uint8_t *) sg) : NULL;
		args.send_sg_len = output ? len : 0;
		args.recv_data = input ? ((uint8_t *) sg) : NULL;
		args.recv_sg_len = input ? len : 0;
		args.input = input;
		args.output = output;
		args.length = length;
		args.lun = lun;
		args.trans_len = trans_len;
		args.cdb = cdb;

		cmd.isid = target;
		cmd.type = ISCSI_SCSI_CMD;
		cmd.ptr = &args;

		if (initiator_command(&cmd) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "initiator_command() failed\n");
			goto done;
		}
		if (args.status) {
			iscsi_trace_error(__FILE__, __LINE__, "scsi_command() failed (status %#x)\n", args.status);
			goto done;
		}
	}

	gettimeofday(&t_stop, 0);
	printf("SCSI op %#x (%s): %u bytes (%s) in %.2f secs --> %.2f MB/sec\n",
	       op, humanise(op), xfer_size, (op == WRITE_10) ? "gathered" : "scattered",
	       toSeconds(t_stop) - toSeconds(t_start),
	  (xfer_size / 1048576) / (toSeconds(t_stop) - toSeconds(t_start)));
	rc = 0;

done:
	for (i = 0; i < len; i++) {
		iscsi_free(buff[i]);
	}
	iscsi_free(buff);
	iscsi_free(sg);
	return rc;
}

/*
 * OSD Tests
 */

static int 
osd_tests(int target, int lun)
{
	uint32_t        GroupID;
	uint64_t        UserID;
	char            buffer[1024];
	OSD_DEVICE_T    dev = {0, 0};
	uint16_t        len;

	if (osd_create_group((void *) &dev, &osd_command, &GroupID) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "osd_create_group() failed\n");
		return -1;
	}
	printf("OSD_CREATE_GROUP: PASSED\n");

	if (osd_create((void *) &dev, GroupID, &osd_command, &UserID) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "osd_create() failed\n");
		return -1;
	}
	printf("OSD_CREATE: PASSED\n");

	if (osd_write((void *) &dev, GroupID, UserID, 0, 13, "Hello, World!", 0, &osd_command) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "osd_write() failed\n");
		return -1;
	}
	printf("OSD_WRITE: PASSED\n");

	if (osd_read((void *) &dev, GroupID, UserID, 0, 13, buffer, 0, &osd_command) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "osd_write() failed\n");
	}
	if (strncmp(buffer, "Hello, World!", 13)) {
		iscsi_trace_error(__FILE__, __LINE__, "osd_read() failed\n");
		return -1;
	}
	printf("OSD_READ: PASSED\n");

	if (osd_set_one_attr((void *) &dev, GroupID, UserID, 0x30000000, 0x1, 480, buffer, &osd_command) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "osd_write() failed\n");
	}
	printf("OSD_SET_ATTR: PASSED\n");

	if (osd_get_one_attr((void *) &dev, GroupID, UserID, 0x30000000, 0, 480, &osd_command, &len, buffer) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "osd_get_one_attr() failed\n");
	}
	if (strncmp(buffer, "Hello, World!", 13)) {
		iscsi_trace_error(__FILE__, __LINE__, "osd_read() failed\n");
		return -1;
	}
	printf("OSD_GET_ATTR: PASSED\n");

	if (osd_remove((void *) &dev, GroupID, UserID, &osd_command) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "osd_remove() failed\n");
		return -1;
	}
	printf("OSD_REMOVE: PASSED\n");

	if (osd_remove_group((void *) &dev, GroupID, &osd_command) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "osd_remove_group() failed\n");
		return -1;
	}
	printf("OSD_REMOVE: PASSED\n");

	return 0;
}

static int 
disk_tests(int target, int lun)
{
	uint32_t        request_size;
	uint32_t        max_lba, block_len;
	uint32_t        min_request_size = 8192;
	uint32_t        max_request_size = 262144;
	uint32_t        xfer_size = 32 * 262144;

	/* Initial Tests */

	if (read_capacity(target, lun, &max_lba, &block_len) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "read_capacity() failed\n");
		return -1;
	}
	printf("read_capacity PASSED\n");
	if (write_read_test(target, lun, 10) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "write_read_test() failed\n");
		return -1;
	}
	printf("write_read_test PASSED\n");
	if (integrity_test(target, lun, max_request_size, 1024) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "integrity_test() failed\n");
		return -1;
	}
	printf("integrity_test PASSED\n");

	/* Latency Tests */

	if (latency_test(target, lun, READ_10, 1000) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "latency_test(READ_10) failed\n");
		return -1;
	}
	if (latency_test(target, lun, WRITE_10, 1000) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "latency_test(WRITE_10) failed\n");
		return -1;
	}
	if (latency_test(target, lun, READ_CAPACITY, 1000) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "latency_test(READ_CAPACITY) failed\n");
		return -1;
	}
	/* Throughput Tests */

	for (request_size = min_request_size; request_size <= max_request_size; request_size *= 2) {
		printf("%u bytes/request: ", request_size);
		if (throughput_test(target, lun, xfer_size, request_size, (xfer_size / request_size) + 1, 1, 1) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "througput_test() failed\n");
			return -1;
		}
	}
	for (request_size = min_request_size; request_size <= max_request_size; request_size *= 2) {
		printf("%u bytes/request: ", request_size);
		if (throughput_test(target, lun, xfer_size, request_size, (xfer_size / request_size) + 1, 0, 1) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "througput_test() failed\n");
			return -1;
		}
	}

	return 0;
}

int 
test_all(int target, int lun)
{
	uint32_t        device_type = 0;
	const char           *data = "Hello, world!";

	/* Initial Tests */

	printf("##BEGIN INITIAL TESTS[%d:%d]##\n", target, lun);
	if (nop_out(target, lun, 13, 0, data) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "nop_out() failed\n");
		return -1;
	}
	printf("nop_out() PASSED\n");
	if (nop_out(target, lun, 13, 1, data) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "nop_out() w/ ping failed\n");
		return -1;
	}
	printf("nop_out() w/ ping PASSED\n");
	if (inquiry(target, lun, &device_type) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "inquiry() failed\n");
		return -1;
	}
	printf("inquiry() PASSED: device type %#x\n", device_type);
	printf("##END INITIAL TESTS[%d:%d]##\n\n", target, lun);

	/* iSCSI Latency Tests */

	printf("##BEGIN iSCSI LATENCY TESTS[%d:%d]##\n", target, lun);
	if (nop_test(target, lun, 1000) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "nop_test() failed\n");
		return -1;
	}
	printf("##END iSCSI LATENCY TESTS[%d:%d]##\n\n", target, lun);

	/* SCSI Latency Tests */

	printf("##BEGIN SCSI LATENCY TESTS[%d:%d]##\n", target, lun);
	if (latency_test(target, lun, TEST_UNIT_READY, 1000) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "latency_test(TEST_UNIT_READY) failed\n");
		return -1;
	}
	if (latency_test(target, lun, INQUIRY, 1000) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "latency_test(INQUIRY) failed\n");
		return -1;
	}
	printf("##END SCSI LATENCY TESTS[%d:%d]##\n\n", target, lun);

	/* Device-specific tests */

	printf("##BEGIN DEVICE-SPECIFIC TESTS[%d:%d]##\n", target, lun);
	switch (device_type) {
	case 0x00:
		if (disk_tests(target, lun) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "disk_tests() failed\n");
			return -1;
		}
		break;
	case 0x0e:
		if (osd_tests(target, lun) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "osd_tests() failed\n");
			return -1;
		}
		break;
	default:
		break;
	}
	printf("##END DEVICE-SPECIFIC TESTS[%d:%d]##\n\n", target, lun);

	return 0;
}

int 
ii_test_all(void)
{
	uint32_t        device_type = 0;
	const char           *data = "Hello, world!";
	int		target = 0;
	int		lun = 0;

	/* Initial Tests */

	printf("##BEGIN INITIAL TESTS[%d:%d]##\n", target, lun);
	if (nop_out(target, lun, 13, 0, data) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "nop_out() failed\n");
		return -1;
	}
	printf("nop_out() PASSED\n");
	if (nop_out(target, lun, 13, 1, data) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "nop_out() w/ ping failed\n");
		return -1;
	}
	printf("nop_out() w/ ping PASSED\n");
	if (inquiry(target, lun, &device_type) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "inquiry() failed\n");
		return -1;
	}
	printf("inquiry() PASSED: device type %#x\n", device_type);
	printf("##END INITIAL TESTS[%d:%d]##\n\n", target, lun);

	/* iSCSI Latency Tests */

	printf("##BEGIN iSCSI LATENCY TESTS[%d:%d]##\n", target, lun);
	if (nop_test(target, lun, 1000) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "nop_test() failed\n");
		return -1;
	}
	printf("##END iSCSI LATENCY TESTS[%d:%d]##\n\n", target, lun);

	/* SCSI Latency Tests */

	printf("##BEGIN SCSI LATENCY TESTS[%d:%d]##\n", target, lun);
	if (latency_test(target, lun, TEST_UNIT_READY, 1000) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "latency_test(TEST_UNIT_READY) failed\n");
		return -1;
	}
	if (latency_test(target, lun, INQUIRY, 1000) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "latency_test(INQUIRY) failed\n");
		return -1;
	}
	printf("##END SCSI LATENCY TESTS[%d:%d]##\n\n", target, lun);

	/* Device-specific tests */

	printf("##BEGIN DEVICE-SPECIFIC TESTS[%d:%d]##\n", target, lun);
	switch (device_type) {
	case 0x00:
		if (disk_tests(target, lun) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "disk_tests() failed\n");
			return -1;
		}
		break;
	case 0x0e:
		if (osd_tests(target, lun) != 0) {
			iscsi_trace_error(__FILE__, __LINE__, "osd_tests() failed\n");
			return -1;
		}
		break;
	default:
		break;
	}
	printf("##END DEVICE-SPECIFIC TESTS[%d:%d]##\n\n", target, lun);

	return 0;
}

int 
osd_command(void *dev, osd_args_t * args, OSD_OPS_MEM * m)
{
	initiator_cmd_t initiator_cmd;
	iscsi_scsi_cmd_args_t scsi_cmd;
	uint8_t   ahs[1024], *ahs_ptr = ahs;
	int             ahs_len = 0;
	uint8_t   cdb[CONFIG_OSD_CDB_LEN];
	uint64_t target = ((OSD_DEVICE_T *) dev)->target;
	uint16_t        len;

	/* Build AHS for expected bidi read length */

	if (m->send_len && m->recv_len) {
		memset(ahs_ptr, 0, 8);
		len = 8;
		*((uint32_t *) ahs_ptr) = ISCSI_HTONS(len);	/* AHS length */
		ahs_ptr[2] = 0x02;	/* Type */
		*((uint32_t *) (ahs_ptr + 4)) = ISCSI_HTONL(m->recv_len);	/* Expected Read length */
		ahs_ptr += 8;
		ahs_len += 8;
	}
	/* Build AHS for extended CDB */

	OSD_ENCAP_CDB(args, cdb);
	/* OSD_PRINT_CDB(cdb, cdb+16); */
	memset(ahs_ptr, 0, 4);
	len = (CONFIG_OSD_CDB_LEN - 15);	/* AHS length */
	*((uint16_t *) ahs_ptr) = ISCSI_HTONS(len);	/* AHS length */
	ahs_ptr[2] = 0x01;	/* Type */
	memcpy(ahs_ptr + 4, cdb + 16, CONFIG_OSD_CDB_LEN - 16);	/* Copy remaining CDB */
	ahs_ptr += CONFIG_OSD_CDB_LEN - 15;
	ahs_len += CONFIG_OSD_CDB_LEN - 15;

	/* Build iscsi_scsi_cmd_args_t */

	memset(&scsi_cmd, 0, sizeof(iscsi_scsi_cmd_args_t));
	scsi_cmd.cdb = cdb;
	scsi_cmd.ahs_len = ahs_len;
	scsi_cmd.ahs = ahs;
	if (m->send_len && m->recv_len) {
		scsi_cmd.input = 1;
		scsi_cmd.output = 1;
		scsi_cmd.length = m->send_len;
		scsi_cmd.trans_len = m->send_len;
		scsi_cmd.bidi_trans_len = m->recv_len;
		scsi_cmd.send_data = __UNCONST(m->send_data); /* XXX - agc */
		scsi_cmd.send_sg_len = m->send_sg;
		scsi_cmd.recv_data = m->recv_data;
		scsi_cmd.recv_sg_len = m->recv_sg;
	} else if (m->send_len) {
		scsi_cmd.output = 1;
		scsi_cmd.length = m->send_len;
		scsi_cmd.trans_len = m->send_len;
		scsi_cmd.send_data = __UNCONST(m->send_data); /* XXX - agc */
		scsi_cmd.send_sg_len = m->send_sg;
	} else if (m->recv_len) {
		scsi_cmd.input = 1;
		scsi_cmd.trans_len = m->recv_len;
		scsi_cmd.recv_data = m->recv_data;
		scsi_cmd.recv_sg_len = m->recv_sg;
	}
	/* Build initiator_cmd_t */

	memset(&initiator_cmd, 0, sizeof(initiator_cmd_t));
	initiator_cmd.isid = target;
	initiator_cmd.type = ISCSI_SCSI_CMD;
	initiator_cmd.ptr = &scsi_cmd;

	/* Execute initiator_cmd_t  */

	if (initiator_command(&initiator_cmd) != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "initiator_command() failed\n");
		return -1;
	}
	if (scsi_cmd.status != 0) {
		iscsi_trace_error(__FILE__, __LINE__, "SCSI command failed\n");
		return -1;
	}
	return 0;
}
