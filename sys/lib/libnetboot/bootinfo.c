/*	$NetBSD: bootinfo.c,v 1.2 1994/10/26 06:43:04 cgd Exp $	*/

#include <sys/param.h>
#include <sys/types.h>
#include <sys/cdefs.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include "netboot.h"
#include "bootbootp.h"
#include "bootp.h"
#include "netif.h"

void get_bootinfo(desc)
     struct iodesc *desc;
{
    bootp(desc);
}
