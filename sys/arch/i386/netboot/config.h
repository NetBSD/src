/* netboot
 *
 * source in this file came from
 */

#if !defined(__config_h_)
#define __config_h_

/* $Id: config.h,v 1.2 1994/02/15 15:10:21 mycroft Exp $
   configuration items shared between .c and .s files
 */

#define WORK_AREA_SIZE 0x10000L

#define T(x) printf(" " #x ":\n")

/* turn a near address into an integer
   representing a linear physical addr */
#define LA(x) ((u_long)(x))

#endif
