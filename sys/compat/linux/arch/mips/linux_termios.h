/*	$NetBSD: linux_termios.h,v 1.2 2001/09/02 07:56:11 manu Exp $ */

/*-
 * Copyright (c) 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Eric Haszlakiewicz and Emmanuel Dreyfus.
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

#ifndef _MIPS_LINUX_TERMIOS_H
#define _MIPS_LINUX_TERMIOS_H

/* 
 * From Linux's include/asm-mips/termios.h
 */
#define LINUX_NCC 8
/*
 * From Linux's include/asm-mips/termbits.h
 */
#define LINUX_NCCS 23

/* 
 * From Linux's include/asm-mips/ioctls.h
 */
#define LINUX_tIOC		('t' << 8)

#define LINUX_TCGETS		0x540d
#define LINUX_TCSETS		0x540e
#define LINUX_TCSETSW		0x540f	
#define LINUX_TCSETSF		0x5410
#define LINUX_TCGETA		0x5401
#define LINUX_TCSETA		0x5402
#define LINUX_TCSETAW		0x5403
#define LINUX_TCSETAF		0x5404
#define LINUX_TCSBRK		0x5405
#define LINUX_TCXONC		0x5406
#define LINUX_TCFLSH		0x5407
#define LINUX_TIOCEXCL		0x740d
#define LINUX_TIOCNXCL		0x740e
#define LINUX_TIOCSCTTY		0x5480
#define LINUX_TIOCSPGRP		_LINUX_IOW('t', 118, int)
#define LINUX_TIOCGPGRP		_LINUX_IOR('t', 119, int)
#define LINUX_TIOCOUTQ		0x7472
#define LINUX_TIOCSTI		0x5472
#define LINUX_TIOCSWINSZ 	_LINUX_IOW('t', 103, struct linux_winsize)
#define LINUX_TIOCGWINSZ 	_LINUX_IOR('t', 104, struct linux_winsize)
#define LINUX_TIOCMGET		0x741d
#define LINUX_TIOCMBIS		0x741b
#define LINUX_TIOCMBIC		0x741c
#define LINUX_TIOCMSET		0x741a
#define LINUX_TIOCGSOFTCAR	0x5481
#define LINUX_TIOCSSOFTCAR	0x5482
#define LINUX_FIONREAD		0x467f
#define LINUX_TIOCINQ		LINUX_FIONREAD
#define LINUX_TIOCLINUX		0x5483
#define LINUX_TIOCCONS		_IOW('t', 120, int)
#define LINUX_TIOCGSERIAL	0x5484
#define LINUX_TIOCSSERIAL	0x5485
#define LINUX_TIOCPKT		0x5470
#define LINUX_FIONBIO		0x667e
#define LINUX_TIOCNOTTY		0x5471
#define LINUX_TIOCSETD		(LINUX_tIOC | 1)
#define LINUX_TIOCGETD		(LINUX_tIOC | 0)
#define LINUX_TCSBRKP		0x5425
#define LINUX_TIOCTTYGSTRUCT	0x5426

#define LINUX_FIONCLEX		0x6602 /* Linux sources: "need to be adjusted" */
#define LINUX_FIOCLEX		0x6601
#define LINUX_FIOASYNC		0x667d
#define LINUX_TIOCSERCONFIG	0x5488
#define LINUX_TIOCSERGWILD	0x5489
#define LINUX_TIOCSERSWILD	0x548a
#define LINUX_TIOCGLCKTRMIOS	0x548b
#define LINUX_TIOCSLCKTRMIOS	0x548c
#define LINUX_TIOCSERGSTRUCT	0x548d
#define LINUX_TIOCSERGETLSR	0x548e
#define LINUX_TIOCSERGETMULTI	0x548f
#define LINUX_TIOCSERSETMULTI	0x5490

/* 
 * linux_termios c_cc chars: 
 * From Linux's include/asm-mips/termbits.h 
 */
#define LINUX_VINTR 	0
#define LINUX_VQUIT 	1
#define LINUX_VERASE	2
#define LINUX_VKILL 	3
#define LINUX_VMIN 	4
#define LINUX_VTIME 	5
#define LINUX_VEOL2 	6
#define LINUX_VSWTC 	7
#define LINUX_VSWTCH	LINUX_VSWTC
#define LINUX_VSTART	8
#define LINUX_VSTOP 	9
#define LINUX_VSUSP	10
/* #define LINUX_VDSUSP	11 is not defined in Linux */
#define LINUX_VREPRINT	12
#define LINUX_VDISCARD	13
#define LINUX_VWERASE	14
#define LINUX_VLNEXT	15    
#define LINUX_VEOF	16
#define LINUX_VEOL	17

/* 
 * Old style linux_termio 
 * XXX Not found anywhere in Linux 
 */
#define	LINUX_OLD_VINTR		LINUX_VINTR
#define	LINUX_OLD_VQUIT		LINUX_VQUIT
#define	LINUX_OLD_VERASE	LINUX_VERASE
#define	LINUX_OLD_VKILL		LINUX_VKILL
#define	LINUX_OLD_VEOF		LINUX_VEOF
#define	LINUX_OLD_VMIN		LINUX_VMIN
#define	LINUX_OLD_VEOL		LINUX_VEOL
#define	LINUX_OLD_VTIME		LINUX_VTIME
#define	LINUX_OLD_VEOL2		LINUX_VEOL2
#define	LINUX_OLD_VSWTC		LINUX_VSWTC

/* 
 * All the remaining stuff is from  Linux's include/asm-mips/termbits.h 
 * Note that most of theses values are octal (one leading 0), not hex...
 * Linux c_iflag masks 
 */
#define LINUX_IGNBRK	0000001
#define LINUX_BRKINT	0000002	
#define LINUX_IGNPAR	0000004
#define LINUX_PARMRK	0000010
#define LINUX_INPCK	0000020
#define LINUX_ISTRIP	0000040
#define LINUX_INLCR	0000100
#define LINUX_IGNCR	0000200
#define LINUX_ICRNL	0000400
#define LINUX_IXON	0002000
#define LINUX_IXOFF	0010000
#define LINUX_IXANY	0004000
#define LINUX_IUCLC	0001000
#define LINUX_IMAXBEL	0020000

/* 
 * Linux c_oflag masks 
 */
#define LINUX_OPOST	0020000
#define LINUX_ONLCR	0000004
#define LINUX_OLCUC	0000002
#define LINUX_OCRNL	0000010
#define LINUX_ONOCR	0000020
#define LINUX_ONLRET	0000040
#define LINUX_OFILL	0000040
#define LINUX_OFDEL	0000200
#define LINUX_NLDLY	0000400
#define LINUX_NL0	0000000
#define LINUX_NL1	0000400
#define LINUX_TABDLY	0014000
#define LINUX_TAB0	0000000
#define LINUX_TAB1	0004000
#define LINUX_TAB2	0010000
#define LINUX_TAB3	0014000
#define LINUX_CRDLY	0003000
#define LINUX_CR0	0000000
#define LINUX_CR1	0001000
#define LINUX_CR2	0002000
#define LINUX_CR3	0003000
#define LINUX_FFDLY	0100000
#define LINUX_FF0	0000000
#define LINUX_FF1	0100000
#define LINUX_BSDLY	0020000
#define LINUX_BS0	0000000
#define LINUX_BS1	0020000
#define LINUX_VTDLY	0040000
#define LINUX_VT0	0000000
#define LINUX_VT1	0040000
/* Hmm.. Linux/i386 considers this part of TABDLY.. */
#define LINUX_XTABS  0014000

/* 
 * Linux c_cflag bit masks 
 * XXX not found in Linux, but it is needed to build
 */
#define LINUX_NSPEEDS   16 

#define LINUX_CBAUD	0010017
#define LINUX_B0	0000000     /* hang up */
#define LINUX_B50  	0000001
#define LINUX_B75   	0000002
#define LINUX_B110  	0000003	
#define LINUX_B134  	0000004
#define LINUX_B150  	0000005
#define LINUX_B200  	0000006
#define LINUX_B300  	0000007
#define LINUX_B600  	0000010
#define LINUX_B1200 	0000011
#define LINUX_B1800 	0000012
#define LINUX_B2400 	0000013
#define LINUX_B4800 	0000014
#define LINUX_B9600 	0000015
#define LINUX_B19200	0000016
#define LINUX_B38400	0000017
#define LINUX_EXTA 	LINUX_B19200
#define LINUX_EXTB 	LINUX_B38400
#define LINUX_CBAUDEX	0010000
#define LINUX_B57600 	0010001
#define LINUX_B115200	0010002
#define LINUX_B230400	0010003
#define LINUX_B460800	0010004
#define LINUX_B500000	0010005
#define LINUX_B576000	0010006
#define LINUX_B921600	0010007
#define LINUX_B1000000	0010010
#define LINUX_B1152000	0010011
#define LINUX_B1500000	0010011
#define LINUX_B2000000	0010013
#define LINUX_B2500000	0010014
#define LINUX_B3000000	0010015
#define LINUX_B3500000	0010016
#define LINUX_B4000000	0010017

#define LINUX_CSIZE	0000060
#define LINUX_CS5	0000000
#define LINUX_CS6	0000020
#define LINUX_CS7 	0000040
#define LINUX_CS8	0000060
   
#define LINUX_CSTOPB 	0000100
#define LINUX_CREAD  	0000200
#define LINUX_PARENB 	0000400
#define LINUX_PARODD 	0001000
#define LINUX_HUPCL  	0002000
   
#define LINUX_CLOCAL 	0004000
#define LINUX_CRTSCTS	020000000000    /* flow control */

/* 
 * Linux c_lflag masks 
 */
#define LINUX_ISIG   	0000001
#define LINUX_ICANON 	0000002
#define LINUX_XCASE  	0000004
#define LINUX_ECHO   	0000010
#define LINUX_ECHOE  	0000020
#define LINUX_ECHOK  	0000040
#define LINUX_ECHONL 	0000100
#define LINUX_NOFLSH 	0000200
#define LINUX_TOSTOP 	0100000
#define LINUX_ECHOCTL	0001000 
#define LINUX_ECHOPRT	0002000
#define LINUX_ECHOKE 	0004000
#define LINUX_FLUSHO 	0020000
#define LINUX_PENDIN 	0040000
#define LINUX_IEXTEN 	0000400

#endif /* !_MIPS_LINUX_TERMIOS_H */
