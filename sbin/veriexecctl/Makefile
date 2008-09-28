#	$NetBSD: Makefile,v 1.12.18.1 2008/09/28 11:17:15 mjf Exp $

PROG=	veriexecctl
MAN=	veriexecctl.8
SRCS=	veriexecctl_parse.y veriexecctl_conf.l veriexecctl.c

YHEADER=	1
CPPFLAGS+=	-I${.CURDIR} -I.
DPADD+=		${LIBL} ${LIBPROP}
LDADD+=		-ll -lprop

.include <bsd.prog.mk>
