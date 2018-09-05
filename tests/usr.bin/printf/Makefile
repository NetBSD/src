# $NetBSD: Makefile,v 1.1 2018/09/05 21:05:40 kre Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/usr.bin/printf

TESTS_SH=	t_builtin t_command

t_builtin: t_builtin.sh printf.sh

t_command: t_command.sh printf.sh

.include <bsd.test.mk>
