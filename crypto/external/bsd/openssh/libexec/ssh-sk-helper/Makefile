#	$NetBSD: Makefile,v 1.1 2020/02/27 00:24:41 christos Exp $

BINDIR=	/usr/libexec

PROG=	ssh-sk-helper
SRCS=	ssh-sk-helper.c ssh-sk.c sk-usbhid.c
MAN=	ssh-sk-helper.8

LDADD+=-lfido2 -lusbhid
DPADD+=${LIBFIDO2} ${LIBUSBHID}

.include <bsd.prog.mk>
