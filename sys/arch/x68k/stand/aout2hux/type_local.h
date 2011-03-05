/*
 *	type and other definitions
 *
 *	written by ITOH Yasufumi
 *	public domain
 *
 *	$NetBSD: type_local.h,v 1.2.8.1 2011/03/05 15:10:07 bouyer Exp $
 */

#ifndef __STDC__
# ifndef const
#  define const
# endif
#endif

#ifndef __BIT_TYPES_DEFINED__
typedef unsigned char	u_int8_t;
typedef unsigned short	u_int16_t;
typedef unsigned int	u_int32_t;
#endif

/*
 * big-endian integer types
 */
typedef union be_uint16 {
	u_int8_t	val[2];
	u_int16_t	hostval;
} be_uint16_t;

typedef union be_uint32 {
	u_int8_t	val[4];
	u_int32_t	hostval;
	be_uint16_t	half[2];
} be_uint32_t;

#define SIZE_16		sizeof(be_uint16_t)
#define SIZE_32		sizeof(be_uint32_t)
