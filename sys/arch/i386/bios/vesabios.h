/* $NetBSD: vesabios.h,v 1.2.6.2 2002/09/06 08:35:58 jdolecek Exp $ */

int vbeprobe __P((void));

struct vesabios_attach_args {
	char *vaa_busname;
};

struct vesabiosdev_attach_args {
	char *vbaa_type;
	int *vbaa_modes;
	int vbaa_nmodes;
};
