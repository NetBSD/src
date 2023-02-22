#	$NetBSD: Makefile,v 1.4.24.1 2023/02/22 19:32:23 martin Exp $

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
