# $NetBSD: Makefile,v 1.3 2013/01/11 23:49:23 mlelstv Exp $

PROG=		devpubd
SRCS=		devpubd.c
MAN=		devpubd.8
WARNS=		4

BINDIR?=	/sbin

CPPFLAGS+=	-DDEVPUBD_RUN_HOOKS=\"/libexec/devpubd-run-hooks\"

SCRIPTS=	devpubd-run-hooks
SCRIPTSDIR_devpubd-run-hooks=	/libexec

.PATH: ${.CURDIR}/hooks
HOOKS=		01-makedev 02-wedgenames
SCRIPTS+=	${HOOKS:C,^,hooks/,}
.for f in ${HOOKS}
SCRIPTSDIR_hooks/${f}=	/libexec/devpubd-hooks
.endfor

LDADD+=	-lprop
DPADD+=	${LIBPROP}

CLEANFILES=	devpubd-run-hooks

.for f in devpubd-run-hooks
${f}: ${f}.in
	${TOOL_SED} -e 's,@HOOKSDIR@,/libexec/devpubd-hooks,g' \
	    ${.CURDIR}/${f}.in > $@
.endfor

.include <bsd.prog.mk>
