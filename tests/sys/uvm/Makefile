# $NetBSD: Makefile,v 1.1.2.2 2017/01/07 08:56:56 pgoyette Exp $
#

WARNS?=6
.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/sys/uvm
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys -I${.CURDIR}/ -D_TEST -g

# Depend on the kernel source files too
DPSRCS=		${NETBSDSRCDIR}/sys/uvm/uvm_physseg.[ch]

.PATH:		${NETBSDSRCDIR}/sys/kern
TESTS_C+=	t_uvm_physseg
SRCS.t_uvm_physseg+=	t_uvm_physseg.c subr_extent.c
CPPFLAGS.t_uvm_physseg.c= -D_EXTENT_TESTING -D__POOL_EXPOSE -DDIAGNOSTIC
CPPFLAGS.subr_extent.c=	  -D_EXTENT_TESTING -D__POOL_EXPOSE -D_KERNTYPES -DDIAGNOSTIC

TESTS_C+=       t_uvm_physseg_load
SRCS.t_uvm_physseg_load+=       t_uvm_physseg_load.c subr_extent.c
CPPFLAGS.t_uvm_physseg_load.c= -D_EXTENT_TESTING -D__POOL_EXPOSE -DDIAGNOSTIC

.include <bsd.dep.mk>
.include <bsd.test.mk>
