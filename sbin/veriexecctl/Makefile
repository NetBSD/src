#	$NetBSD: Makefile,v 1.13 2008/07/21 11:38:44 gmcgarry Exp $

PROG=	veriexecctl
MAN=	veriexecctl.8
SRCS=	veriexecctl_parse.y veriexecctl_conf.l veriexecctl.c

YHEADER=	1
CPPFLAGS+=	-I${.CURDIR} -I.
DPADD+=		${LIBL} ${LIBPROP}
LDADD+=		-ll -lprop

.include <bsd.prog.mk>

.if ${HAVE_GCC} == 4 || defined(HAVE_PCC)
COPTS.veriexecctl_parse.c	+=	-Wno-pointer-sign
.endif
