/*	$NetBSD: maple.h,v 1.3.2.1 2002/06/23 17:35:34 jdolecek Exp $	*/

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

#include <sys/device.h>

#define MAPLE_PORTS 4
#define MAPLE_SUBUNITS 6

/* Maple Bus command and response codes */

#define MAPLE_RESPONSE_FILEERR -5
#define MAPLE_RESPONSE_AGAIN   -4  /* request should be retransmitted */
#define MAPLE_RESPONSE_BADCMD  -3
#define MAPLE_RESPONSE_BADFUNC -2
#define MAPLE_RESPONSE_NONE    -1  /* unit didn't respond at all */
#define MAPLE_COMMAND_DEVINFO  1
#define MAPLE_COMMAND_ALLINFO  2
#define MAPLE_COMMAND_RESET    3
#define MAPLE_COMMAND_KILL     4
#define MAPLE_RESPONSE_DEVINFO 5
#define MAPLE_RESPONSE_ALLINFO 6
#define MAPLE_RESPONSE_OK      7
#define MAPLE_RESPONSE_DATATRF 8
#define MAPLE_COMMAND_GETCOND  9
#define MAPLE_COMMAND_GETMINFO 10
#define MAPLE_COMMAND_BREAD    11
#define MAPLE_COMMAND_BWRITE   12
#define MAPLE_COMMAND_SETCOND  14


/* Function codes */

#define MAPLE_FUNC_CONTROLLER 0x001
#define MAPLE_FUNC_MEMCARD    0x002
#define MAPLE_FUNC_LCD        0x004
#define MAPLE_FUNC_CLOCK      0x008
#define MAPLE_FUNC_MICROPHONE 0x010
#define MAPLE_FUNC_ARGUN      0x020
#define MAPLE_FUNC_KEYBOARD   0x040
#define MAPLE_FUNC_LIGHTGUN   0x080
#define MAPLE_FUNC_PURUPURU   0x100
#define MAPLE_FUNC_MOUSE      0x200

struct maple_devinfo {
	u_int32_t di_func;		/* function code */
	u_int32_t di_function_data[3];  /* function data */
	u_int8_t di_area_code;		/* region settings */
	u_int8_t di_connector_direction;	/* ? */
	char di_product_name[30];	/* name of the device */
	char di_product_license[60];	/* manufacturer info */
	u_int16_t di_standby_power;	/* standby power consumption */
	u_int16_t di_max_power;		/* maximum power consumption */
};

struct maple_unit {
	int port, subunit;
	int status;
	u_int32_t getcond_func;
	void (*getcond_callback)(void *, void *, int);
	void *getcond_data;
	struct maple_devinfo devinfo;
};

extern void	maple_set_condition_callback(struct device *, int, int,
		    u_int32_t, void (*)(void *, void *, int), void *);
extern u_int32_t maple_get_function_data(struct maple_devinfo *, u_int32_t);
extern void	maple_run_polling(struct device *);

#endif /* _DREAMCAST_DEV_MAPLE_MAPLE_H_ */

