/* netboot
 *
 * $Log: assert.h,v $
 * Revision 1.1  1993/07/08 16:03:51  brezak
 * Diskless boot prom code from Jim McKim (mckim@lerc.nasa.gov)
 *
 * Revision 1.1.1.1  1993/05/28  11:41:07  mckim
 * Initial version.
 *
 *
 * source in this file came from
 * the Mach ethernet boot written by Leendert van Doorn.
 *
 * assert.h
 */

#ifndef	NDEBUG
#define	assert(exp) \
	if (!(exp)) { \
	    printf("Assertion \"%s\" failed: file %s, line %d\n", \
		#exp, __FILE__, __LINE__); \
	    exit(1); \
	}
#else
#define	assert(exp)	/* */
#endif /* _ASSERT_H */
