/*
 * Copyright (c) 1989, 1990, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * @(#) Header: os-sunos4.h,v 1.17 94/06/12 14:34:23 leres Exp (LBL)
 */

/* Map things in the ether_arp struct */
#define SHA(ap) ((ap)->arp_sha.ether_addr_octet)
#define SPA(ap) ((ap)->arp_spa)
#define THA(ap) ((ap)->arp_tha.ether_addr_octet)
#define TPA(ap) ((ap)->arp_tpa)

#define EDST(ep) ((ep)->ether_dhost.ether_addr_octet)
#define ESRC(ep) ((ep)->ether_shost.ether_addr_octet)

/* Map protocol types */
#define ETHERPUP_IPTYPE ETHERTYPE_IP
#define ETHERPUP_REVARPTYPE ETHERTYPE_REVARP
#define ETHERPUP_ARPTYPE ETHERTYPE_ARP

/* Eliminate some bogus warnings. */
struct timezone;
struct timeval;

/* Prototypes missing in SunOS 4 */
int	_flsbuf(u_char, FILE *);
int	bcmp(const char *, const char *, u_int);
void	bcopy(const void *, void *, u_int);
int	close(int);
void	endservent(void);
int	fclose(FILE *);
int	fflush(FILE *);
int	fprintf(FILE *, const char *, ...);
int	fputc(int, FILE *);
int	fputs(const char *, FILE *);
u_int	fread(void *, u_int, u_int, FILE *);
u_int	fwrite(const void *, u_int, u_int, FILE *);
long	gethostid(void);
int	getopt(int, char * const *, const char *);
int	gettimeofday(struct timeval *, struct timezone *);
int	ioctl(int, int, caddr_t);
off_t	lseek(int, off_t, int);
#ifdef __GNUC__
#if __GNUC__ == 1
void	*malloc(u_int);
#endif
#else
char	*malloc(u_int);
#endif
void	perror(const char *);
int	printf(const char *, ...);
int	puts(const char *);
#if __GNUC__ <= 1
int	read(int, char *, u_int);
#endif
int	setlinebuf(FILE *);
int	socket(int, int, int);
int	sscanf(char *, const char *, ...);
int	strcasecmp(const char *, const char *);
long	tell(int);
int	vfprintf(FILE *, const char *, ...);
