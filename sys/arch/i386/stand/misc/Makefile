#	$NetBSD: Makefile,v 1.1.10.2 2002/06/23 17:37:33 jdolecek Exp $

MISCFILES=	pfdisk.doc pfdisk.exe pfdisktc.zip \
		rawr32.zip rawr32src.zip \
		rawrite.c rawrite.doc rawrite.exe

CLEANFILES=	${MISCFILES:M*.exe} ${MISCFILES:M*.doc}

.include <bsd.hostprog.mk>

.SUFFIXES: .gze .exe .txt .doc

# Extract gzipped plain executables.
.gze.exe:
	gzip -dc $< >$@

# Convert Unix (LF) to DOS (CR-LF) line-ending.
.txt.doc:
	awk '{printf "%s\r\n", $$0}' $< >$@

realall: ${MISCFILES}

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
release: ${MISCFILES}
	${HOST_INSTALL_FILE} -m 0644 $> ${RELEASEDIR}/installation/misc
.endif
