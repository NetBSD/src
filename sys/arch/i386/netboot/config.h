/* netboot
 *
 * $Log: config.h,v $
 * Revision 1.1  1993/07/08 16:03:55  brezak
 * Diskless boot prom code from Jim McKim (mckim@lerc.nasa.gov)
 *
 * Revision 1.1.1.1  1993/05/28  11:41:07  mckim
 * Initial version.
 *
 *
 * source in this file came from
 */

#if !defined(__config_h_)
#define __config_h_

/* $Id: config.h,v 1.1 1993/07/08 16:03:55 brezak Exp $
   configuration items shared between .c and .s files
 */

#define WORK_AREA_SIZE 0x10000L

#define T(x) printf(" " #x ":\n")

/* turn a near address into an integer
   representing a linear physical addr */
#define LA(x) ((u_long)(x))

#endif
