#	$NetBSD: Makefile,v 1.3 2005/01/19 20:42:04 xtraeme Exp $

WARNS=	2
PROG=	veriexecctl
MAN=	veriexecctl.8
SRCS=	veriexecctl_parse.y veriexecctl_conf.l veriexecctl.c

YHEADER=	1
CPPFLAGS+=	-I.
DPADD+=		${LIBL}
LDADD+=		-ll

.include <bsd.prog.mk>
