/*	$NetBSD: ansi.h,v 1.1 2001/02/09 18:35:21 uch Exp $	*/

/* Windows CE architecture */

#include <machine/int_types.h>

#define	_BSD_CLOCK_T_		unsigned int	/* clock() */
#define	_BSD_PTRDIFF_T_		int		/* ptr1 - ptr2 */
#define	_BSD_SSIZE_T_		int		/* byte count or error */
#define	_BSD_VA_LIST_		char *		/* va_list */
#define	_BSD_CLOCKID_T_		int		/* clockid_t */
#define	_BSD_TIMER_T_		int		/* timer_t */
#define	_BSD_SUSECONDS_T_	int		/* suseconds_t */
#define	_BSD_USECONDS_T_	unsigned int	/* useconds_t */

