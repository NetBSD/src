#	$NetBSD: Makefile,v 1.8.8.1 2023/08/11 13:40:10 martin Exp $

BINDIR=	/usr/bin

PROG=	ssh-keygen
SRCS=	ssh-keygen.c moduli.c sshsig.c

COPTS.ssh-keygen.c+=	-Wno-pointer-sign

COPTS.ssh-keygen.c+=	${GCC_NO_FORMAT_TRUNCATION}
COPTS.ssh-keygen.c+= -Wno-error=deprecated-declarations
COPTS.moduli.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>
