/*	$Id: netbsd_sparc.c,v 1.1 1994/01/28 12:38:30 pk Exp $ */

#define TARGET_IS_BIG_ENDIAN_P
#define HOST_BIG_ENDIAN_P
#define	BYTES_IN_WORD	4
#define	ARCH	32

#define	PAGE_SIZE	4096
#define	SEGMENT_SIZE	PAGE_SIZE
#define __LDPGSZ	8192
#define MID_SPARC	138
#define	DEFAULT_ARCH	bfd_arch_sparc

#define MACHTYPE_OK(mtype) ((mtype) == MID_SPARC || (mtype) == M_UNKNOWN)

#define MY(OP) CAT(netbsd_sparc_,OP)
/* This needs to start with a.out so GDB knows it is an a.out variant.  */
#define TARGETNAME "a.out-netbsd-sparc"

#include "netbsd.h"

