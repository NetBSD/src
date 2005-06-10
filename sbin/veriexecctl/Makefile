#	$NetBSD: Makefile,v 1.4.2.2 2005/06/10 14:51:51 tron Exp $

PROG=	veriexecctl
MAN=	veriexec.4 veriexecctl.8
SRCS=	veriexecctl_parse.y veriexecctl_conf.l veriexecctl.c

YHEADER=	1
CPPFLAGS+=	-I${.CURDIR}
DPADD+=		${LIBL}
LDADD+=		-ll

.include <bsd.prog.mk>
