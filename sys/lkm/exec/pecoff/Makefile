#	$NetBSD: Makefile,v 1.2.2.5 2002/12/11 06:46:29 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/pecoff

CPPFLAGS+=	-nostdinc -D_KERNEL -DCOMPAT_PECOFF

MKMAN=	no

KMOD=	exec_pecoff

SRCS=	lkminit_exec.c
SRCS+=	pecoff_exec.c

# compat_util.c symbol redifinitions
CPPFLAGS+=	-Demul_find=pecoff_emul_find				\
		-Demul_find_interp=pecoff_emul_find_interp		\
		-Dstackgap_init=pecoff_stackgap_init			\
		-Demul_flags_translate=pecoff_emul_flags_translate	\
		-Dstackgap_alloc=pecoff_stackgap_alloc		 	\
		-Dcompat_offseterr=pecoff_compat_offseterr
.PATH: $S/compat/common
SRCS+=	compat_util.c

.include <bsd.kmod.mk>
