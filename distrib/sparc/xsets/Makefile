#	$NetBSD: Makefile,v 1.1 1996/08/22 03:32:21 mrg Exp $
#
# build X11 binary sets.

all: bin include man

bin: xbin.tar.gz

xbin.tar.gz:
	tar -C / -cf - usr/X11R6/bin | gzip --best > xbin.tar.gz

include: xinc.tar.gz

xinc.tar.gz:
	tar -C / -cf - usr/X11R6/include | gzip --best > xinc.tar.gz

man: xman.tar.gz

xman.tar.gz:
	tar -C / -cf - usr/X11R6/man | gzip --best > xman.tar.gz

clean:
	rm -f x???.tar.gz
