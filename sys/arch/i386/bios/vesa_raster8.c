/* $NetBSD: vesa_raster8.c,v 1.6 2003/02/26 22:21:19 fvdl Exp $ */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <machine/frame.h>
#include <machine/kvm86.h>
#include <machine/bus.h>

#include <arch/i386/bios/vesabios.h>
#include <arch/i386/bios/vesabiosreg.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>

static int vesaraster8_match(struct device *, struct cfdata *, void *);
static void vesaraster8_attach(struct device *, struct device *, void *);

struct vesaraster8sc {
	struct device sc_dev;
	int sc_mode;
	struct rasops_info sc_ri;
};

CFATTACH_DECL(vesarasterviii, sizeof(struct vesaraster8sc),
    vesaraster8_match, vesaraster8_attach, NULL, NULL);

static int
vesaraster8_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct vesabiosdev_attach_args *vaa = aux;

	if (strcmp(vaa->vbaa_type, "raster8"))
		return (0);

	return (1);
}

static void
vesaraster8_attach(parent, dev, aux)
	struct device *parent, *dev;
	void *aux;
{
	struct vesaraster8sc *sc = (struct vesaraster8sc *)dev;
	struct vesabiosdev_attach_args *vaa = aux;
	unsigned char *buf;
	struct trapframe tf;
	int res;
	struct modeinfoblock *mi;
	struct rasops_info *ri;
	bus_space_handle_t h;

	buf = kvm86_bios_addpage(0x2000);
	if (!buf) {
		printf("vesaraster8_attach: kvm86_bios_addpage(0x2000) failed\n");
		return;
	}

	sc->sc_mode = vaa->vbaa_modes[0]; /* XXX */

	memset(&tf, 0, sizeof(struct trapframe));
	tf.tf_eax = 0x4f01; /* function code */
	tf.tf_ecx = sc->sc_mode;
	tf.tf_vm86_es = 0;
	tf.tf_edi = 0x2000; /* buf ptr */

	res = kvm86_bioscall(0x10, &tf);
	if (res || tf.tf_eax != 0x004f) {
		printf("vbecall: res=%d, ax=%x\n", res, tf.tf_eax);
		goto out;
	}
	mi = (struct modeinfoblock *)buf;

	ri = &sc->sc_ri;
	ri->ri_flg = RI_CENTER;
	ri->ri_depth = 8;
	ri->ri_width = mi->XResolution;
	ri->ri_height = mi->YResolution;
	ri->ri_stride = mi->BytesPerScanLine;

	res = _x86_memio_map(X86_BUS_SPACE_MEM, mi->PhysBasePtr,
			      ri->ri_height * ri->ri_stride,
			      BUS_SPACE_MAP_LINEAR, &h);
	if (res) {
		printf("framebuffer mapping failed\n");
		goto out;
	}
	ri->ri_bits = bus_space_vaddr(X86_BUS_SPACE_MEM, h);

	printf(": fb %dx%d @%x\n", ri->ri_width, ri->ri_height,
	       mi->PhysBasePtr);
out:
	kvm86_bios_delpage(0x2000, buf);
	return;
}

#if 0
static void
vb8test(sc)
	struct vesaraster8sc *sc;
{
	struct trapframe tf;
	int res;
	int savbufsiz, i;
	char *errstr = 0;
	struct rasops_info *ri = &sc->sc_ri;
	u_long attr;

	memset(&tf, 0, sizeof(struct trapframe));
	tf.tf_eax = 0x4f04; /* function code */
	tf.tf_edx = 0x0000; /* query */
	tf.tf_ecx = 0x000f;

	res = kvm86_bioscall(0x10, &tf);
	if (res || tf.tf_eax != 0x004f) {
		printf("vbecall: res=%d, ax=%x\n", res, tf.tf_eax);
		return;
	}
	savbufsiz = roundup(tf.tf_ebx * 64, NBPG) / NBPG;
	printf("savbuf: %d pg\n", savbufsiz);

	for (i = 0; i < savbufsiz; i++)
		(void)kvm86_bios_addpage(0x3000 + i * 0x1000);

	memset(&tf, 0, sizeof(struct trapframe));
	tf.tf_eax = 0x4f04; /* function code */
	tf.tf_edx = 0x0001; /* save */
	tf.tf_ecx = 0x000f; /* all (but framebuffer) */
	tf.tf_vm86_es = 0;
	tf.tf_ebx = 0x3000; /* buffer */

	res = kvm86_bioscall(0x10, &tf);
	if (res || tf.tf_eax != 0x004f) {
		printf("vbecall: res=%d, ax=%x\n", res, tf.tf_eax);
		return;
	}

	memset(&tf, 0, sizeof(struct trapframe));
	tf.tf_eax = 0x4f02; /* function code */
	tf.tf_ebx = sc->sc_mode | 0x4000; /* flat */
	tf.tf_ebx |= 0x8000; /* XXX no clear (save fonts in fb) */

	res = kvm86_bioscall(0x10, &tf);
	if (res || tf.tf_eax != 0x004f) {
		printf("vbecall: res=%d, ax=%x\n", res, tf.tf_eax);
		return;
	}

	if (rasops_init(ri, 20, 50)) {
		errstr = "rasops_init";
		goto back;
	}

	ri->ri_ops.allocattr(ri, 0, 0, 0, &attr);
	ri->ri_ops.putchar(ri, 1, 1, 'X', attr);
	ri->ri_ops.putchar(ri, 1, 2, 'Y', attr);
	ri->ri_ops.putchar(ri, 1, 3, 'Z', attr);

	delay(5000000);
	errstr = 0;
back:
	memset(&tf, 0, sizeof(struct trapframe));
	tf.tf_eax = 0x4f04; /* function code */
	tf.tf_edx = 0x0002; /* restore */
	tf.tf_ecx = 0x000f;
	tf.tf_vm86_es = 0;
	tf.tf_ebx = 0x3000; /* buffer */

	res = kvm86_bioscall(0x10, &tf);
	if (res || tf.tf_eax != 0x004f) {
		printf("vbecall: res=%d, ax=%x\n", res, tf.tf_eax);
		return;
	}

	if (errstr)
		printf("error: %s\n", errstr);
}
#endif
