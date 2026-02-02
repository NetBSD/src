#	$NetBSD: Makefile,v 1.6.4.1 2026/02/02 18:07:58 martin Exp $

BINDIR=	/usr/bin

PROG=	ssh-keyscan
SRCS=	ssh-keyscan.c ssh_api.c
MAN=	ssh-keyscan.1

.include "../Makefile.inc"

.ifdef WITH_OPENSSL
SRCS+= kexgexs.c
.endif

COPTS.kexgexs.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>
