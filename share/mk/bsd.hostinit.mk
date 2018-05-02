# $NetBSD: bsd.hostinit.mk,v 1.1.2.2 2018/05/02 07:20:02 pgoyette Exp $

.if !defined(_BSD_HOSTINIT_MK_)
_BSD_HOSTINIT_MK_=1

NOINFO=		# defined
NOLINT=		# defined
NOMAN=		# defined
MKREPRO=no	# Native toolchain might be unable to do it

.include <bsd.init.mk>

.endif  # !defined(_BSD_HOSTINIT_MK_)
