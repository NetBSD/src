#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <machine/bus.h>
#include <machine/bootinfo.h>

#include "vga.h"
#include "ega.h"
#include "pcdisplay.h"
#if (NVGA > 0) || (NEGA > 0) || (NPCDISPLAY > 0)
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#if (NVGA > 0)
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>
#endif
#if (NEGA > 0)
#include <dev/isa/egavar.h>
#endif
#if (NPCDISPLAY > 0)
#include <dev/isa/pcdisplayvar.h>
#endif
#endif

#include "pckbc.h"
#if (NPCKBC > 0)
#include <dev/isa/isareg.h>
#include <dev/ic/pckbcvar.h>
#endif

#include "pc.h"
#if (NPC > 0)
#include <machine/pccons.h>
#endif

#include "vt.h"
#if (NVT > 0)
#include <i386/isa/pcvt/pcvt_cons.h>
#endif

#include "com.h"
#if (NCOM > 0)
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

#ifndef CONSDEVNAME
#define CONSDEVNAME "pc"
#endif

#if (NCOM > 0)
#ifndef CONADDR
#define CONADDR 0x3f8
#endif
#ifndef CONSPEED
#define CONSPEED TTYDEF_SPEED
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
int comcnmode = CONMODE;
#endif /* NCOM */

struct btinfo_console default_consinfo = {
	{0, 0},
	CONSDEVNAME,
#if (NCOM > 0)
	CONADDR, CONSPEED
#else
	0, 0
#endif
};

#ifdef KGDB
#ifndef KGDB_DEVNAME
#define KGDB_DEVNAME "com"
#endif
char kgdb_devname[] = KGDB_DEVNAME;

#if (NCOM > 0)
#ifndef KGDBADDR
#define KGDBADDR 0x3f8
#endif
int comkgdbaddr = KGDBADDR;
#ifndef KGDBRATE
#define KGDBRATE TTYDEF_SPEED
#endif
int comkgdbrate = KGDBRATE;
#ifndef KGDBMODE
#define KGDBMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
int comkgdbmode = KGDBMODE;
#endif /* NCOM */

#endif /* KGDB */

/*
 * consinit:
 * initialize the system console.
 * XXX - shouldn't deal with this initted thing, but then,
 * it shouldn't be called from init386 either.
 */
void
consinit()
{
	struct btinfo_console *consinfo;
	static int initted;

	if (initted)
		return;
	initted = 1;

#ifndef CONS_OVERRIDE
	consinfo = lookup_bootinfo(BTINFO_CONSOLE);
	if (!consinfo)
#endif
		consinfo = &default_consinfo;

#if (NPC > 0) || (NVT > 0) || (NVGA > 0) || (NEGA > 0) || (NPCDISPLAY > 0)
	if (!strcmp(consinfo->devname, "pc")) {
#if (NVGA > 0)
		if (!vga_cnattach(I386_BUS_SPACE_IO, I386_BUS_SPACE_MEM,
				  -1, 1))
			goto dokbd;
#endif
#if (NEGA > 0)
		if (!ega_cnattach(I386_BUS_SPACE_IO, I386_BUS_SPACE_MEM))
			goto dokbd;
#endif
#if (NPCDISPLAY > 0)
		if (!pcdisplay_cnattach(I386_BUS_SPACE_IO, I386_BUS_SPACE_MEM))
			goto dokbd;
#endif
#if (NPC > 0) || (NVT > 0)
		pccnattach();
#endif
		if (0) goto dokbd; /* XXX stupid gcc */
dokbd:
#if (NPCKBC > 0)
		pckbc_cnattach(I386_BUS_SPACE_IO, IO_KBD, PCKBC_KBD_SLOT);
#endif
		return;
	}
#endif /* PC | VT | VGA | PCDISPLAY */
#if (NCOM > 0)
	if (!strcmp(consinfo->devname, "com")) {
		bus_space_tag_t tag = I386_BUS_SPACE_IO;

		if (comcnattach(tag, consinfo->addr, consinfo->speed,
				COM_FREQ, comcnmode))
			panic("can't init serial console @%x", consinfo->addr);

		return;
	}
#endif
	panic("invalid console device %s", consinfo->devname);
}

#if (NPCKBC > 0) && (NPCKBD == 0)
/*
 * glue code to support old console code with the
 * mi keyboard controller driver
 */
int
pckbc_machdep_cnattach(kbctag, kbcslot)
	pckbc_tag_t kbctag;
	pckbc_slot_t kbcslot;
{
#if (NPC > 0) && (NPCCONSKBD > 0)
	return (pcconskbd_cnattach(kbctag, kbcslot));
#else
	return (ENXIO);
#endif
}
#endif

#ifdef KGDB
void
kgdb_port_init()
{
#if (NCOM > 0)
	if(!strcmp(kgdb_devname, "com")) {
		bus_space_tag_t tag = I386_BUS_SPACE_IO;

		com_kgdb_attach(tag, comkgdbaddr, comkgdbrate, COM_FREQ, 
		    comkgdbmode);
	}
#endif
}
#endif
