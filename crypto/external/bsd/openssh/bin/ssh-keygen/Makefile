#	$NetBSD: Makefile,v 1.11 2025/10/15 16:52:59 christos Exp $

BINDIR=	/usr/bin

PROG=	ssh-keygen
SRCS=	ssh-keygen.c moduli.c sshsig.c

COPTS.ssh-keygen.c+=	-Wno-pointer-sign

COPTS.ssh-keygen.c+=	${CC_WNO_FORMAT_TRUNCATION}
COPTS.ssh-keygen.c+= -Wno-error=deprecated-declarations
COPTS.moduli.c+= -Wno-error=deprecated-declarations
COPTS.ssh-keygen.c+= ${${ACTIVE_CC} == "clang":? -Wno-error=invalid-noreturn :}

.include <bsd.prog.mk>
