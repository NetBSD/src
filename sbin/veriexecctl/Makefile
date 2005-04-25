#	$NetBSD: Makefile,v 1.8 2005/04/25 00:40:31 blymn Exp $

WARNS=3
PROG=	veriexecctl
MAN=	veriexecctl.8
SRCS=	veriexecctl_parse.y veriexecctl_conf.l veriexecctl.c

YHEADER=	1
CPPFLAGS+=	-I${.CURDIR}
DPADD+=		${LIBL}
LDADD+=		-ll

.include <bsd.prog.mk>
