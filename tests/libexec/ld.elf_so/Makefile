# $NetBSD: Makefile,v 1.24 2024/07/22 02:34:58 riastradh Exp $
#

NOMAN=		# defined

.include <bsd.own.mk>

.if ${MKPIC} != "no"

SUBDIR+=	helper_dso1 helper_dso3 .WAIT helper_dso2 .WAIT \
		helper_ifunc_dso \
		helper_symver_dso0 .WAIT helper_symver_dso1 .WAIT \
		helper_symver_dso2 .WAIT \
		data

SUBDIR+=	helper_def_dynamic
SUBDIR+=	helper_def_static
SUBDIR+=	helper_onlydef
SUBDIR+=	.WAIT
SUBDIR+=	helper_abuse_dynamic
SUBDIR+=	helper_abuse_static
SUBDIR+=	helper_onlyctor_dynamic
SUBDIR+=	helper_onlyuse_dynamic
SUBDIR+=	helper_onlyuse_static
SUBDIR+=	helper_use_dynamic
SUBDIR+=	helper_use_static

TESTSDIR=	${TESTSBASE}/libexec/ld.elf_so

TESTS_C+=	t_dlerror-cleared t_dlerror-false t_dlinfo t_dlvsym t_ifunc
TESTS_C+=	t_ifunc_now
TESTS_C+=	t_ifunc_norelro
TESTS_C+=	t_ifunc_norelro_now
TESTS_C+=	t_rtld_r_debug
TESTS_C+=	t_tls_extern
TESTS_C+=	t_hash

SRCS.t_ifunc_now=		t_ifunc.c
SRCS.t_ifunc_norelro=		t_ifunc.c
SRCS.t_ifunc_norelro_now=	t_ifunc.c

LDADD.t_ifunc+=			${${MKRELRO} != "no":?-Wl,-z,relro:}
LDADD.t_ifunc_now+=		${${MKRELRO} != "no":?-Wl,-z,relro:} -Wl,-z,now
LDADD.t_ifunc_norelro+=		${${MKRELRO} != "no":?-Wl,-z,norelro:}
LDADD.t_ifunc_norelro_now+=	${${MKRELRO} != "no":?-Wl,-z,norelro:} -Wl,-z,now

.PATH:	${NETBSDSRCDIR}/libexec/ld.elf_so
SRCS.t_hash+=	t_hash.c
SRCS.t_hash+=	hash.c
CPPFLAGS.t_hash.c+=	-I${NETBSDSRCDIR}/libexec/ld.elf_so

COPTS.t_rtld_r_debug.c += ${CC_WNO_MAYBE_UNINITIALIZED}

LDADD.t_dlerror-false=	-Wl,-rpath,/var/nonexistent/lib
LDADD.t_dlvsym=		-Wl,-rpath,${TESTSDIR}/h_helper_symver_dso2
.for t in t_ifunc t_ifunc_now t_ifunc_norelro t_ifunc_norelro_now
LDADD.${t}+=		-Wl,-rpath,${TESTSDIR} -lutil
DPADD.${t}+=		${LIBUTIL}
.endfor
LDADD.t_tls_extern+=	-Wl,-rpath,${TESTSDIR}

TESTS_SH+=		t_df_1_noopen t_dl_symver t_thread_local_dtor

BINDIR=			${TESTSDIR}
PROGS+=			h_df_1_noopen1
SRCS.h_df_1_noopen1=	h_df_1_noopen.c

PROGS+=			h_df_1_noopen2
SRCS.h_df_1_noopen2=	h_df_1_noopen.c
LDADD.h_df_1_noopen2=	-lpthread

PROGS+=			h_thread_local_dtor
LDADD.h_thread_local_dtor=	-Wl,-rpath,${TESTSDIR}  -lpthread

PROGS+=			h_ifunc
SRCS.h_ifunc=		h_ifunc.c
IFUNCDIR!=		cd ${.CURDIR}/helper_ifunc_dso && ${PRINTOBJDIR}
LDADD.h_ifunc=		-Wl,-rpath,${TESTSDIR} -L${IFUNCDIR} -lh_helper_ifunc_dso

PROGS+=			h_locking
SRCS.h_locking=		h_locking.c
LDADD.h_locking=	-lpthread -Wl,--export-dynamic -Wl,-rpath,${TESTSDIR}

PROGS+=			h_dl_symver_v0
SRCS.h_dl_symver_v0=	h_dl_symver.c
V0ODIR!=		cd ${.CURDIR}/helper_symver_dso0 && ${PRINTOBJDIR}
LDADD.h_dl_symver_v0=	-L${V0ODIR} -lh_helper_symver_dso

PROGS+=			h_dl_symver_v1
SRCS.h_dl_symver_v1=	h_dl_symver.c
V1ODIR!=		cd ${.CURDIR}/helper_symver_dso1 && ${PRINTOBJDIR}
LDADD.h_dl_symver_v1=	-L${V1ODIR} -lh_helper_symver_dso

PROGS+=			h_dl_symver_v2
SRCS.h_dl_symver_v2=	h_dl_symver.c
V2ODIR!=		cd ${.CURDIR}/helper_symver_dso2 && ${PRINTOBJDIR}
LDADD.h_dl_symver_v2=	-L${V2ODIR} -lh_helper_symver_dso

.include <bsd.test.mk>

.else

NOPROG=	# defined
.include <bsd.prog.mk>

.endif
