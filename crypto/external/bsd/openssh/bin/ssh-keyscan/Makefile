#	$NetBSD: Makefile,v 1.6.2.1 2026/11/04 22:36:36 perseant Exp $

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
