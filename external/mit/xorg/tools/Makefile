#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:54 wrstuden Exp $

.include <bsd.own.mk>

# These are used by the 'includes' phase,
# and can't assume that ${DESTDIR}${X11INCDIR} has been populated.

SUBDIR+= makestrs .WAIT

# The rest of the tools can rely upon 'includes'

# XXX pswrap/gen_matypes -- missing

SUBDIR+=bdftopcf fc-cache ucs2any mkg3states
SUBDIR+=makekeys mkfontscale .WAIT xkbcomp

.include <bsd.subdir.mk>
