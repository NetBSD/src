/*	$NetBSD: svr4_sockmod.h,v 1.1 1994/11/14 06:13:19 christos Exp $	 */

/*
 * Copyright (c) 1994 Christos Zoulas
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#ifndef	_SVR4_SOCKMOD_H_
#define	_SVR4_SOCKMOD_H_

#define	SVR4_SIMOD 		('I' << 8)

#define	SVR4_SI_GETUDATA	(SVR4_SIMOD|101)
#define	SVR4_SI_SHUTDOWN	(SVR4_SIMOD|102)
#define	SVR4_SI_LISTEN		(SVR4_SIMOD|103)
#define	SVR4_SI_SETMYNAME	(SVR4_SIMOD|104)
#define	SVR4_SI_SETPEERNAME	(SVR4_SIMOD|105)
#define	SVR4_SI_GETINTRANSIT	(SVR4_SIMOD|106)
#define	SVR4_SI_TCL_LINK	(SVR4_SIMOD|107)
#define	SVR4_SI_TCL_UNLINK	(SVR4_SIMOD|108)


struct svr4_si_udata {
	int	tidusize;
	int	addrsize;
	int	optsize;
	int	etsdusize;
	int	servtype;
	int	so_state;
	int	so_options;
};

/*
 * The following structure is determined empirically.
 */
struct svr4_sockctl {
	long	cmd;	/* command ? */
	long	unk1;	
	long	unk2;
	long	unk3;
	long	unk4;
	u_short	family;
	u_short	port;
	u_long	addr;
	long	unk5;
	long	unk6;
};

struct svr4_sockctl1 {
	long	cmd;	/* command ? */
	long	unk1;	
	long	unk2;
	long	unk3;
	long	unk4;
	long	unk5;
	u_short	family;
	u_short	port;
	u_long	addr;
	long	unk6;
	long	unk7;
};
#define SVR4_SC_CMD_CONNECT	0x0
#define SVR4_SC_CMD_SENDTO	0x8

#endif /* !_SVR4_SOCKMOD_H_ */
