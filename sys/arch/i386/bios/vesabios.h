/* $NetBSD: vesabios.h,v 1.4 2005/06/01 16:49:14 drochner Exp $ */

int vbeprobe(void);

struct vesabiosdev_attach_args {
	const char *vbaa_type;
	int *vbaa_modes;
	int vbaa_nmodes;
};
