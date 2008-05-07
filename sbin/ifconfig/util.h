#ifndef _IFCONFIG_UTIL_H
#define _IFCONFIG_UTIL_H

#include "parse.h"

struct afswtch {
	const char *af_name;
	short af_af;
	void (*af_status)(prop_dictionary_t, prop_dictionary_t, bool);
	void (*af_getaddr)(const struct paddr_prefix *, int);
	void (*af_getprefix)(int, int);
	void (*af_addr_commit)(prop_dictionary_t, prop_dictionary_t);
	unsigned long af_difaddr;
	unsigned long af_aifaddr;
	unsigned long af_gifaddr;
	void *af_ridreq;
	void *af_addreq;
};

const char *get_string(const char *, const char *, u_int8_t *, int *);
const struct afswtch *lookup_af_byname(const char *);
const struct afswtch *lookup_af_bynum(int);
void	print_string(const u_int8_t *, int);
int    getsock(int);
struct paddr_prefix *prefixlen_to_mask(int, int);
int direct_ioctl(prop_dictionary_t, unsigned long, void *);
int indirect_ioctl(prop_dictionary_t, unsigned long, void *);

#endif /* _IFCONFIG_UTIL_H */
