# $NetBSD: Makefile,v 1.1.86.1 2020/04/13 08:05:56 martin Exp $

.include <bsd.own.mk>

FILES=		postinstall
MAN=		postinstall.8

FILESDIR=	/usr/sbin
FILESMODE=	${BINMODE}
FILESBUILD=	yes

ARCHSUBDIRS	!= ${TOOL_SED} -n '/ARCHDIR_SUBDIR/s/[[:space:]]//gp' \
	${NETBSDSRCDIR}/compat/archdirs.mk

.SUFFIXES: .in
.in:
	${_MKTARGET_CREATE}
	@rm -f ${.TARGET}
	${TOOL_SED} -e "s#@ARCHSUBDIRS@#${ARCHSUBDIRS}#" < ${.ALLSRC} > ${.TARGET}

CLEANFILES+= postinstall

.include <bsd.prog.mk>
