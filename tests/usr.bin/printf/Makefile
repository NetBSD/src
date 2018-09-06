# $NetBSD: Makefile,v 1.1.2.2 2018/09/06 06:56:49 pgoyette Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/usr.bin/printf

TESTS_SH=	t_builtin t_command

t_builtin: t_builtin.sh printf.sh

t_command: t_command.sh printf.sh

.include <bsd.test.mk>
