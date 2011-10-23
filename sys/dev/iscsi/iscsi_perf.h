/*	$NetBSD: iscsi_perf.h,v 1.1 2011/10/23 21:15:02 agc Exp $	*/

/*-
 * Copyright (c) 2005,2006,2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wasabi Systems, Inc.
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

typedef enum
{
	PERF_BEGIN_COMMAND,			/* start of command */
	PERF_END_COMMAND,			/* end of command */
	PERF_BEGIN_PDUWRITECMD,		/* start of write of command PDU */
	PERF_END_PDUWRITECMD,		/* end of write of command PDU */
	PERF_BEGIN_PDUWRITEDATA,	/* start of first data PDU write (write only) */
	PERF_END_PDUWRITEDATA,		/* end of last data PDU write */
	PERF_BEGIN_PDURCVDATA,		/* start of first data PDU receive (read only) */
	PERF_END_PDURCVDATA,		/* end of last data PDU receive */
	PERF_PDURCVSTS,				/* receive of status PDU */
	PERF_NUM_PERFPOINTS
} perfpoint_t;


#define PERF_FLG_READ      0x10000000	/* this is a read command */
#define PERF_FLG_NOWAIT    0x20000000	/* this command was non-waiting */
#define PERF_FLG_COMPLETE  0x80000000	/* command completed */


typedef struct
{
	uint64_t pctr[PERF_NUM_PERFPOINTS];
	uint32_t datalen;			/* data transfer size */
	uint32_t status;			/* Result of command (lower 16 bits) + flags */
} command_perf_t;


/*
   Perfdata_Start:
      Allocates data buffer in driver with given number of command_perf
      elements. If a buffer of larger or equal size already exists, it is
      not reallocated.
      Resets indices and clears the buffer.

   Perfdata_Stop:
      Stops data collection, waits (up to 5 seconds) for completion.
      Returns the number of data points collected.
*/

typedef struct
{
	int num_elements;			/* start: IN number of elements to allocate */
								/* stop: OUT number of elements used */
	uint32_t status;			/* out: status */
} iscsi_perf_startstop_parameters_t;


#define ISCSI_PERFDATA_START _IOWR (0, 90, iscsi_perf_startstop_parameters_t)
#define ISCSI_PERFDATA_STOP  _IOWR (0, 91, iscsi_perf_startstop_parameters_t)


/*
   Perfdata_Get:
      Retrieves the current data. Note that this may be called while data
      collection is still active, it does not automatically stop collection.
      It will not reset collection or release  any buffers.

      To determine the required buffer size, call with buffersize set to zero.
      In that case, no data is transferred, the buffersize field is set to the
      required size in bytes, and num_elements is set to the corresponding
      number of data points.

      If buffersize is nonzero, the number of data points that will fit into
      the buffer, or the number of data points collected so far, will be
      copied into the buffer, whichever is smallest. num_elements is set to
      the number of elements copied.

      ticks_per_100ms is the approximate conversion base to convert the CPU
      cycle counter values collected into time units. This value is determined
      by measuring the cycles for a delay(100) five times and taking the
      average.
*/

typedef struct
{
	void *buffer;				/* in: buffer pointer */
	uint32_t buffersize;		/* in: size of buffer in bytes (0 to query size) */
	uint32_t status;			/* out: status */
	int num_elements;			/* out: number of elements written */
	uint64_t ticks_per_100us;	/* out: ticks per 100 usec */
} iscsi_perf_get_parameters_t;


#define ISCSI_PERFDATA_GET   _IOWR (0, 92, iscsi_perf_get_parameters_t)
