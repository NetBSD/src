#	$NetBSD: Makefile,v 1.9 2008/09/01 10:24:15 rtr Exp $

# These are used by the 'includes' phase,
# and can't assume that ${DESTDIR}${X11INCDIR} has been populated.
#
SUBDIR=	pswrap makestrs .WAIT

# The rest of the tools can rely upon 'includes'
#
SUBDIR+=bdftopcf fc-cache ucs2any mkg3states rgb
SUBDIR+=makekeys mkfontscale gen_matypes .WAIT xkbcomp

.include <bsd.subdir.mk>
