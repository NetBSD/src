#	$NetBSD: Makefile,v 1.2 2001/12/12 00:07:47 tv Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/8/93

.if make(distribution)
FILES=		dot.cshrc dot.login dot.mailrc dot.profile dot.rhosts
FILESDIR=	/etc/skel
.for F in ${FILES}
FILESNAME_${F}=	${F:S/dot//}
.endfor

distribution: filesinstall
.endif

NOOBJ=		# defined

.include <bsd.prog.mk>
