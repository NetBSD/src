/*	$NetBSD: dp8390.h,v 1.3 1998/05/06 13:32:18 drochner Exp $	*/

extern int dp8390_config __P((void));
extern void dp8390_stop __P((void));

extern int dp8390_iobase;
extern int dp8390_membase;
extern int dp8390_memsize;
#ifdef SUPPORT_WD80X3
#ifdef SUPPORT_SMC_ULTRA
extern int dp8390_is790;
#else
#define dp8390_is790 0
#endif
#else
#ifdef SUPPORT_SMC_ULTRA
#define dp8390_is790 1
#endif
#endif
extern u_int8_t dp8390_cr_proto; /* values always set in CR */
extern u_int8_t dp8390_dcr_reg; /* override DCR if LS is set */
