#	$NetBSD: Makefile,v 1.3 2002/08/06 13:45:30 soren Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/8/93

.if make(distribution)
FILES=		dot.cshrc dot.login dot.mailrc dot.profile
FILESDIR=	/etc/skel
.for F in ${FILES}
FILESNAME_${F}=	${F:S/dot//}
.endfor

distribution: filesinstall
.endif

NOOBJ=		# defined

.include <bsd.prog.mk>
