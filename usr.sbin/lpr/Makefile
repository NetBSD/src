#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$Id: Makefile,v 1.4 1994/06/25 05:58:08 cgd Exp $

SUBDIR=	lpc lpd lpq lpr lprm lptest pac filters

.if !make(all) && !make(depend) && !make(lint) && !make(tags)
SUBDIR+= SMM.doc
.endif

.include <bsd.subdir.mk>
