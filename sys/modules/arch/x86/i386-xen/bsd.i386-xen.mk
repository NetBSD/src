#	$NetBSD: bsd.i386-xen.mk,v 1.1.28.1 2018/03/26 10:49:45 pgoyette Exp $

.ifndef _BSD_I386_XEN_MK_
_BSD_I386_XEN_MK_=1

KMODULEARCHDIR:=	i386-xen

XEN=	1

CPPFLAGS+=	-DXEN

.endif # _BSD_I386_XEN_MK_
