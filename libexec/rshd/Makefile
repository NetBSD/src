#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93
#	$Id: Makefile,v 1.4 1994/06/05 15:33:20 cgd Exp $

PROG=	rshd
CFLAGS+=-DKERBEROS -DCRYPT
SRCS=	rshd.c des_rw.c
MAN8=	rshd.0
DPADD=	${LIBKRB} ${LIBDES}
LDADD=	-lkrb -ldes
.PATH: ${.CURDIR}/../../usr.bin/rlogin

.include <bsd.prog.mk>
