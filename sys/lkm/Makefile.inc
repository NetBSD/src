#	$NetBSD: Makefile.inc,v 1.1 1998/03/28 23:58:44 thorpej Exp $

.if !defined(__SYS_LKM_MAKEFILE_INC)
__SYS_LKM_MAKEFILE_INC=	1

.include <bsd.own.mk>

.if defined(UVM)
CPPFLAGS+=	-DUVM
.endif
.if defined(PMAP_NEW)
CPPFLAGS+=	-DPMAP_NEW
.endif

.endif	# __SYS_LKM_MAKEFILE_INC
