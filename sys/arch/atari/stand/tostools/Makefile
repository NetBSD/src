#	$NetBSD: Makefile,v 1.2 2000/08/30 23:51:54 jhawk Exp $

SUBDIR= libtos loadbsd aptck chg_pid rawwrite

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
