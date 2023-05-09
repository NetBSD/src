#	$NetBSD: Makefile,v 1.4 2023/05/09 22:25:26 christos Exp $

BINDIR=	/usr/libexec

PROG=	ssh-sk-helper
SRCS=	ssh-sk-helper.c ssh-sk.c sk-usbhid.c
MAN=	ssh-sk-helper.8

LDADD+=-lfido2 -lcbor -lusbhid -lm
DPADD+=${LIBFIDO2} ${LIBCBOR} ${LIBUSBHID} ${LIBM}

COPTS.ssh-sk.c+= -Wno-error=deprecated-declarations
COPTS.sk-usbhid.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>
