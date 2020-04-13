#	$NetBSD: Makefile,v 1.3.2.2 2020/04/13 07:45:21 martin Exp $

BINDIR=	/usr/libexec

PROG=	ssh-sk-helper
SRCS=	ssh-sk-helper.c ssh-sk.c sk-usbhid.c
MAN=	ssh-sk-helper.8

LDADD+=-lfido2 -lcbor -lusbhid -lm
DPADD+=${LIBFIDO2} ${LIBCBOR} ${LIBUSBHID} ${LIBM}

.include <bsd.prog.mk>
