#	$NetBSD: Makefile,v 1.14 2008/07/24 06:13:21 dholland Exp $

PROG=	veriexecctl
MAN=	veriexecctl.8
SRCS=	veriexecctl_parse.y veriexecctl_conf.l veriexecctl.c

YHEADER=	1
CPPFLAGS+=	-I${.CURDIR} -I.
DPADD+=		${LIBL} ${LIBPROP}
LDADD+=		-ll -lprop

.include <bsd.prog.mk>
