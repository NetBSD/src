#	$NetBSD: Makefile,v 1.4.2.1 2005/06/10 14:47:36 tron Exp $

PROG=	veriexecctl
MAN=	veriexec.4 veriexecctl.8
SRCS=	veriexecctl_parse.y veriexecctl_conf.l veriexecctl.c

YHEADER=	1
CPPFLAGS+=	-I.
DPADD+=		${LIBL}
LDADD+=		-ll

.include <bsd.prog.mk>
