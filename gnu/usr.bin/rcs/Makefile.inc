#	$Id: Makefile.inc,v 1.3 1993/12/28 11:31:15 davidb Exp $

LIBRCS!=cd $(.CURDIR)/../lib; \
	printf "xxx:\n\techo \$${.OBJDIR}/librcs.a\n" | make -r -s -f - xxx
