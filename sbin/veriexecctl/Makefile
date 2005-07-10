#	$NetBSD: Makefile,v 1.4.2.5 2005/07/10 15:33:32 tron Exp $

WARNS=3
PROG=	veriexecctl
MAN=	veriexecctl.8
SRCS=	veriexecctl_parse.y veriexecctl_conf.l veriexecctl.c

YHEADER=	1
CPPFLAGS+=	-I${.CURDIR} -I.
DPADD+=		${LIBL}
LDADD+=		-ll

.include <bsd.prog.mk>
