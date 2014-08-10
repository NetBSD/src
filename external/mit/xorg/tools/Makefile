#	$NetBSD: Makefile,v 1.3.22.1 2014/08/10 06:50:58 tls Exp $

.include <bsd.own.mk>

# These are used by the 'includes' phase,
# and can't assume that ${DESTDIR}${X11INCDIR} has been populated.

SUBDIR+= makestrs .WAIT

# The rest of the tools can rely upon 'includes'

# XXX pswrap/gen_matypes -- missing

SUBDIR+=bdftopcf fc-cache ucs2any
SUBDIR+=makekeys mkfontscale .WAIT xkbcomp

.include <bsd.subdir.mk>
