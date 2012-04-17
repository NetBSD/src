/*	$NetBSD: printtcpflags.c,v 1.1.1.1.2.2 2012/04/17 00:03:21 yamt Exp $	*/

#include "ipf.h"


void
printtcpflags(tcpf, tcpfm)
	u_32_t tcpf, tcpfm;
{
	u_char *t;
	char *s;

	if (tcpf & ~TCPF_ALL) {
		PRINTF("0x%x", tcpf);
	} else {
		for (s = flagset, t = flags; *s; s++, t++) {
			if (tcpf & *t)
				(void)putchar(*s);
		}
	}

	if (tcpfm) {
		(void)putchar('/');
		if (tcpfm & ~TCPF_ALL) {
			PRINTF("0x%x", tcpfm);
		} else {
			for (s = flagset, t = flags; *s; s++, t++)
				if (tcpfm & *t)
					(void)putchar(*s);
		}
	}
}
