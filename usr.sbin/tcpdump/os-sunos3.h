/*
 * Copyright (c) 1990, 1993, 1994
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
 * @(#) Header: os-sunos3.h,v 1.15 94/06/12 14:34:16 leres Exp (LBL)
 */

/* Map things in the ether_arp struct */
#define SHA(ap) ((ap)->arp_xsha)
#define SPA(ap) ((ap)->arp_xspa)
#define THA(ap) ((ap)->arp_xtha)
#define TPA(ap) ((ap)->arp_xtpa)

#define EDST(ep) ((ep)->ether_dhost.ether_addr_octet)
#define ESRC(ep) ((ep)->ether_shost.ether_addr_octet)

/* Prototypes missing in SunOS 3 */
int	_flsbuf(u_char, FILE *);
u_int	alarm(u_int);
int	atoi(const char *);
int	bcmp(const char *, const char *, u_int);
void	bcopy(const void *, void *, u_int);
char	*calloc(u_int, u_int);
int	close(int);
void	endservent(void);
__dead	void exit(int);
int	fclose(FILE *);
int	fflush(FILE *);
int	fprintf(FILE *, const char *, ...);
int	fputc(int, FILE *);
int	fputs(const char *, FILE *);
u_int	fread(void *, u_int, u_int, FILE *);
void	free(void *);
u_int	fwrite(const void *, u_int, u_int, FILE *);
int	getopt(int, char * const *, const char *);
int	gettimeofday(struct timeval *, struct timezone *);
uid_t	getuid(void);
int	ioctl(int, int, caddr_t);
off_t	lseek(int, off_t, int);
char	*malloc(u_int);
int	open(char *, int);
void	perror(const char *);
int	printf(const char *, ...);
int	puts(const char *);
int	read(int, char *, u_int);
int	setlinebuf(FILE *);
int	setuid(uid_t);
int	socket(int, int, int);
char	*sprintf(char *, const char *, ...);
int	sscanf(char *, const char *, ...);
int	strcasecmp(const char *, const char *);
long	tell(int);
int	vfprintf(FILE *, const char *, ...);
