#	$NetBSD: Makefile,v 1.16 1999/11/28 03:17:11 simonb Exp $
#	@(#)Makefile	8.3 (Berkeley) 2/16/94

# first stage bootblocks
SUBDIR+=bootxx_cd9660 bootxx_ffs bootxx_lfs # netboot

# remove this when netboot is working - we don't go in here
# by default...
#SUBDIR+=smallnet

# second stage bootblocks
SUBDIR+=boot

# program for installing first stage bootblocks
SUBDIR+=installboot

.include <bsd.subdir.mk>
