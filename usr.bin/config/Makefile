#	$NetBSD: Makefile,v 1.8 2007/05/13 20:22:45 veego Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/19/94

.include <bsd.own.mk>

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

.ifndef HOSTPROG
LDADD+=-lutil
DPADD+=${LIBUTIL}
.endif

CWARNFLAGS+=-Wno-format-y2k

.include <bsd.prog.mk>
