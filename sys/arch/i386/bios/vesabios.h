/* $NetBSD: vesabios.h,v 1.2.24.1 2005/02/12 18:17:33 yamt Exp $ */

int vbeprobe(void);

struct vesabios_attach_args {
	char *vaa_busname;
};

struct vesabiosdev_attach_args {
	char *vbaa_type;
	int *vbaa_modes;
	int vbaa_nmodes;
};
