#	$NetBSD: Makefile,v 1.2 2001/12/12 01:50:15 tv Exp $

PROG=	testsyscall
NOMAN=	# defined

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
	modstat -n syscall_example
	@echo
	@./testsyscall

unload:
	@echo "This test program will cause an error '3' if the call"
	@echo "has been successfully unloaded by building 'unload' in"
	@echo "the 'module' subdirectory."
	@echo
	modstat -n syscall_example

.include <bsd.prog.mk>
