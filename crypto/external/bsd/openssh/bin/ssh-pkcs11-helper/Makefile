#	$NetBSD: Makefile,v 1.1.50.1 2023/12/25 12:31:01 martin Exp $

BINOWN= root
BINMODE=555
BINDIR= /usr/libexec

PROG=	ssh-pkcs11-helper
MAN=	ssh-pkcs11-helper.8

COPTS.ssh-pkcs11-helper.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>
