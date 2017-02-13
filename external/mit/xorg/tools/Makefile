#	$NetBSD: Makefile,v 1.8 2017/02/13 07:37:19 snj Exp $

.include <bsd.own.mk>

# These are used by the 'includes' phase,
# and can't assume that ${DESTDIR}${X11INCDIR} has been populated.

#SUBDIR+= makestrs .WAIT

# The rest of the tools can rely upon 'includes'

# makekeys is now done src/tools
SUBDIR+=bdftopcf fc-cache ucs2any
SUBDIR+=mkfontscale

.include <bsd.buildinstall.mk>
.include <bsd.subdir.mk>
