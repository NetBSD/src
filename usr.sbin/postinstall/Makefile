# $NetBSD: Makefile,v 1.2 2019/06/14 01:06:33 christos Exp $

.include <bsd.own.mk>

FILES=		postinstall
MAN=		postinstall.8

FILESDIR=	/usr/sbin
FILESMODE=	${BINMODE}
FILESBUILD=	yes

ARCHSUBDIRS	!= ${MAKE} -f ${NETBSDSRCDIR}/compat/archdirs.mk \
	ACTIVE_CC=clang -V ARCHDIR_SUBDIR | ${TOOL_SED} -e 's@[a-z0-9_]*/@@g'

.SUFFIXES: .in
.in:
	${_MKTARGET_CREATE}
	@rm -f ${.TARGET}
	${TOOL_SED} -e "s#@ARCHSUBDIRS@#${ARCHSUBDIRS}#" < ${.ALLSRC} > ${.TARGET}

CLEANFILES+= postinstall

.include <bsd.prog.mk>
