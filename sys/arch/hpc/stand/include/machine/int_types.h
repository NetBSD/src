/*	$NetBSD: int_types.h,v 1.1 2001/02/09 18:35:28 uch Exp $	*/

/* Windows CE architecture */

/* BSD types. */
typedef	unsigned char		u_char;
typedef	unsigned short		u_short;
typedef	unsigned int		u_int;
typedef	unsigned long		u_long;

/*
 * 7.18.1 Integer types
 */

/* 7.18.1.1 Exact-width integer types */
typedef	signed char		 __int8_t;
typedef	unsigned char		__uint8_t;
typedef	short int		__int16_t;
typedef	unsigned short int     __uint16_t;
typedef	int			__int32_t;
typedef	unsigned int	       __uint32_t;
typedef	signed __int64		__int64_t;
typedef	unsigned __int64	__uint64_t;

/* 7.18.1.4 Integer types capable of holding object pointers */

typedef	int		       __intptr_t;
typedef	unsigned int	      __uintptr_t;
