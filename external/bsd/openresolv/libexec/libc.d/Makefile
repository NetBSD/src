# $NetBSD: Makefile,v 1.1 2020/07/22 19:00:20 roy Exp $
#

.include <bsd.own.mk>
.include "../../Makefile.inc"

FILES=		avahi-daemon mdnsd
FILESDIR=	/libexec/resolvconf/libc.d
# We should not have to do this ...
.for f in ${FILES}
FILESBUILD_$f=	yes
.endfor

CLEANFILES=	${FILES}

.for f in ${FILES}
${f}:	Makefile ${f}.in
	${TOOL_SED} ${RESOLVCONF_SED} ${DIST}/${f}.in >$@
.endfor

.include <bsd.prog.mk>
