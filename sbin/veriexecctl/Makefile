#	$NetBSD: Makefile,v 1.5 2005/04/20 13:44:45 blymn Exp $

PROG=	veriexecctl
MAN=	veriexec.4 veriexecctl.8
SRCS=	veriexecctl_parse.y veriexecctl_conf.l veriexecctl.c

YHEADER=	1
CPPFLAGS+=	-I.
DPADD+=		${LIBL}
LDADD+=		-ll

.include <bsd.prog.mk>
