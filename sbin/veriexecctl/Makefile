#	$NetBSD: Makefile,v 1.2 2003/04/26 12:46:35 jdolecek Exp $

PROG=	veriexecctl
MAN=	veriexecctl.8
SRCS=	veriexecctl_parse.y veriexecctl_conf.l veriexecctl.c

YHEADER=	1
CPPFLAGS+=	-I.
DPADD+=		${LIBL}
LDADD+=		-ll

.include <bsd.prog.mk>
