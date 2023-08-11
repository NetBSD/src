#	$NetBSD: Makefile,v 1.5.8.1 2023/08/11 13:40:10 martin Exp $

BINDIR=	/usr/bin

PROG=	ssh-keyscan
SRCS=	ssh-keyscan.c ssh_api.c kexgexs.c
MAN=	ssh-keyscan.1

COPTS.kexgexs.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>
