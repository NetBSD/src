/*	$Id: at91twivar.h,v 1.2.86.1 2020/04/08 14:07:28 martin Exp $	*/
/*	$NetBSD: at91twivar.h,v 1.2.86.1 2020/04/08 14:07:28 martin Exp $	*/

/*-
 * Copyright (c) 2007 Embedtronics Oy. All rights reserved.
 *
 * Based on arch/macppc/dev/ki2c.c,
 * Copyright (c) 2001 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_AT91TWIVAR_H_
#define	_AT91TWIVAR_H_	1

struct at91twi_softc {
	device_t		sc_dev;		/* generic device	*/
	bus_space_tag_t		sc_iot;		/* I/O space tag	*/
	bus_space_handle_t	sc_ioh;		/* I/O space handle	*/

	int			sc_pid;		/* peripheral id	*/
	struct i2c_controller	sc_i2c;		/* I2C device desc	*/

	void			*sc_ih;		/* interrupt handle	*/

	int			sc_flags;
#define	I2C_BUSY	0x0001
#define	I2C_READING	0x0002
#define	I2C_ERROR	0x8000
	u_char			*sc_data;
	int			sc_resid;
};

#endif	// !_AT91TWIVAR_H_
