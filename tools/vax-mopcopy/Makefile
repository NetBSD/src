#	$NetBSD: Makefile,v 1.1 2024/10/23 00:45:59 kalvisd Exp $

.include <bsd.hostinit.mk>

HOSTPROGNAME=	${_TOOL_PREFIX}vax-mopcopy
HOST_SRCDIR=	usr.sbin/mopd/mopcopy

LIBMOPCOPYOBJ!= cd ${.CURDIR}/../libmopcopy && ${PRINTOBJDIR}

LDADD+=		-L${LIBMOPCOPYOBJ} -lmopcopy

# VAX binaries are now ELF
HOST_CPPFLAGS+=	-DNOAOUT

.include "${.CURDIR}/../Makefile.nbincludes"
.include "${.CURDIR}/../Makefile.host"
