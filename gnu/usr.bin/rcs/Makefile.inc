#	$Id: Makefile.inc,v 1.5 1995/10/22 22:01:43 christos Exp $

LIBRCS!=cd $(.CURDIR)/../lib; \
	printf "xxx:\n\techo \$${.OBJDIR}/librcs.a\n" | \
	${MAKE} -r -s -f - xxx | grep librcs
