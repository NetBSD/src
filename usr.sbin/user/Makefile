# $NetBSD: Makefile,v 1.1 1999/12/06 21:31:42 agc Exp $
#
CPPFLAGS+= -DEXTENSIONS

SKEL_DIR=	/etc/skel
EXAMPLE_DIR=	${DESTDIR}/usr/share/examples/usermgmt

INSTALL_DATA?= ${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m 0444

PROG= user
LINKS=	${BINDIR}/user ${BINDIR}/useradd
LINKS=	${BINDIR}/user ${BINDIR}/userdel
LINKS=	${BINDIR}/user ${BINDIR}/usermod
LINKS=	${BINDIR}/user ${BINDIR}/group
LINKS=	${BINDIR}/user ${BINDIR}/groupadd
LINKS=	${BINDIR}/user ${BINDIR}/groupdel
LINKS=	${BINDIR}/user ${BINDIR}/groupmod
LDADD+= -lutil 
DPADD+= -lutil
MAN= user.8 useradd.8 userdel.8 usermod.8 group.8 groupadd.8 groupdel.8 groupmod.8

.include <bsd.prog.mk>

install: ${PROG}
	-mkdir -p ${EXAMPLE_DIR}
	${INSTALL_DATA} dot.cshrc ${EXAMPLE_DIR}/.cshrc
	${INSTALL_DATA} dot.login ${EXAMPLE_DIR}/.login
	${INSTALL_DATA} dot.profile ${EXAMPLE_DIR}/.profile
	@echo "**********************************************************************"
	@echo "Any files matching ${SKEL_DIR}/.[A-z]* will be copied"
	@echo "to a new user's home directory."
	@echo "Example dot files can be found in ${EXAMPLE_DIR}."
	@echo "**********************************************************************"

test: ${PROG}
	@echo "No news is good news"
	@echo "1. Adding new user"
	./${PROG} add -m -g=uid test1.1
	@echo "2. Modifying new user"
	./${PROG} mod -l test1.2 test1.1
	@echo "3. Deleting new user"
	./${PROG} del -r test1.2
	@echo "4. Attempting to add an invalid user name - IGNORE ANY ERROR"
	-./${PROG} add -m test1%1
	@echo "5. Bad usage - IGNORE ANY ERROR"
	-./${PROG} add -m
	@echo "All tests completed"
