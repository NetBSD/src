#	$NetBSD: Makefile,v 1.5 2022/08/30 13:14:48 riastradh Exp $

CONFIGFILES=		dot.cshrc dot.klogin dot.login dot.profile dot.shrc

FILESDIR=		/root
FILESMODE=		644
FILESMODE_dot.klogin=	600
.for F in ${CONFIGFILES}
FILESNAME_${F}=		${F:S/dot//}
.endfor

CONFIGLINKS=		/root/.cshrc	/.cshrc \
			/root/.profile	/.profile

LINKSMODE=		${FILESMODE}

.include <bsd.files.mk>
.include <bsd.links.mk>
