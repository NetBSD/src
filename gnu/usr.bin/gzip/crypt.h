/* crypt.h (dummy version) -- do not perform encryption
 * Hardly worth copyrighting :-)
 *
 *	$Id: crypt.h,v 1.3 1993/10/15 23:05:28 jtc Exp $
 */

#ifdef CRYPT
#  undef CRYPT      /* dummy version */
#endif

#define RAND_HEAD_LEN  12  /* length of encryption random header */

#define zencode
#define zdecode
