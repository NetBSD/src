#	$NetBSD: Makefile,v 1.3 2002/02/27 20:29:14 leo Exp $

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
