#	$NetBSD: Makefile,v 1.1.1.1.10.1 2009/05/13 18:53:30 jym Exp $

.include <bsd.own.mk>

# These are used by the 'includes' phase,
# and can't assume that ${DESTDIR}${X11INCDIR} has been populated.

SUBDIR+= makestrs .WAIT

# The rest of the tools can rely upon 'includes'

# XXX pswrap/gen_matypes -- missing

SUBDIR+=bdftopcf fc-cache ucs2any
SUBDIR+=makekeys mkfontscale .WAIT xkbcomp

.include <bsd.subdir.mk>
