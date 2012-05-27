#	$NetBSD: Makefile,v 1.2 2012/05/27 19:52:51 christos Exp $

PROG=	iscsid

SRCS=	iscsid_main.c iscsid_lists.c iscsid_driverif.c \
	iscsid_discover.c iscsid_targets.c

CPPFLAGS+= -I${DESTDIR}/usr/include/dev/iscsi
CPPFLAGS+= -I${DESTDIR}/usr/include
CPPFLAGS+= -D_THREAD_SAFE
CPPFLAGS+= -DISCSI_NOTHREAD
DBG=-g

MAN=	iscsid.8

WARNS=	4

# CPPFLAGS+= -DISCSI_DEBUG

LDADD = -lpthread -lisns

INCSDIR= /usr/include

INCS= iscsid.h

.include <bsd.prog.mk>

