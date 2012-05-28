#	$NetBSD: Makefile,v 1.4 2012/05/28 00:13:19 riz Exp $

PROG=	iscsid

SRCS=	iscsid_main.c iscsid_lists.c iscsid_driverif.c \
	iscsid_discover.c iscsid_targets.c

CPPFLAGS+= -I${DESTDIR}/usr/include/dev/iscsi
CPPFLAGS+= -I${DESTDIR}/usr/include
CPPFLAGS+= -D_THREAD_SAFE

MAN=	iscsid.8

WARNS=	4

LDADD = -lpthread -lisns

INCSDIR= /usr/include

INCS= iscsid.h

.include <bsd.prog.mk>

