#	$NetBSD: Makefile,v 1.1 2000/12/11 13:48:11 jdolecek Exp $

PROG=	testmisc
MKMAN=	no

load:
	@echo "This test program will call the sample system call;"
	@echo "the "offset requested will be shown as 'Off' in the"
	@echo "status display below.  If no modules are loaded, an"
	@echo "error '3' will be reported."
	@echo
	@echo "The sample system call will display a message on the"
	@echo "system console each time it is run."
	@echo
	@echo
	modstat -n misc_example
	@echo
	@./testmisc

unload:
	@echo "This test program will cause an error '3' if the call"
	@echo "has been successfully unloaded by building 'unload' in"
	@echo "the 'module' subdirectory."
	@echo
	modstat -n misc_example

.include <bsd.prog.mk>
