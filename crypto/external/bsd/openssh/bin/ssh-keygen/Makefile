#	$NetBSD: Makefile,v 1.4.36.1 2023/12/25 12:31:01 martin Exp $

BINDIR=	/usr/bin

PROG=	ssh-keygen
SRCS=	ssh-keygen.c moduli.c sshsig.c

COPTS.ssh-keygen.c+=	-Wno-pointer-sign

COPTS.ssh-keygen.c+=	${CC_WNO_FORMAT_TRUNCATION}
COPTS.ssh-keygen.c+= -Wno-error=deprecated-declarations
COPTS.moduli.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>
