#ifndef _IFCONFIG_AF_INETANY_H
#define _IFCONFIG_AF_INETANY_H

#include <sys/types.h>
#include <prop/proplib.h>

#define	IFADDR_PARAM(__arg)	{.cmd = (__arg), .desc = #__arg}
#define	BUFPARAM(__arg) 	{.buf = &(__arg), .buflen = sizeof(__arg)}

struct apbuf {
	void *buf;
	size_t buflen;
};

struct afparam {
	struct {
		char *buf;
		size_t buflen;
	} name[2];
	struct apbuf dgaddr, addr, brd, dst, mask, req, dgreq, defmask,
	    pre_aifaddr_arg;
	struct {
		unsigned long cmd;
		const char *desc;
	} aifaddr, difaddr, gifaddr;
	int (*pre_aifaddr)(prop_dictionary_t, void *);
};

void	commit_address(prop_dictionary_t, prop_dictionary_t, struct afparam *);

#endif /* _IFCONFIG_AF_INETANY_H */
