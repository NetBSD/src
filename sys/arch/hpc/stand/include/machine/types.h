/*	$NetBSD: types.h,v 1.3 2001/04/30 13:41:33 uch Exp $	*/

/* Windows CE architecture */

#ifndef	_MACHTYPES_H_
#define	_MACHTYPES_H_

#include <sys/cdefs.h>
#include <machine/int_types.h>

/* BSD types. */
typedef	unsigned char		u_char;
typedef	unsigned short		u_short;
typedef	unsigned int		u_int;
typedef	unsigned long		u_long;

typedef unsigned char		u_int8_t;
typedef unsigned short		u_int16_t;
typedef unsigned int		u_int32_t;
typedef unsigned __int64	u_int64_t;

typedef signed char		int8_t;	
typedef signed short		int16_t;
typedef signed int		int32_t;
typedef signed __int64		int64_t;

typedef u_int32_t		off_t;
#define off_t			u_int32_t
#ifndef _TIME_T_DEFINED
#if _WIN32_WCE < 210
typedef long			time_t;
#else
typedef unsigned long		time_t;
#endif
#define _TIME_T_DEFINED
#endif

typedef unsigned int		size_t;

/* Windows CE virtual address */
typedef u_int32_t		vaddr_t;
typedef u_int32_t		vsize_t;
/* Physical address */
typedef u_int32_t		paddr_t;
typedef u_int32_t		psize_t;

/* kernel virtual address */
typedef u_int32_t		kaddr_t;
typedef u_int32_t		ksize_t;

#endif /* _MACHTYPES_H_ */
