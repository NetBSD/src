/*	$NetBSD: ntpdc-layout.c,v 1.3 2015/07/10 14:20:33 christos Exp $	*/

/*
 * ntpdc-layout - print layout of NTP mode 7 request/response packets
 */

#include <config.h>
#include <stdio.h>
#include <stddef.h>

#include "ntpdc.h"
#include "ntp_stdlib.h"

#if defined(IMPL_XNTPD_OLD) && IMPL_XNTPD != 3
#error Unexpected IMPL_XNTPD
#endif

int
main(void)
{
#include "nl.c"

  return (EXIT_SUCCESS);
}
