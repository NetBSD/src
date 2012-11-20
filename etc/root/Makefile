#	$NetBSD: Makefile,v 1.3.12.1 2012/11/20 02:57:56 tls Exp $

CONFIGFILES=		dot.cshrc dot.klogin dot.login dot.profile dot.shrc

FILESDIR=		/root
FILESMODE=		644
FILESMODE_dot.klogin=	600
.for F in ${CONFIGFILES}
FILESNAME_${F}=		${F:S/dot//}
.endfor

CONFIGLINKS=		/root/.cshrc	/.cshrc \
			/root/.profile	/.profile

.include <bsd.files.mk>
.include <bsd.links.mk>
