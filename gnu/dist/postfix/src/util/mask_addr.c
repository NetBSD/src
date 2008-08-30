/*++
/* NAME
/*	mask_addr 3
/* SUMMARY
/*	address bit banging
/* SYNOPSIS
/*	#include <mask_addr.h>
/*
/*	void	mask_addr(addr_bytes, addr_byte_count, network_bits)
/*	unsigned char *addr_bytes;
/*	unsigned addr_byte_count;
/*	unsigned network_bits;
/* DESCRIPTION
/*	mask_addr() clears all the host bits in the specified
/*	address.  The code can handle addresses of any length,
/*	and bytes of any width.
/*
/*	Arguments:
/* .IP addr_bytes
/*	The network address in network byte order.
/* .IP addr_byte_count
/*	The network address length in bytes.
/* .IP network_bits
/*	The number of initial bits that will not be cleared.
/* DIAGNOSTICS
/*	Fatal errors: the number of network bits exceeds the address size.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <limits.h>			/* CHAR_BIT */

/* Utility library. */

#include <msg.h>
#include <mask_addr.h>

/* mask_addr - mask off a variable-length address */

void    mask_addr(unsigned char *addr_bytes,
		          unsigned addr_byte_count,
		          unsigned network_bits)
{
    unsigned char *p;

    if (network_bits > addr_byte_count * CHAR_BIT)
	msg_panic("mask_addr: address byte count %d too small for bit count %d",
		  addr_byte_count, network_bits);

    p = addr_bytes + network_bits / CHAR_BIT;
    network_bits %= CHAR_BIT;

    if (network_bits != 0)
	*p++ &= ~0 << (CHAR_BIT - network_bits);

    while (p < addr_bytes + addr_byte_count)
	*p++ = 0;
}
