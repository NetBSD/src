/*	$NetBSD: ntpdc-layout.c,v 1.1.1.1.12.1 2014/08/19 23:51:42 tls Exp $	*/

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
