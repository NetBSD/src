/* crypt.h (dummy version) -- do not perform encryption
 * Hardly worth copyrighting :-)
 *
 *	$Id: crypt.h,v 1.2 1993/08/02 17:46:16 mycroft Exp $
 */

#ifdef CRYPT
#  undef CRYPT      /* dummy version */
#endif

#define RAND_HEAD_LEN  12  /* length of encryption random header */

#define zencode
#define zdecode
