#	$NetBSD: Makefile,v 1.1 2002/11/23 10:52:49 blymn Exp $

PROG=	veriexecctl
MAN=	veriexecctl.8
SRCS=	veriexecctl_parse.y veriexecctl_conf.l veriexecctl.c

YHEADER=	1
CPPFLAGS+=	-I.
DPADD+=		${LIBL}
LDADD+=		-ll

LDSTATIC=	-static

.include <bsd.prog.mk>
