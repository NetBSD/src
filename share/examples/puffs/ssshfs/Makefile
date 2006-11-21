#	$NetBSD: Makefile,v 1.1 2006/11/21 00:54:06 pooka Exp $

NOMAN=	1

.include <bsd.own.mk>

SSHDIST= ${NETBSDSRCDIR}/crypto/dist/ssh
.PATH: ${SSHDIST}

PROG=	ssshfs
SRCS=	ssshfs.c sftp-getput.c sftp-connect.c sftp-client.c		\
	sftp-common.c sftp-glob.c misc.c
LDADD+= -lpuffs -lutil -lssh -lcrypto -lcrypt -lz
CPPFLAGS+= -I${SSHDIST}
DBG=-g -O0

WARNS=	0 # XXX: openssh code

.include <bsd.prog.mk>
