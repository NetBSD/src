/* $NetBSD: vesabios.h,v 1.2.8.2 2002/10/18 03:13:08 nathanw Exp $ */

int vbeprobe __P((void));

struct vesabios_attach_args {
	char *vaa_busname;
};

struct vesabiosdev_attach_args {
	char *vbaa_type;
	int *vbaa_modes;
	int vbaa_nmodes;
};
