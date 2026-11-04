#	$NetBSD: Makefile,v 1.4.2.1 2026/11/04 22:36:48 perseant Exp $

BINDIR=	/usr/libexec

PROG=	ssh-sk-helper
SRCS=	ssh-sk-helper.c ssh-sk.c sk-usbhid.c
MAN=	ssh-sk-helper.8

.include "../Makefile.inc"

# Even if OpenSSH is built without libcrypto, it's still needed by fido2.

LDADD+=-lfido2 -lcbor -lusbhid -lcrypto -lm
DPADD+=${LIBFIDO2} ${LIBCBOR} ${LIBUSBHID} ${LIBCRYPTO} ${LIBM}

COPTS.ssh-sk.c+= -Wno-error=deprecated-declarations
COPTS.sk-usbhid.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>
