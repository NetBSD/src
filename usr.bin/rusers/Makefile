#	$Id: Makefile,v 1.5 1993/11/21 19:01:01 brezak Exp $
.PATH:  ${.CURDIR}/../../libexec/rpc.rusersd

SRCS = rusers.c rusersxdr.c
PROG = rusers
MAN1 = rusers.0

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

.include <bsd.prog.mk>
