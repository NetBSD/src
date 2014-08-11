#	$NetBSD: bsd.i386pae-xen.mk,v 1.1 2014/08/11 03:43:26 jnemeth Exp $

.ifndef _BSD_I386PAE_XEN_MK_
_BSD_I386PAE_XEN_MK_=1

KMODULEARCHDIR:=	i386pae-xen

XEN=	1
PAE=	1

.endif # _BSD_I386PAE_XEN_MK_
