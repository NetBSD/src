# $NetBSD
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/libexec/ld.elf_so

TESTS_C+=	t_dlerror-cleared t_dlerror-false t_dlinfo

LDADD.t_dlerror-false=	-Wl,-rpath,/var/nonexistent/lib

.include <bsd.test.mk>
