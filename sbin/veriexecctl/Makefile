#	$NetBSD: Makefile,v 1.11 2006/05/11 23:16:29 mrg Exp $

PROG=	veriexecctl
MAN=	veriexecctl.8
SRCS=	veriexecctl_parse.y veriexecctl_conf.l veriexecctl.c

YHEADER=	1
CPPFLAGS+=	-I${.CURDIR} -I.
DPADD+=		${LIBL}
LDADD+=		-ll

.include <bsd.prog.mk>

.if ${HAVE_GCC} == 4
COPTS+=	-Wno-pointer-sign
.endif
