/*
 * source in this file came from
 * the Mach ethernet boot written by Leendert van Doorn.
 *
 * assert.h
 *
 *	$Id: assert.h,v 1.2 1993/08/02 17:52:50 mycroft Exp $
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
