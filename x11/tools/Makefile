#	$NetBSD: Makefile,v 1.7 2004/02/09 00:47:16 lukem Exp $

# These are used by the 'includes' phase,
# and can't assume that ${DESTDIR}${X11INCDIR} has been populated.
#
SUBDIR=	pswrap makestrs .WAIT

# The rest of the tools can rely upon 'includes'
#
SUBDIR+=bdftopcf fc-cache mkfontdir ucs2any mkg3states
SUBDIR+=makekeys gen_matypes .WAIT xkbcomp

.include <bsd.subdir.mk>
