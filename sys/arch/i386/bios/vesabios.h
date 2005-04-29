/* $NetBSD: vesabios.h,v 1.2.22.1 2005/04/29 11:28:11 kent Exp $ */

int vbeprobe(void);

struct vesabios_attach_args {
	char *vaa_busname;
};

struct vesabiosdev_attach_args {
	char *vbaa_type;
	int *vbaa_modes;
	int vbaa_nmodes;
};
