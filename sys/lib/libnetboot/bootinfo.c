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
