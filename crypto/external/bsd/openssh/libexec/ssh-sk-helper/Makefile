#	$NetBSD: Makefile,v 1.2 2020/03/03 00:43:20 christos Exp $

BINDIR=	/usr/libexec

PROG=	ssh-sk-helper
SRCS=	ssh-sk-helper.c ssh-sk.c sk-usbhid.c
MAN=	ssh-sk-helper.8

LDADD+=-lfido2 -lcbor -lusbhid
DPADD+=${LIBFIDO2} ${LIBCBOR} ${LIBUSBHID}

.include <bsd.prog.mk>
