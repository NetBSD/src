#	Id: Solaris.m4,v 8.4 1999/05/27 22:03:29 peterh Exp
#	$NetBSD: Solaris.m4,v 1.4 2003/06/01 14:06:53 atatat Exp $
depend: ${BEFORE} ${LINKS}
	@mv Makefile Makefile.old
	@sed -e '/^# Do not edit or remove this line or anything below it.$$/,$$d' < Makefile.old > Makefile
	@echo "# Do not edit or remove this line or anything below it." >> Makefile
	${CC} -xM ${COPTS} ${SRCS} >> Makefile

#	End of RCSfile: Solaris.m4,v
