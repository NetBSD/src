#	@(#)Makefile.inc	5.3 (Berkeley) 2/20/91

# compat-43 sources
.PATH: ${.CURDIR}/${MACHINE}/yp ${.CURDIR}/yp

SRCS+=	xdryp.c xdryppasswd.c yplib.c

