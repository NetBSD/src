/*	$NetBSD: ntp_random.h,v 1.6 2024/08/18 20:46:50 christos Exp $	*/


#include <ntp_types.h>

void ntp_crypto_srandom(void);
int ntp_crypto_random_buf(void *buf, size_t nbytes);

long ntp_random (void);
double ntp_uurandom(void);
void ntp_srandom (unsigned long);
void ntp_srandomdev (void);
char * ntp_initstate (unsigned long, 	/* seed for R.N.G. */
			char *,		/* pointer to state array */
			long		/* # bytes of state info */
			);
char * ntp_setstate (char *);	/* pointer to state array */



