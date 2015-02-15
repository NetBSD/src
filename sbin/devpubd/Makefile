# $NetBSD: Makefile,v 1.5 2015/02/15 21:46:49 christos Exp $

PROG=		devpubd
SRCS=		devpubd.c
MAN=		devpubd.8
WARNS=		6

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
