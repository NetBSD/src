#	$NetBSD: Makefile,v 1.13 1997/10/10 19:51:04 christos Exp $
#	@(#)Makefile	8.3 (Berkeley) 3/27/94

PROG=	mount_portal
SRCS=	mount_portal.c activate.c conf.c getmntopts.c pt_conf.c \
	pt_exec.c pt_file.c pt_tcp.c
MAN=	mount_portal.8

MOUNT=	${.CURDIR}/../mount
CPPFLAGS+= -I${.CURDIR}/../../sys -I${MOUNT}
.PATH:	${MOUNT}

DPADD=	$(LIBCOMPAT)
LDADD=	-lcompat

.include <bsd.prog.mk>
