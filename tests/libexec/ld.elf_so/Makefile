# $NetBSD
#

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/libexec/ld.elf_so

TESTS_C+=	t_dlerror-cleared t_dlerror-false t_dlinfo

LDADD.t_dlerror-false=	-Wl,-rpath,/var/nonexistent/lib

TESTS_SH+=		t_df_1_noopen

BINDIR=			${TESTSDIR}
PROGS+=			h_df_1_noopen1
SRCS.h_df_1_noopen1=	h_df_1_noopen.c

PROGS+=			h_df_1_noopen2
SRCS.h_df_1_noopen2=	h_df_1_noopen.c
LDADD.h_df_1_noopen2=	-lpthread

.include <bsd.test.mk>
