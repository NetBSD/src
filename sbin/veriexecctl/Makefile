#	$NetBSD: Makefile,v 1.4 2005/01/20 16:39:23 xtraeme Exp $

PROG=	veriexecctl
MAN=	veriexecctl.8
SRCS=	veriexecctl_parse.y veriexecctl_conf.l veriexecctl.c

YHEADER=	1
CPPFLAGS+=	-I.
DPADD+=		${LIBL}
LDADD+=		-ll

.include <bsd.prog.mk>
