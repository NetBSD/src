#	$NetBSD: Makefile,v 1.9 2005/05/06 21:51:21 jmc Exp $

WARNS=3
PROG=	veriexecctl
MAN=	veriexecctl.8
SRCS=	veriexecctl_parse.y veriexecctl_conf.l veriexecctl.c

YHEADER=	1
CPPFLAGS+=	-I${.CURDIR} -I.
DPADD+=		${LIBL}
LDADD+=		-ll

.include <bsd.prog.mk>
