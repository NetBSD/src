#	$NetBSD: Makefile,v 1.2.4.1 2002/03/16 15:56:54 jdolecek Exp $

SUBDIR= libtos loadbsd aptck chg_pid file2swp rawwrite

.PHONY: all clean install

all:
	@for dir in ${SUBDIR}; do	\
	    ${MAKE} -C $$dir all;		\
	 done

clean:
	@for dir in ${SUBDIR}; do	\
	    ${MAKE} -C $$dir clean;	\
	 done

install:
	@for dir in ${SUBDIR}; do	\
	    ${MAKE} -C $$dir install;	\
	 done
