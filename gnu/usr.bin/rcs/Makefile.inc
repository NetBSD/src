#	$NetBSD: Makefile.inc,v 1.6 1996/10/15 06:59:51 veego Exp $

LIBRCS!=cd $(.CURDIR)/../lib; \
	printf "xxx:\n\techo \$${.OBJDIR}/librcs.a\n" | \
	${MAKE} -r -s -f - xxx | grep librcs
