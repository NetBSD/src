#	$NetBSD: Makefile,v 1.2 2001/01/08 14:59:32 itojun Exp $

# RCSid:
#	Id: Makefile,v 1.33 1998/11/11 11:53:53 sjg Exp
#
#	@(#) Copyright (c) 1994 Simon J. Gerraty
#
#	This file is provided in the hope that it will
#	be of use.  There is absolutely NO WARRANTY.
#	Permission to copy, redistribute or otherwise
#	use this file is hereby granted provided that 
#	the above copyright notice and this notice are
#	left intact. 
#      
#	Please send copies of changes and bug-fixes to:
#	sjg@quick.com.au
#

.include <bsd.own.mk>

# XXX There's a bit of work to do before we can enable warnings.
WARNS=0

LIB=	crypto_idea
CPPFLAGS+= -Dlib${LIB} -I. -I${OPENSSLSRC}/crypto

CRYPTODIST=	${.CURDIR}/../../crypto/dist

.include "../../crypto/Makefile.openssl"
.PATH: ${OPENSSLSRC}

.PATH:	${OPENSSLSRC}/crypto/idea
CPPFLAGS+=	-I${OPENSSLSRC}/crypto/idea
SRCS+=i_cbc.c i_skey.c

# XXX
.if ${OBJECT_FMT} == "ELF"
AFLAGS+=-DELF
.else
AFLAGS+=-DOUT
.endif

MKPIC=no
#MKPROFILE=no
#MKLINT=no

.include <bsd.lib.mk>
