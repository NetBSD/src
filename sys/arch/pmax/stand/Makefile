#	$NetBSD: Makefile,v 1.16.12.1 2002/02/28 04:11:23 nathanw Exp $
#	@(#)Makefile	8.3 (Berkeley) 2/16/94

# first stage bootblocks
SUBDIR+=bootxx_cd9660 bootxx_ffs bootxx_lfs netboot

# remove this when netboot is working - we don't go in here
# by default...
#SUBDIR+=smallnet

# second stage bootblocks
SUBDIR+=boot

# program for installing first stage bootblocks
SUBDIR+=installboot

.include <bsd.subdir.mk>
