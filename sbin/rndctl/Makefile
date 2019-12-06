#	$NetBSD: Makefile,v 1.4 2019/12/06 14:43:18 riastradh Exp $

PROG=	rndctl
MAN=	rndctl.8

COPTS.rndctl.c+=	${GCC_NO_STRINGOP_TRUNCATION}

SRCS+=	rndctl.c

# Hack: libc does not export public SHA-3 symbols, so we'll just copy
# them here statically.
.PATH:	${NETBSDSRCDIR}/common/lib/libc/hash/sha3

# Hack for namespace.h in sha3.c.
CPPFLAGS+=	-I${.CURDIR}

SRCS+=	sha3.c
SRCS+=	keccak.c

.include <bsd.prog.mk>
