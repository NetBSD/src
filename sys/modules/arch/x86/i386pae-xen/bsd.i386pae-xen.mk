#	$NetBSD: bsd.i386pae-xen.mk,v 1.1.28.2 2018/03/27 03:57:37 pgoyette Exp $

.ifndef _BSD_I386PAE_XEN_MK_
_BSD_I386PAE_XEN_MK_=1

KMODULEARCHDIR:=	i386pae-xen

XEN=	1
PAE=	1

#CPPFLAGS+=	-DPAE -DXEN

.endif # _BSD_I386PAE_XEN_MK_
