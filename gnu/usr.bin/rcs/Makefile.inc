#	$NetBSD: Makefile.inc,v 1.1.1.2 1996/10/13 21:56:39 veego Exp $

LIBRCS!=cd $(.CURDIR)/../lib; \
	printf "xxx:\n\techo \$${.OBJDIR}/librcs.a\n" | \
	${MAKE} -r -s -f - xxx | grep librcs
