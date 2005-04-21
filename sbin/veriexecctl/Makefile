#	$NetBSD: Makefile,v 1.7 2005/04/21 12:45:12 christos Exp $

WARNS=3
PROG=	veriexecctl
MAN=	veriexec.4 veriexecctl.8
SRCS=	veriexecctl_parse.y veriexecctl_conf.l veriexecctl.c

YHEADER=	1
CPPFLAGS+=	-I${.CURDIR}
DPADD+=		${LIBL}
LDADD+=		-ll

.include <bsd.prog.mk>
