/*	$NetBSD: scaffold.h,v 1.5 2012/03/21 10:11:34 matt Exp $	*/

 /*
  * @(#) scaffold.h 1.3 94/12/31 18:19:19
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

struct addrinfo *find_inet_addr(char *, int);
int check_dns(char *);
void shell_cmd(char *);
void clean_exit(struct request_info *);
#if 0
void rfc931(struct request_info *);
#endif
int check_path(const char *, struct stat *);
