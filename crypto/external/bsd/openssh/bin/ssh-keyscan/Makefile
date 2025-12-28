#	$NetBSD: Makefile,v 1.7 2025/12/28 09:39:35 nia Exp $

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
