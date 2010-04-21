#ifndef _IFCONFIG_UTIL_H
#define _IFCONFIG_UTIL_H

#include <netinet/in.h>

#include "parse.h"

struct afswtch {
	const char *af_name;
	short af_af;
	void (*af_status)(prop_dictionary_t, prop_dictionary_t, bool);
	void (*af_addr_commit)(prop_dictionary_t, prop_dictionary_t);
	SIMPLEQ_ENTRY(afswtch)	af_next;
};

void print_link_addresses(prop_dictionary_t, bool);
const char *get_string(const char *, const char *, u_int8_t *, int *);
const struct afswtch *lookup_af_byname(const char *);
const struct afswtch *lookup_af_bynum(int);
void	print_string(const u_int8_t *, int);
int    getsock(int);
struct paddr_prefix *prefixlen_to_mask(int, int);
int direct_ioctl(prop_dictionary_t, unsigned long, void *);
int indirect_ioctl(prop_dictionary_t, unsigned long, void *);
#ifdef INET6
void in6_fillscopeid(struct sockaddr_in6 *sin6);
#endif /* INET6	*/

#endif /* _IFCONFIG_UTIL_H */
