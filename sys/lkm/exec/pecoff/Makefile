#	$NetBSD: Makefile,v 1.2.2.3 2002/04/17 00:06:22 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/pecoff

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DCOMPAT_PECOFF

MKMAN=	no

KMOD=	exec_pecoff

SRCS=	lkminit_exec.c
SRCS+=	pecoff_exec.c

# compat_util.c symbol redifinitions
CPPFLAGS+=	-Demul_find=pecoff_emul_find				\
		-Dstackgap_init=pecoff_stackgap_init			\
		-Demul_flags_translate=pecoff_emul_flags_translate	\
		-Dstackgap_alloc=pecoff_stackgap_alloc		 	\
		-Dcompat_offseterr=pecoff_compat_offseterr
.PATH: $S/compat/common
SRCS+=	compat_util.c

.include <bsd.kmod.mk>
