/*	$NetBSD: timepps.h,v 1.1 1998/06/10 08:18:58 jonathan Exp $	*/

/*
 * Copyright (c) 1998 Jonathan Stone
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Jonathan Stone for
 *      the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SYS_TIMEPPS_H_
#define _SYS_TIMEPPS_H_

#include <sys/ioccom.h>

typedef int32_t pps_handle_t;
typedef u_int32_t pps_seq_t;

typedef union pps_timeu {
	struct timespec	tspec;
 	struct {        /* NTP long fixed-point format */
		unsigned int     integral;
		unsigned int     fractional;
	} ntplfp;
	unsigned long   longpair[2];
} pps_timeu_t;

#define assert_timestamp	assert_tu.tspec
#define clear_timestamp		clear_tu.tspec


typedef struct {
	pps_seq_t	assert_sequence;	/* assert event seq # */
	pps_seq_t	clear_sequence;		/* clear event seq # */
	pps_timeu_t	assert_tu;
	pps_timeu_t	clear_tu;
	int		current_mode;		/* current mode bits */
} pps_info_t;


typedef struct {
	int mode;				/* mode bits */
	pps_timeu_t	assert_off_tu;
	pps_timeu_t	clear_off_tu;
} pps_params_t;
#define assert_offset		assert_off_tu.tspec
#define clear_offset		clear_off_tu.tspec

struct pps_wait_args {
	struct timespec	timeout;
	pps_info_t	pps_info_buf;
};

#define PPS_CAPTUREASSERT	0x01
#define PPS_CAPTURECLEAR	0x02
#define PPS_CAPTUREBOTH		0x03

#define PPS_HARDPPSONASSERT	0x04
#define PPS_HARDPPSONCLEAR	0x08

#define PPS_OFFSETASSERT	0x10
#define PPS_OFFSETCLEAR		0x20

#define PPS_ECHOASSERT		0x40
#define PPS_ECHOCLEAR		0x80

#define PPS_CANWAIT		0x100

#define PPS_TSFMT_TSPEC		0x1000
#define PPS_TSFMT_NTPLFP	0x2000

#define PPS_CREATE		_IO('1', 1)
#define PPS_DESTROY		_IO('1', 2)
#define PPS_SETPARAMS		_IOW('1', 3, pps_params_t)
#define PPS_GETPARAMS		_IOR('1', 4, pps_params_t)
#define PPS_GETCAP		_IOR('1', 5, int)
#define PPS_FETCH		_IOWR('1', 6, pps_info_t)
#define PPS_WAIT		_IOWR('1', 6, struct pps_wait_args)

#ifndef _KERNEL
int time_pps_create __P((int filedes, pps_handle_t *handle));
int time_pps_destroy __P((pps_handle_t handle));
int time_pps_setparams __P((pps_handle_t handle, 
	const pps_params_t *ppsparams));
int time_pps_getparams __P((pps_handle_t handle, pps_params_t *ppsparams));
int time_pps_getcap __P((pps_handle_t handle, int *mode));
int time_pps_fetch __P((pps_handle_t handle, pps_info_t *ppsinfobuf));
int time_pps_wait __P((pps_handle_t handle, const struct timespec *timeout,
	pps_info_t *ppsinfobuf));
#endif /* !_KERNEL*/

#endif /* SYS_TIMEPPS_H_ */
