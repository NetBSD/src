.if !exists(Makefile.inc)

error:
	@echo
	@echo "                      You MUST link/copy"
	@echo
	@echo "       Makefile.inc.NetBSD    or   Makefile.inc.FreeBSD"
	@echo
	@echo "             to Makefile.inc before you can proceed !"
	@echo

all:		error
clean:		error
cleandir:	error
depend:		error
install:	error

.else
SUBDIR= keycap cursor fontedit fonts kcon loadfont scon \
	userkeys vttest ispcvt mcon demo

.include <bsd.subdir.mk>
.endif
