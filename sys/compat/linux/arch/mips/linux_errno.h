/* $NetBSD: linux_errno.h,v 1.1 2001/08/26 15:16:42 manu Exp $ */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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

#ifndef _MIPS_LINUX_ERRNO_H
#define _MIPS_LINUX_ERRNO_H

/*
 * Linux/mips returns negative errors to userland  
 * The libc makes the errno positive.
 */
#define LINUX_SCERR_SIGN -

/* Use common/linux_errno.h for #1-35 */

/* Linux/mips switches EDEADLK and ENOMSG. */
#undef LINUX_EDEADLK
#define LINUX_EDEADLK		45		
#define LINUX_ENOMSG			35

/* 
 * From Linux's include/asm-mips/errno.h
 */
#define LINUX_EIDRM			36
#define LINUX_ECHRNG			37
#define LINUX_EL2NSYNC		38
#define LINUX_EL3HLT			39
#define LINUX_EL3RST			40
#define LINUX_ELNRNG			41
#define LINUX_EUNATCH		42
#define LINUX_ENOCSI			43
#define LINUX_EL2HLT			44
/* LINUX_EDEADLK defined above */
#define LINUX_ENOLCK			46
#define LINUX_EBADE			50
#define LINUX_EBADR			51
#define LINUX_EXFULL			52
#define LINUX_ENOANO			53
#define LINUX_EBADRQC		54
#define LINUX_EBADSLT		55
#define LINUX_EDEADLOCK		56
#define LINUX_EBFONT			59
#define LINUX_ENOSTR			60
#define LINUX_ENODATA		61
#define LINUX_ETIME			62
#define LINUX_ENOSR			63
#define LINUX_ENONET			64
#define LINUX_ENOPKG			65
#define LINUX_EREMOTE		66
#define LINUX_ENOLINK		67
#define LINUX_EADV			68
#define LINUX_ESRMNT			69
#define LINUX_ECOMM			70
#define LINUX_EPROTO			71
#define LINUX_EDOTDOT		73
#define LINUX_EMULTIHOP		74
#define LINUX_EBADMSG		77
#define LINUX_ENAMETOOLONG	78
#define LINUX_EOVERFLOW		79
#define LINUX_ENOTUNIQ		80
#define LINUX_EBADFD			81
#define LINUX_EREMCHG		82
#define LINUX_ELIBACC		83
#define LINUX_ELIBBAD		84
#define LINUX_ELIBSCN		85
#define LINUX_ELIBMAX		86
#define LINUX_ELIBEXEC		87
#define LINUX_EILSEQ			88
#define LINUX_ENOSYS			89
#define LINUX_ELOOP			90
#define LINUX_ERESTART		91
#define LINUX_ESTRPIPE		92
#define LINUX_ENOTEMPTY		93
#define LINUX_EUSERS			94
#define LINUX_ENOTSOCK		95
#define LINUX_EDESTADDRREQ	96
#define LINUX_EMSGSIZE		97
#define LINUX_EPROTOTYPE	98
#define LINUX_ENOPROTOOPT	99
#define LINUX_EPROTONOSUPPORT		120
#define LINUX_ESOCKTNOSUPPORT		121
#define LINUX_EOPNOTSUPP	122
#define LINUX_EPFNOSUPPORT	123
#define LINUX_EAFNOSUPPORT	124
#define LINUX_EADDRINUSE	125
#define LINUX_EADDRNOTAVAIL		126
#define LINUX_ENETDOWN		127
#define LINUX_ENETUNREACH	128
#define LINUX_ENETRESET		129
#define LINUX_ECONNABORTED	130
#define LINUX_ECONNRESET	131
#define LINUX_ENOBUFS		132
#define LINUX_EISCONN		133
#define LINUX_ENOTCONN		134
#define LINUX_EUCLEAN		135
#define LINUX_ENOTNAM		137
#define LINUX_ENAVAIL		138
#define LINUX_EISNAM			139
#define LINUX_EREMOTEIO		140
#define LINUX_EINIT			141
#define LINUX_EREMDEV		142
#define LINUX_ESHUTDOWN		143
#define LINUX_ETOOMANYREFS	144
#define LINUX_ETIMEDOUT		145
#define LINUX_ECONNREFUSED	146
#define LINUX_EHOSTDOWN		147
#define LINUX_EHOSTUNREACH	148
#define LINUX_EWOULDBLOCK	LINUX_EAGAIN
#define LINUX_EALREADY		149
#define LINUX_EINPROGRESS	150
#define LINUX_ESTALE			151
#define LINUX_ECANCELED		158
/* linux/include/asm-mips/errno.h states theses are Linux extensions */
#define LINUX_ENOMEDIUM		159
#define LINUX_EMEDIUMTYPE	160
#define LINUX_EDQUOT			1133
/* Biggest errno */			
#define LINUX_EMAXERRNO		1133
			
#endif /* !_MIPS_LINUX_ERRNO_H */
