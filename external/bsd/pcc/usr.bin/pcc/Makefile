#	$NetBSD: Makefile,v 1.10 2014/07/24 20:55:45 plunky Exp $

WARNS?=	2

.include <bsd.init.mk>

.PATH: ${PCC_DIST}/cc/cc ${PCC_DIST}/cc/driver

#
# we build PCC cc(1) as pcc(1) to avoid conflicts with GCC
#

PROG=	pcc
SRCS=	cc.c strlist.o xalloc.o
MAN=	pcc.1

CPPFLAGS+=	-DTARGOS=\"${TARGOS}\"
CPPFLAGS+=	-DTARGMACH=\"${TARGMACH}\"
CPPFLAGS+=	-DLIBEXECDIR=\"/usr/libexec/\"
CPPFLAGS+=	-DINCLUDEDIR=\"/usr/include/\"
CPPFLAGS+=	-I${PCC_DIST}/cc/driver
CPPFLAGS+=	-I${PCC_DIST}/mip
CPPFLAGS+=	-I${PCC_DIST}/os/${TARGOS}
CPPFLAGS+=	-I${PCC_DIST}/arch/${TARGMACH}

pcc.1:	cc.1
	${TOOL_SED} -e "s,Nm cc,Nm pcc,"	\
		    -e "s,Dt CC,Dt PCC,"	\
		    -e "s,Xr cpp,Xr pcpp,"	\
		    ${.ALLSRC} > ${.TARGET}

.if ${MKCXX} != "no"
SYMLINKS+=	${BINDIR}/pcc /usr/bin/p++
.endif

CLEANFILES+=	pcc.1

.include <bsd.prog.mk>
