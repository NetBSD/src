#	$NetBSD: Makefile,v 1.4.2.3 2005/06/10 14:53:16 tron Exp $

WARNS=3
PROG=	veriexecctl
MAN=	veriexec.4 veriexecctl.8
SRCS=	veriexecctl_parse.y veriexecctl_conf.l veriexecctl.c

YHEADER=	1
CPPFLAGS+=	-I${.CURDIR}
DPADD+=		${LIBL}
LDADD+=		-ll

.include <bsd.prog.mk>
