#	$NetBSD: Makefile,v 1.1 2011/10/23 21:11:23 agc Exp $

PROG=	iscsid

SRCS=	iscsid_main.c iscsid_lists.c iscsid_driverif.c \
	iscsid_discover.c iscsid_targets.c

CPPFLAGS+= -I${DESTDIR}/usr/include/dev/iscsi
CPPFLAGS+= -I${DESTDIR}/usr/include
CPPFLAGS+= -D_THREAD_SAFE

MAN=	iscsid.8

WARNS=	4

# CPPFLAGS+= -DISCSI_DEBUG

LDADD = -lpthread -lisns

INCSDIR= /usr/include

INCS= iscsid.h

.include <bsd.prog.mk>

