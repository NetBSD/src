#	$NetBSD: Makefile,v 1.4 2023/05/09 22:25:26 christos Exp $

BINOWN= root
BINMODE=555
BINDIR= /usr/libexec

PROG=	ssh-pkcs11-helper
MAN=	ssh-pkcs11-helper.8

COPTS.ssh-pkcs11-helper.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>
