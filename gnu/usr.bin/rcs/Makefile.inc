#	$Id: Makefile.inc,v 1.4 1994/01/24 18:39:52 cgd Exp $

LIBRCS!=cd $(.CURDIR)/../lib; \
	printf "xxx:\n\techo \$${.OBJDIR}/librcs.a\n" | ${MAKE} -r -s -f - xxx
