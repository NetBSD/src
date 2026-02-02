#	$NetBSD: Makefile,v 1.10.4.1 2026/02/02 18:07:58 martin Exp $

BINDIR=	/usr/bin

PROG=	ssh-keygen
SRCS=	ssh-keygen.c sshsig.c

.include "../Makefile.inc"

.ifdef WITH_OPENSSL
SRCS+=	moduli.c
.endif

COPTS.ssh-keygen.c+=	-Wno-pointer-sign

COPTS.ssh-keygen.c+=	${CC_WNO_FORMAT_TRUNCATION}
COPTS.ssh-keygen.c+= -Wno-error=deprecated-declarations
COPTS.moduli.c+= -Wno-error=deprecated-declarations
COPTS.ssh-keygen.c+= ${${ACTIVE_CC} == "clang":? -Wno-error=invalid-noreturn :}

.include <bsd.prog.mk>
