#	Id: CC-M.m4,v 8.5 1999/05/27 22:03:28 peterh Exp
#	$NetBSD: CC-M.m4,v 1.4 2003/06/01 14:06:53 atatat Exp $
depend: ${BEFORE} ${LINKS}
	@mv Makefile Makefile.old
	@sed -e '/^# Do not edit or remove this line or anything below it.$$/,$$d' < Makefile.old > Makefile
	@echo "# Do not edit or remove this line or anything below it." >> Makefile
	${CC} -M ${COPTS} ${SRCS} >> Makefile

#	End of RCSfile: CC-M.m4,v
