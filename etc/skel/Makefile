#	$NetBSD: Makefile,v 1.5 2002/12/02 13:33:14 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/8/93

NOOBJ=		# defined

.if make(distribution)
FILES=		dot.cshrc dot.login dot.profile
FILESDIR=	/etc/skel
.for F in ${FILES}
FILESNAME_${F}=	${F:S/dot//}
.endfor

distribution: filesinstall
.endif

.include <bsd.prog.mk>
