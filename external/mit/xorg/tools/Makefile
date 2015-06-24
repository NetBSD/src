#	$NetBSD: Makefile,v 1.6 2015/06/24 22:20:25 matt Exp $

.include <bsd.own.mk>

# These are used by the 'includes' phase,
# and can't assume that ${DESTDIR}${X11INCDIR} has been populated.

#SUBDIR+= makestrs .WAIT

# The rest of the tools can rely upon 'includes'

# XXX pswrap/gen_matypes -- missing

# makekeys is now done src/tools
SUBDIR+=bdftopcf fc-cache ucs2any
SUBDIR+=mkfontscale .WAIT xkbcomp

.include <bsd.buildinstall.mk>
.include <bsd.subdir.mk>
