#	$NetBSD: Makefile,v 1.7.2.1 2017/03/20 06:56:50 pgoyette Exp $

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
