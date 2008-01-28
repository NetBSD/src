/*	types.h,v 1.7.12.1 2007/11/06 23:22:11 matt Exp	*/

#ifndef _SHARK_TYPES_H_
#define	_SHARK_TYPES_H_

#include <arm/arm32/types.h>

/* We need to invoke FIQs indirectly. */
#define	__ARM_FIQ_INDIRECT

#define	__HAVE_DEVICE_REGISTER
#define	__HAVE_TIMECOUNTER
#define	__HAVE_GENERIC_TODR
#define	__HAVE_FAST_SOFTINTS

#endif /* _SHARK_TYPES_H_ */
