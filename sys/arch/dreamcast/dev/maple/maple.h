/*	$NetBSD: maple.h,v 1.9.26.1 2007/03/12 05:47:36 rmind Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by ITOH Yasufumi.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*-
 * Copyright (c) 2001 Marcus Comstedt
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
 *	This product includes software developed by Marcus Comstedt.
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

#ifndef _DREAMCAST_DEV_MAPLE_MAPLE_H_
#define _DREAMCAST_DEV_MAPLE_MAPLE_H_

/* signed is more effective than unsigned on SH */
typedef int8_t maple_response_t;

/* Maple Bus command and response codes */

#define MAPLE_RESPONSE_LCDERR	 (-6)
#define MAPLE_RESPONSE_FILEERR	 (-5)
#define MAPLE_RESPONSE_AGAIN	 (-4)	/* request should be retransmitted */
#define MAPLE_RESPONSE_BADCMD	 (-3)
#define MAPLE_RESPONSE_BADFUNC	 (-2)
#define MAPLE_RESPONSE_NONE	 (-1)	/* unit didn't respond at all */
#define MAPLE_COMMAND_DEVINFO	 1
#define MAPLE_COMMAND_ALLINFO	 2
#define MAPLE_COMMAND_RESET	 3
#define MAPLE_COMMAND_KILL	 4
#define MAPLE_RESPONSE_DEVINFO	 5
#define MAPLE_RESPONSE_ALLINFO	 6
#define MAPLE_RESPONSE_OK	 7
#define MAPLE_RESPONSE_DATATRF	 8
#define MAPLE_COMMAND_GETCOND	 9
#define MAPLE_COMMAND_GETMINFO	 10
#define MAPLE_COMMAND_BREAD	 11
#define MAPLE_COMMAND_BWRITE	 12
#define MAPLE_COMMAND_GETLASTERR 13
#define MAPLE_COMMAND_SETCOND	 14

/* Function codes */
#define MAPLE_FN_CONTROLLER	0
#define MAPLE_FN_MEMCARD	1
#define MAPLE_FN_LCD		2
#define MAPLE_FN_CLOCK		3
#define MAPLE_FN_MICROPHONE	4
#define MAPLE_FN_ARGUN		5
#define MAPLE_FN_KEYBOARD	6
#define MAPLE_FN_LIGHTGUN	7
#define MAPLE_FN_PURUPURU	8
#define MAPLE_FN_MOUSE		9

#define MAPLE_FUNC(fn)		(1 << (fn))

struct maple_devinfo {
	uint32_t di_func;		/* function code */
	uint32_t di_function_data[3];	/* function data */
	uint8_t di_area_code;		/* region settings */
	uint8_t di_connector_direction; /* direction of expansion connector */
	char di_product_name[30];	/* name of the device */
	char di_product_license[60];	/* manufacturer info */
	uint16_t di_standby_power;	/* standby power consumption */
	uint16_t di_max_power;		/* maximum power consumption */
};

#define MAPLE_CONN_TOP		0	/* connector is to the top */
#define MAPLE_CONN_BOTTOM	1	/* connector is to the bottom */

struct maple_response {
	uint32_t	response_code;
	uint32_t	data[1];	/* variable length */
};

#define MAPLE_FLAG_PERIODIC		1
#define MAPLE_FLAG_CMD_PERIODIC_TIMING	2

struct maple_unit;

extern void	maple_set_callback(struct device *, struct maple_unit *, int,
		    void (*)(void *, struct maple_response *, int, int),
		    void *);
extern void	maple_enable_unit_ping(struct device *, struct maple_unit *,
		    int /*func*/, int /*enable*/);
extern void	maple_enable_periodic(struct device *, struct maple_unit *,
		    int /*func*/, int /*on*/);
extern void	maple_command(struct device *, struct maple_unit *,
		    int /*func*/, int /*command*/, int /*datalen*/,
		    const void *, int /*flags*/);
extern uint32_t	maple_get_function_data(struct maple_devinfo *, int);
extern void	maple_run_polling(struct device *);
extern int	maple_unit_ioctl(struct device *, struct maple_unit *,
		    u_long, void *, int, struct lwp *);

#endif /* _DREAMCAST_DEV_MAPLE_MAPLE_H_ */
