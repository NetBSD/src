/*	$NetBSD: ntp_random.h,v 1.1.1.1.22.1 2014/12/25 02:13:01 snj Exp $	*/


#include <ntp_types.h>

void ntp_crypto_srandom(void);
int ntp_crypto_random_buf(void *buf, size_t nbytes);

long ntp_random (void);
void ntp_srandom (unsigned long);
void ntp_srandomdev (void);
char * ntp_initstate (unsigned long, 	/* seed for R.N.G. */
			char *,		/* pointer to state array */
			long		/* # bytes of state info */
			);
char * ntp_setstate (char *);	/* pointer to state array */



