# $NetBSD: Makefile,v 1.1.4.2 2019/06/10 22:10:12 christos Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/usr.bin/printf

TESTS_SH=	t_builtin t_command

t_builtin: t_builtin.sh printf.sh

t_command: t_command.sh printf.sh

.include <bsd.test.mk>
