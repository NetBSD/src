#	$NetBSD: Makefile,v 1.6 2003/04/24 01:01:39 perry Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/8/93

NOOBJ=		# defined

.if make(distribution)
FILES=		dot.cshrc dot.login dot.logout dot.profile
FILESDIR=	/etc/skel
.for F in ${FILES}
FILESNAME_${F}=	${F:S/dot//}
.endfor

distribution: filesinstall
.endif

.include <bsd.prog.mk>
