#	$NetBSD: Makefile,v 1.2 1994/10/27 04:19:36 cgd Exp $

.if !exists(${.CURDIR}/../Makefile.inc)
error:
	@echo
	@echo "                      You MUST link/copy"
	@echo
	@echo "       ../Makefile.inc.NetBSD    or   ../Makefile.inc.FreeBSD"
	@echo
	@echo "             to ../Makefile.inc before you can proceed !"
	@echo
.else

PROG	=	cursor

.include <bsd.prog.mk>

$(PROG):	$(INCDIR)/pcvt_ioctl.h

.endif
