#	$NetBSD: Makefile,v 1.2 2010/02/04 21:01:16 roy Exp $

CONFIGFILES=		dot.cshrc dot.klogin dot.login dot.profile dot.shrc
CONFIGFILES+=		dot.terminfo.db dot.terminfo
CLEANFILES=		dot.terminfo.db

FILESDIR=		/root
FILESMODE=		644
FILESMODE_dot.klogin=	600
.for F in ${CONFIGFILES}
FILESNAME_${F}=		${F:S/dot//}
.endfor

CONFIGLINKS=		/root/.cshrc	/.cshrc \
			/root/.profile	/.profile

realall: dot.terminfo.db

dot.terminfo.db: dot.terminfo
	${_MKTARGET_CREATE}
	${TOOL_TIC} -x -o "${.OBJDIR}/dot.terminfo" "${.CURDIR}/dot.terminfo" 


.include <bsd.prog.mk>
