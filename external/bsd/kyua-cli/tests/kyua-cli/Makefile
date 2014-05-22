# $NetBSD: Makefile,v 1.1.12.2 2014/05/22 15:45:32 yamt Exp $

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/kyua-cli

TESTS_SUBDIRS=	cli engine examples integration store utils

# Build and install the 'bootstrap' tests but prevent them from being
# recursed into from the Atffile.  These tests are broken in platforms
# where /bin/sh is not bash (like NetBSD).  Kyua's Issue 24 has more
# details.
SUBDIR=	bootstrap

.include <bsd.test.mk>
