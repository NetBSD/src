/*
 * Copyright (c) 1991 Regents of the University of California.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)proc.h	7.1 (Berkeley) 5/15/91
 */

/*
 * Machine-dependent part of the proc structure for amiga.
 */
struct mdproc {
	short	md_flags;		/* machine-dependent flags */
	short	md_emul;		/* emulating a different OS, see 
					   defines below */
#ifdef notyet
	int	*p_regs;		/* registers on current frame */
#endif
};

/* md_flags */
#define	MDP_AST		0x0001	/* async trap pending */
#define MDP_STACKADJ	0x0002	/* frame SP adjusted, might have to
				   undo when system call returns
				   ERESTART. */

/* currently defined (not necessarily supported yet...) OS-emulators.
   These are *NOT* flags, they're values (you can't have a simulatanous
   SunOS and ADOS process, for example..) */
#define MDPE_NETBSD	0x0000	/* default */
#define MDPE_SUNOS	0x0001	/* SunOS 4.x for sun3 */
#define MDPE_SVR40	0x0002	/* Amiga Unix System V R4.0 */
#define MDPE_HPUX	0x0003	/* if someone *really* wants to... */
#define MDPE_ADOS	0x0004	/* AmigaDOS process */
#define MDPE_LINUX	0x0005	/* lets see who can first run the others
				   binaries :-)) */
