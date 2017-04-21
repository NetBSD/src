#	$NetBSD: Makefile,v 1.7.4.1 2017/04/21 16:53:00 bouyer Exp $

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
