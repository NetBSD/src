/*	$NetBSD: wsconsio.h,v 1.3 2015/09/07 03:49:45 dholland Exp $	*/

#ifndef _NEWSMIPS_WSCONSIO_H_
#define	_NEWSMIPS_WSCONSIO_H_

#include <sys/ioccom.h>
#include <dev/wscons/wsconsio.h>

struct newsmips_wsdisplay_fbinfo {
	struct wsdisplay_fbinfo wsdisplay_fbinfo;
	u_int stride;
};

#define	NEWSMIPS_WSDISPLAYIO_GINFO						\
	_IOR('n', 65, struct newsmips_wsdisplay_fbinfo)

#endif /* !_NEWSMIPS_WSCONSIO_H_ */
