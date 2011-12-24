/*	$NetBSD: autoconf.h,v 1.1.2.3 2011/12/24 01:44:43 matt Exp $	*/

#ifndef _EVBMIPS_RMIXL_AUTOCONF_H_
#define _EVBMIPS_RMIXL_AUTOCONF_H_

struct mainbus_attach_args {
	const char *	ma_name;
	bus_dma_tag_t	ma_dmat29;
	bus_dma_tag_t	ma_dmat32;
	bus_dma_tag_t	ma_dmat64;
	int		ma_node;
};

#endif	/* _EVBMIPS_RMIXL_AUTOCONF_H_ */
