/*	$NetBSD: types.h,v 1.6.4.1 2006/04/22 11:37:28 simonb Exp $	*/

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

/* 7.18.1.1 Exact-width integer types */
typedef signed char		int8_t;
typedef signed short		int16_t;
typedef signed int		int32_t;
typedef signed __int64		int64_t;

typedef unsigned char		uint8_t;
typedef unsigned short		uint16_t;
typedef unsigned int		uint32_t;
typedef unsigned __int64	uint64_t;

/* compatibility names */
typedef uint8_t			u_int8_t;
typedef uint16_t		u_int16_t;
typedef uint32_t		u_int32_t;
typedef uint64_t		u_int64_t;

typedef int32_t			off_t;
#define	off_t			int32_t
#ifndef _TIME_T_DEFINED
#if _WIN32_WCE < 210
typedef long			time_t;
#else
typedef unsigned long		time_t;
#endif
#define	_TIME_T_DEFINED
#endif

typedef unsigned int		size_t;

/* Windows CE virtual address */
typedef uint32_t		vaddr_t;
typedef uint32_t		vsize_t;
/* Physical address */
typedef uint32_t		paddr_t;
typedef uint32_t		psize_t;

/* kernel virtual address */
typedef uint32_t		kaddr_t;
typedef uint32_t		ksize_t;

#endif /* _MACHTYPES_H_ */
