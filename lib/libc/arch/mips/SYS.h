/*	$NetBSD: SYS.h,v 1.12.6.2 2002/06/21 18:18:00 nathanw Exp $ */

/*-
 * Copyright (c) 1996 Jonathan STone
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

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *	from: @(#)SYS.h	8.1 (Berkeley) 6/4/93
 */

#include <sys/syscall.h>
#include <mips/asm.h>


/*
 * If compiling for shared libs, Emit sysV ABI PIC segment pseudo-ops.
 *
 * i)  Emit .abicalls before .LEAF entrypoint, and .cpload/.cprestore after.
 * ii) Do interprocedure jumps indirectly via t9, with the side-effect of
 *     preserving the callee's entry address in t9.
 */
#ifdef ABICALLS
	.abicalls
# define PIC_PROLOGUE(x,sr)	.set noreorder; .cpload sr; .set reorder
# define PIC_CALL(l,sr)		la sr, _C_LABEL(l); jr sr
#else
# define PIC_PROLOGUE(x,sr)
# define PIC_CALL(l,sr)		j  _C_LABEL(l)
#endif


#ifdef __STDC__
# define SYSTRAP(x)	li v0,SYS_ ## x; syscall;
#else
# define SYSTRAP(x)	li v0,SYS_/**/x; syscall;
#endif


/*
 * Do a syscall that cannot fail (sync, get{p,u,g,eu,eg)id)
 */
#define RSYSCALL_NOERROR(x)						\
	PSEUDO_NOERROR(x,x)

/*
 * Do a normal syscall.
 */
#define RSYSCALL(x)							\
	PSEUDO(x,x)

/*
 * Do a syscall that has an internal name and a weak external alias.
 */
#define	WSYSCALL(weak,strong)						\
	WEAK_ALIAS(weak,strong);					\
	PSEUDO(strong,weak)

/*
 * Do a renamed or pseudo syscall (e.g., _exit()), where the entrypoint
 * and syscall name are not the same.
 */
#define PSEUDO_NOERROR(x,y)						\
LEAF(x);								\
	SYSTRAP(y);							\
	j ra;								\
	END(x)

#define PSEUDO(x,y)							\
LEAF(x);								\
	PIC_PROLOGUE(x,t9);						\
	SYSTRAP(y);							\
	bne a3,zero,err;						\
	j ra;								\
err:									\
	PIC_CALL(__cerror,t9);						\
	END(x)
