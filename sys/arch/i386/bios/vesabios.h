/* $NetBSD: vesabios.h,v 1.3 2005/02/03 20:24:25 perry Exp $ */

int vbeprobe(void);

struct vesabios_attach_args {
	char *vaa_busname;
};

struct vesabiosdev_attach_args {
	char *vbaa_type;
	int *vbaa_modes;
	int vbaa_nmodes;
};
