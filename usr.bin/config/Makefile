#	$NetBSD: Makefile,v 1.12 2020/02/07 20:17:48 fox Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/19/94

.include <bsd.own.mk>
WARNS=6

PROG=	config
SRCS=	files.c gram.y hash.c lint.c main.c mkdevsw.c mkheaders.c mkioconf.c \
	mkmakefile.c mkswap.c pack.c scan.l sem.c util.c

.PATH:	${NETBSDSRCDIR}/usr.bin/cksum
SRCS+=  crc.c

MAN=	config.1
MAN+=	config.5 config.samples.5
YHEADER=1
CPPFLAGS+=-I${.CURDIR} -I.
CPPFLAGS+= -I${NETBSDSRCDIR}/usr.bin/cksum

COPTS.scan.c+=		${GCC_NO_STRINGOP_TRUNCATION}

.ifndef HOSTPROG
LDADD+=-lutil
DPADD+=${LIBUTIL}
COPTS.mkheaders.c+=	${GCC_NO_FORMAT_TRUNCATION}
COPTS.sem.c+=		${GCC_NO_FORMAT_TRUNCATION}
.endif

CWARNFLAGS+=-Wno-format-y2k

.include <bsd.prog.mk>
