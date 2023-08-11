#	$NetBSD: Makefile,v 1.3.8.1 2023/08/11 13:40:11 martin Exp $

BINOWN= root
BINMODE=555
BINDIR= /usr/libexec

PROG=	ssh-pkcs11-helper
MAN=	ssh-pkcs11-helper.8

COPTS.ssh-pkcs11-helper.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>
