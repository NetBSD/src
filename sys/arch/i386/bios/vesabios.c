/* $NetBSD: vesabios.c,v 1.1 2002/07/07 12:59:58 drochner Exp $ */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <machine/frame.h>
#include <machine/kvm86.h>

#include <arch/i386/bios/vesabios.h>

struct vbeinfoblock
{
	char VbeSignature[4];
	u_int16_t VbeVersion;
	u_int32_t OemStringPtr;
	u_int32_t Capabilities;
	u_int32_t VideoModePtr;
	u_int16_t TotalMemory;
	u_int16_t OemSoftwareRev;
	u_int32_t OemVendorNamePtr, OemProductNamePtr, OemProductRevPtr;
	/* data area, in total max 512 bytes for VBE 2.0 */
} __attribute__ ((packed));

static int vesabios_match(struct device *, struct cfdata *, void *);

struct cfattach vesabios_ca = {
	sizeof(struct device), vesabios_match, 0
};

static int
vesabios_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct vesabios_attach_args *vaa = aux;

	/* These are not the droids you're looking for. */
	if (strcmp(vaa->vaa_busname, "vesabios") != 0)
		return (0);

	return (0); /* XXX for now */
}

int
vbeprobe()
{
	unsigned char *buf;
	struct trapframe tf;
	int res, ok = 0;
	struct vbeinfoblock *vi;

	buf = kvm86_bios_addpage(0x2000);
	if (!buf) {
		printf("vbeprobe: kvm86_bios_addpage(0x2000) failed\n");
		return (0);
	}

	memcpy(buf, "VBE2", 4);

	memset(&tf, 0, sizeof(struct trapframe));
	tf.tf_eax = 0x4f00; /* function code */
	tf.tf_es = 0;
	tf.tf_edi = 0x2000; /* buf ptr */

	res = kvm86_bioscall(0x10, &tf);

	printf("vbecall: res=%d, ax=%x\n", res, tf.tf_eax);
	if (res || tf.tf_eax != 0x004f)
		goto out;

	vi = (struct vbeinfoblock *)buf;
	if (memcmp(vi->VbeSignature, "VESA", 4)) {
		printf("VESA: bad sig\n");
		goto out;
	}
	printf("VESA: version: %x, cap: %x, oem @%x, modes @%x\n",
	       vi->VbeVersion, vi->Capabilities, vi->OemStringPtr,
	       vi->VideoModePtr);
	ok = 1;

out:
	kvm86_bios_delpage(0x2000, buf);
	return (ok);
}
