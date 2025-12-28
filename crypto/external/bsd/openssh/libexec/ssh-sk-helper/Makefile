#	$NetBSD: Makefile,v 1.5 2025/12/28 09:39:35 nia Exp $

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
