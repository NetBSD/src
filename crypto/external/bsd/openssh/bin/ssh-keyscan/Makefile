#	$NetBSD: Makefile,v 1.3.2.1 2023/12/25 12:31:01 martin Exp $

BINDIR=	/usr/bin

PROG=	ssh-keyscan
SRCS=	ssh-keyscan.c ssh_api.c kexgexs.c
MAN=	ssh-keyscan.1

COPTS.kexgexs.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>
