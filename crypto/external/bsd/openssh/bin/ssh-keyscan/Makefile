#	$NetBSD: Makefile,v 1.6 2023/05/09 22:25:26 christos Exp $

BINDIR=	/usr/bin

PROG=	ssh-keyscan
SRCS=	ssh-keyscan.c ssh_api.c kexgexs.c
MAN=	ssh-keyscan.1

COPTS.kexgexs.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>
