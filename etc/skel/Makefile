#	$NetBSD: Makefile,v 1.7 2004/05/16 09:53:10 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/8/93

CONFIGFILES=	dot.cshrc dot.login dot.logout dot.profile
FILESDIR=	/etc/skel
.for F in ${CONFIGFILES}
FILESNAME_${F}=	${F:S/dot//}
.endfor

.include <bsd.prog.mk>
