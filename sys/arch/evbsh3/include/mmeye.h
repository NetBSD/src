/* 
 * Brains mmEye specific register definition
 */

#ifndef _MMEYE_H_
#define _MMEYE_H_

#ifdef _KERNEL
#include "opt_led_addr.h"

#define MMEYE_LED       (*(volatile unsigned char *)LED_ADDR)

#if 0
#if 1
#define MMEYE_LED       (*(volatile unsigned char *)0xa8000000)
#else
#define MMEYE_LED       (*(volatile unsigned char *)0xb8000000)
#endif
#endif

#endif /* _KERNEL */

#endif /* _MMEYE_H_ */
