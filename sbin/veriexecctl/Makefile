#	$NetBSD: Makefile,v 1.12 2006/11/28 22:22:03 elad Exp $

PROG=	veriexecctl
MAN=	veriexecctl.8
SRCS=	veriexecctl_parse.y veriexecctl_conf.l veriexecctl.c

YHEADER=	1
CPPFLAGS+=	-I${.CURDIR} -I.
DPADD+=		${LIBL} ${LIBPROP}
LDADD+=		-ll -lprop

.include <bsd.prog.mk>

.if ${HAVE_GCC} == 4
COPTS+=	-Wno-pointer-sign
.endif
