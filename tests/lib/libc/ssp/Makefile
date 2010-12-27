# $NetBSD: Makefile,v 1.1 2010/12/27 02:04:19 pgoyette Exp $

NOMAN=		# defined

.include <bsd.own.mk>

WARNS=	4

CPPFLAGS+=	-D_FORTIFY_SOURCE=2
CFLAGS+=	-fstack-protector-all -Wstack-protector
LDFLAGS+=	-fstack-protector-all -Wstack-protector

# Bootstrap hack

.ifmake !clean && !obj && !cleandir
.BEGIN:
	${AR} cr libssp_nonshared.a
.endif

CLEANFILES+= 	libssp_nonshared.a
LDFLAGS+=	-L.

TESTSDIR=	${TESTSBASE}/lib/libc/ssp

TESTS_SH+=	t_ssp

BINDIR=		${TESTSDIR}

PROGS=		h_fgets
PROGS+=		h_gets
PROGS+=		h_getcwd
PROGS+=		h_memcpy
PROGS+=		h_memmove
PROGS+=		h_memset
PROGS+=		h_raw
PROGS+=		h_read
PROGS+=		h_readlink
PROGS+=		h_snprintf
PROGS+=		h_sprintf
PROGS+=		h_strcat
PROGS+=		h_strcpy
PROGS+=		h_strncat
PROGS+=		h_strncpy
PROGS+=		h_vsnprintf
PROGS+=		h_vsprintf

.include <bsd.test.mk>
