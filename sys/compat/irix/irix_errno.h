/*	$NetBSD: irix_errno.h,v 1.3.70.1 2008/06/02 13:22:58 mjf Exp $ */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
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

#ifndef	_IRIX_ERRNO_H_
#define	_IRIX_ERRNO_H_

extern const int native_to_irix_errno[];

/* From IRIX's <sys/errno.h> */

#define IRIX_EPERM 		1
#define IRIX_ENOENT 		2
#define IRIX_ESRCH 		3
#define IRIX_EINTR 		4
#define IRIX_EIO 		5
#define IRIX_ENXIO 		6
#define IRIX_E2BIG 		7
#define IRIX_ENOEXEC 		8
#define IRIX_EBADF 		9
#define IRIX_ECHILD 		10
#define IRIX_EAGAIN 		11
#define IRIX_ENOMEM 		12
#define IRIX_EACCES 		13
#define IRIX_EFAULT 		14
#define IRIX_ENOTBLK 		15
#define IRIX_EBUSY 		16
#define IRIX_EEXIST 		17
#define IRIX_EXDEV 		18
#define IRIX_ENODEV 		19
#define IRIX_ENOTDIR 		20
#define IRIX_EISDIR 		21
#define IRIX_EINVAL 		22
#define IRIX_ENFILE 		23
#define IRIX_EMFILE 		24
#define IRIX_ENOTTY 		25
#define IRIX_ETXTBSY 		26
#define IRIX_EFBIG 		27
#define IRIX_ENOSPC 		28
#define IRIX_ESPIPE 		29
#define IRIX_EROFS 		30
#define IRIX_EMLINK 		31
#define IRIX_EPIPE 		32
#define IRIX_EDOM 		33
#define IRIX_ERANGE 		34
#define IRIX_ENOMSG 		35
#define IRIX_EIDRM 		36
#define IRIX_ECHRNG 		37
#define IRIX_EL2NSYNC 		38
#define IRIX_EL3HLT 		39
#define IRIX_EL3RST 		40
#define IRIX_ELNRNG 		41
#define IRIX_EUNATCH 		42
#define IRIX_ENOCSI 		43
#define IRIX_EL2HLT 		44
#define IRIX_EDEADLK 		45
#define IRIX_ENOLCK 		46
#define IRIX_ECKPT 		47
#define IRIX_EBADE 		50
#define IRIX_EBADR 		51
#define IRIX_EXFULL 		52
#define IRIX_ENOANO 		53
#define IRIX_EBADRQC 		54
#define IRIX_EBADSLT 		55
#define IRIX_EDEADLOCK 		56
#define IRIX_EBFONT 		57
#define IRIX_ENOSTR 		60
#define IRIX_ENODATA 		61
#define IRIX_ETIME 		62
#define IRIX_ENOSR 		63
#define IRIX_ENONET 		64
#define IRIX_ENOPKG 		65
#define IRIX_EREMOTE 		66
#define IRIX_ENOLINK 		67
#define IRIX_EADV 		68
#define IRIX_ESRMNT 		69
#define IRIX_ECOMM 		70
#define IRIX_EPROTO 		71
#define IRIX_EMULTIHOP 		74
#define IRIX_EBADMSG 		77
#define IRIX_ENAMETOOLONG 	78
#define IRIX_EOVERFLOW 		79
#define IRIX_ENOTUNIQ 		80
#define IRIX_EBADFD 		81
#define IRIX_EREMCHG 		82
#define IRIX_ELIBACC 		83
#define IRIX_ELIBBAD 		84
#define IRIX_ELIBSCN 		85
#define IRIX_ELIBMAX 		86
#define IRIX_ELIBEXEC 		87
#define IRIX_EILSEQ 		88
#define IRIX_ENOSYS 		89
#define IRIX_ELOOP 		90
#define IRIX_ERESTART 		91
#define IRIX_ESTRPIPE 		92
#define IRIX_ENOTEMPTY 		93
#define IRIX_EUSERS 		94
#define IRIX_ENOTSOCK 		95
#define IRIX_EDESTADDRREQ 	96
#define IRIX_EMSGSIZE 		97
#define IRIX_EPROTOTYPE 	98
#define IRIX_ENOPROTOOPT 	99
#define IRIX_EPROTONOSUPPORT 	120
#define IRIX_ESOCKTNOSUPPORT 	121
#define IRIX_EOPNOTSUPP 	122
#define IRIX_EPFNOSUPPORT 	123
#define IRIX_EAFNOSUPPORT 	124
#define IRIX_EADDRINUSE 	125
#define IRIX_EADDRNOTAVAIL 	126
#define IRIX_ENETDOWN 		127
#define IRIX_ENETUNREACH 	128
#define IRIX_ENETRESET 		129
#define IRIX_ECONNABORTED 	130
#define IRIX_ECONNRESET 	131
#define IRIX_ENOBUFS 		132
#define IRIX_EISCONN 		133
#define IRIX_ENOTCONN 		134
#define IRIX_ESHUTDOWN 		143
#define IRIX_ETOOMANYREFS 	144
#define IRIX_ETIMEDOUT 		145
#define IRIX_ECONNREFUSED 	146
#define IRIX_EHOSTDOWN 		147
#define IRIX_EHOSTUNREACH 	148
#define IRIX_LASTERRNO 		IRIX_ENOTCONN
#define IRIX_EWOULDBLOCK 	IRIX_EAGAIN
#define IRIX_EALREADY 		149
#define IRIX_EINPROGRESS 	150
#define IRIX_ESTALE 		151
#define IRIX_EIORESID 		500
#define IRIX_EUCLEAN 		135
#define IRIX_ENOTNAM 		137
#define IRIX_ENAVAIL 		138
#define IRIX_EISNAM 		139
#define IRIX_EREMOTEIO 		140
#define IRIX_EINIT 		141
#define IRIX_EREMDEV 		142
#define IRIX_ECANCELED 		158

/*
 * The following seems to be kernel specific, it
 * is possible that we don't need them.
 */
#define IRIX_ENOLIMFILE 	1001
#define IRIX_EPROCLIM 		1002
#define IRIX_EDISJOINT 		1003
#define IRIX_ENOLOGIN 		1004
#define IRIX_ELOGINLIM 		1005
#define IRIX_EGROUPLOOP 	1006
#define IRIX_ENOATTACH 		1007
#define IRIX_ENOTSUP 		1008
#define IRIX_ENOATTR 		1009
#define IRIX_EFSCORRUPTED 	1010
#define IRIX_EDIRCORRUPTED 	1010
#define IRIX_EWRONGFS 		1011
#define IRIX_EDQUOT 		1133
#define IRIX_ENFSREMOTE 	1135
#define IRIX_ECONTROLLER 	1300
#define IRIX_ENOTCONTROLLER 	1301
#define IRIX_EENQUEUED 		1302
#define IRIX_ENOTENQUEUED 	1303
#define IRIX_EJOINED 		1304
#define IRIX_ENOTJOINED 	1305
#define IRIX_ENOPROC 		1306
#define IRIX_EMUSTRUN 		1307
#define IRIX_ENOTSTOPPED 	1308
#define IRIX_ECLOCKCPU 		1309
#define IRIX_EINVALSTATE 	1310
#define IRIX_ENOEXIST 		1311
#define IRIX_EENDOFMINOR 	1312
#define IRIX_EBUFSIZE 		1313
#define IRIX_EEMPTY 		1314
#define IRIX_ENOINTRGROUP 	1315
#define IRIX_EINVALMODE 	1316
#define IRIX_ECANTEXTENT 	1317
#define IRIX_EINVALTIME 	1318
#define IRIX_EDESTROYED 	1319
#define IRIX_EBDHDL 		1400
#define IRIX_EDELAY 		1401
#define IRIX_ENOBWD 		1402
#define IRIX_EBADRSPEC 		1403
#define IRIX_EBADTSPEC 		1404
#define IRIX_EBADFILT 		1405
#define IRIX_EMIGRATED 		1500
#define IRIX_EMIGRATING 	1501
#define IRIX_ECELLDOWN 		1502
#define IRIX_EMEMRETRY 		1600

#endif /* _IRIX_ERRNO_H_ */
