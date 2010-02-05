#	$NetBSD: Makefile,v 1.3 2010/02/05 09:44:23 roy Exp $

CONFIGFILES=		dot.cshrc dot.klogin dot.login dot.profile dot.shrc

FILESDIR=		/root
FILESMODE=		644
FILESMODE_dot.klogin=	600
.for F in ${CONFIGFILES}
FILESNAME_${F}=		${F:S/dot//}
.endfor

CONFIGLINKS=		/root/.cshrc	/.cshrc \
			/root/.profile	/.profile

.include <bsd.prog.mk>
