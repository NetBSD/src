/*	$NetBSD: utilities.c,v 1.1.1.1 2009/12/13 16:57:12 kardel Exp $	*/

#include "utilities.h"

/* Display a NTP packet in hex with leading address offset 
 * e.g. offset: value, 0: ff 1: fe ... 255: 00
 */ 
void 
pkt_output (
		struct pkt *dpkg,
		int pkt_length, 
		FILE *output
	   )
{
	register int a;
	u_char *pkt;

	pkt = (u_char *)dpkg;

	fprintf(output, HLINE);

	for (a = 0; a < pkt_length; a++) {
		if (a > 0 && a % 8 == 0)
			fprintf(output, "\n");

		fprintf(output, "%d: %x \t", a, pkt[a]);
	}

	fprintf(output, "\n");
	fprintf(output, HLINE);
}

/* Output a long floating point value in hex in the style described above 
 */
void
l_fp_output (
		l_fp *ts,
		FILE *output
	    )
{
	register int a;

	fprintf(output, HLINE);

	for(a=0; a<8; a++) 
		fprintf(output, "%i: %x \t", a, ((unsigned char *) ts)[a]);

	fprintf(output, "\n");
	fprintf(output, HLINE);

}

/* Output a long floating point value in binary in the style described above
 */
void 
l_fp_output_bin (
		l_fp *ts,
		FILE *output
		)
{
	register int a, b;

	fprintf(output, HLINE);

	for(a=0; a<8; a++) {
		short tmp = ((unsigned char *) ts)[a];
		tmp++;

		fprintf(output, "%i: ", a);

		for(b=7; b>=0; b--) {
			int texp = (int) pow(2, b);

			if(tmp - texp > 0) {
				fprintf(output, "1");
				tmp -= texp;
			}
			else {
				fprintf(output, "0");
			}
		}

		fprintf(output, " ");
	}

	fprintf(output, "\n");
	fprintf(output, HLINE);
}

/* Output a long floating point value in decimal in the style described above
 */
void
l_fp_output_dec (
		l_fp *ts,
		FILE *output
	    )
{
	register int a;

	fprintf(output, HLINE);

	for(a=0; a<8; a++) 
		fprintf(output, "%i: %i \t", a, ((unsigned char *) ts)[a]);

	fprintf(output, "\n");
	fprintf(output, HLINE);

}

/* Convert a struct addrinfo to a string containing the address in style
 * of inet_ntoa
 */
char *
addrinfo_to_str (
		struct addrinfo *addr
		)
{
	char *buf = (char *) emalloc(sizeof(char) * INET6_ADDRSTRLEN);

	getnameinfo(addr->ai_addr, addr->ai_addrlen, buf, 
			INET6_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);

	return buf;
}

/* Convert a sockaddr_u to a string containing the address in
 * style of inet_ntoa
 * Why not switch callers to use stoa from libntp?  No free() needed
 * in that case.
 */
char *
ss_to_str (
		sockaddr_u *saddr
		)
{
	char *buf = (char *) emalloc(sizeof(char) * INET6_ADDRSTRLEN);

	getnameinfo(&saddr->sa, SOCKLEN(saddr), buf,
			INET6_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);


	return buf;
}

/* Converts a struct tv to a date string
 */
char *
tv_to_str (
		struct timeval *tv
	  )
{
	static const char *month_names[] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};

	char *buf = (char *) emalloc(sizeof(char) * 48);
	time_t cur_time = time(NULL);
	struct tm *tm_ptr;

	tm_ptr = localtime(&cur_time);


	snprintf(buf, 48, "%i %s %.2d %.2d:%.2d:%.2d.%.3d", 
			tm_ptr->tm_year + 1900,
			month_names[tm_ptr->tm_mon],
			tm_ptr->tm_mday,
			tm_ptr->tm_hour,
			tm_ptr->tm_min,
			tm_ptr->tm_sec,
			(int)tv->tv_usec);

	return buf;
}



		
