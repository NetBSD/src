/* $XFree86: xc/programs/Xserver/hw/xfree68/fbdev/fbdev.c,v 3.1 1996/08/18 03:54:46 dawes Exp $ */
/*
 *
 *  Author: Thomas Gerner. Taken from fbdev.c by Martin Schaller
 *
 *  Generic version by Geert Uytterhoeven (Geert.Uytterhoeven@cs.kuleuven.ac.be)
 *
 *  This version contains support for:
 *
 *	- Monochrome, 1 bitplane [mfb]
 *	- Color, 4/8 interleaved bitplanes with 2 bytes interleave [iplan2p?]
 */


#define fbdev_PATCHLEVEL "4"


#include "X.h"
#include "input.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "regionstr.h"
#include "mipointer.h"
#include "cursorstr.h"
#include "gcstruct.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86_Config.h"
#include "mfb.h"

#include "colormapst.h"
#include "resource.h"

#undef MODES

#include <sys/queue.h>
#include "/sys/arch/atari/dev/grfabs_reg.h"
#include "/sys/arch/atari/dev/viewioctl.h"

static int *fbdevPrivateIndexP;
static void (*fbdevBitBlt)();
static int (*CreateDefColormap)(ScreenPtr);

extern int mfbCreateDefColormap(ScreenPtr);

#if defined(CONFIG_IPLAN2p2) || defined(CONFIG_IPLAN2p4) || \
     defined(CONFIG_IPLAN2p8)
extern int iplCreateDefColormap(ScreenPtr);
#ifdef CONFIG_IPLAN2p2
extern int ipl2p2ScreenPrivateIndex;
extern void ipl2p2DoBitblt();
#endif /* CONFIG_IPLAN2p2 */
#ifdef CONFIG_IPLAN2p4
extern int ipl2p4ScreenPrivateIndex;
extern void ipl2p4DoBitblt();
#endif /* CONFIG_IPLAN2p4 */
#ifdef CONFIG_IPLAN2p8
extern int ipl2p8ScreenPrivateIndex;
extern void ipl2p8DoBitblt();
#endif /* CONFIG_IPLAN2p8 */
#endif /* defined(CONFIG_IPLAN2p2) || defined(CONFIG_IPLAN2p4) || \
	  defined(CONFIG_IPLAN2p8) */
#ifdef CONFIG_CFB16
extern int cfb16ScreenPrivateIndex;
extern void cfb16DoBitblt();
#endif /* CONFIG_CFB16 */

extern int fbdevValidTokens[];

static Bool fbdevProbe(void);
static void fbdevPrintIdent(void);
static Bool fbdevSaveScreen(ScreenPtr pScreen, int on);
static Bool fbdevScreenInit(int scr_index, ScreenPtr pScreen, int argc,
			    char **argv);
static void fbdevEnterLeaveVT(Bool enter, int screen_idx);
static Bool fbdevCloseScreen(int screen_idx, ScreenPtr screen);
static Bool fbdevValidMode(DisplayModePtr mode);
static Bool fbdevSwitchMode(DisplayModePtr mode);

static void xfree2fbdev(DisplayModePtr mode, struct view_size *var);
static void fbdev2xfree(struct view_size *var, DisplayModePtr mode);

extern Bool xf86Exiting, xf86Resetting, xf86ProbeFailed;

ScrnInfoRec fbdevInfoRec = {
    FALSE,			/* Bool configured */
    -1,				/* int tmpIndex */
    -1,				/* int scrnIndex */
    fbdevProbe,			/* Bool (*Probe)() */
    fbdevScreenInit,		/* Bool (*Init)() */
    (Bool (*)())NoopDDA,	/* Bool (*ValidMode)() */
    fbdevEnterLeaveVT,		/* void (*EnterLeaveVT)() */
    (void (*)())NoopDDA,	/* void (*EnterLeaveMonitor)() */
    (void (*)())NoopDDA,	/* void (*EnterLeaveCursor)() */
    (void (*)())NoopDDA,	/* void (*AdjustFrame)() */
    (Bool (*)())NoopDDA,	/* void (*SwitchMode)() */
    (void (*)())NoopDDA,	/* void (*DPMSSet)() */
    fbdevPrintIdent,		/* void (*PrintIdent)() */
    8,				/* int depth */
    {0, },			/* xrgb weight */
    8,				/* int bitsPerPixel */
    PseudoColor,		/* int defaultVisual */
    -1, -1,			/* int virtualX,virtualY */
    -1,				/* int displayWidth */
    -1, -1, -1, -1,		/* int frameX0, frameY0, frameX1, frameY1 */
    {0, },			/* OFlagSet options */
    {0, },			/* OFlagSet clockOptions */
    {0, },			/* OFlagSet xconfigFlag */
    NULL,			/* char *chipset */
    NULL,			/* char *ramdac */
    {0, },			/* int dacSpeeds[MAXDACSPEEDS] */
    0,				/* int dacSpeed */
    0,				/* int clocks */
    {0, },			/* int clock[MAXCLOCKS] */
    0,				/* int maxClock */
    0,				/* int videoRam */
    0,				/* int BIOSbase */
    0,				/* unsigned long MemBase */
    240, 180,			/* int width, height */
    0,				/* unsigned long speedup */
    NULL,			/* DisplayModePtr modes */
    NULL,			/* MonPtr monitor */
    NULL,			/* char *clockprog */
    -1,				/* int textclock */
    FALSE,			/* Bool bankedMono */
    "FBDev",			/* char *name */
    {0, },			/* RgbRec blackColour */
    {0, },			/* RgbRec whiteColour */
    fbdevValidTokens,		/* int *validTokens */
    fbdev_PATCHLEVEL,		/* char *patchLevel */
    0,				/* unsigned int IObase */
    0,				/* unsigned int DACbase */
    0,				/* unsigned int COPbase */
    0,				/* unsigned int POSbase */
    0,				/* unsigned int instance */
    0,				/* int s3Madjust */
    0,				/* int s3Nadjust */
    0,				/* int s3MClk */
    0,				/* int chipID */
    0,				/* int chipRev */
    0,				/* unsigned long VGAbase */
    0,				/* int s3RefClk */
    -1,				/* int s3BlankDelay */
    0,				/* int textClockFreq */
    NULL,			/* char *DCConfig */
    NULL,			/* char *DCOptions */
    0,				/* int MemClk */
    0,				/* int LCDClk */
#ifdef XFreeXDGA
    0,				/* int directMode */
    NULL,			/* void (*setBank)() */
    0,				/* unsigned long physBase */
    0,				/* int physSize */
#endif
#ifdef XF86SETUP
    NULL,			/* void *device */
#endif
};

static pointer fbdevVirtBase = NULL;

static ScreenPtr savepScreen = NULL;
static PixmapPtr ppix = NULL;

extern miPointerScreenFuncRec xf86PointerScreenFuncs;

#define NOMAPYET	(ColormapPtr)0

static ColormapPtr InstalledMaps[MAXSCREENS];
	/* current colormap for each screen */


#define StaticGrayMask	(1 << StaticGray)
#define GrayScaleMask	(1 << GrayScale)
#define StaticColorMask	(1 << StaticColor)
#define PseudoColorMask	(1 << PseudoColor)
#define TrueColorMask	(1 << TrueColor)
#define DirectColorMask	(1 << DirectColor)

#define ALL_VISUALS	(StaticGrayMask|\
			 GrayScaleMask|\
			 StaticColorMask|\
			 PseudoColorMask|\
			 TrueColorMask|\
			 DirectColorMask)


typedef struct {
    struct view_size vs;
    bmap_t bm;
    colormap_t colormap;
    size_t smem_len;
} fb_fix_screeninfo;

static int fb_fd = -1; 

static u_long colorentry[256];
static int colorbitshift;
static struct view_size vs;
static fb_fix_screeninfo fb_fix;


static Bool UseModeDB = FALSE;


static void open_framebuffer(void)
{
    char buffer[13];
    int i;

    if (fb_fd == -1) {
	for (i = 0; i < 100; i++) {
	    sprintf(buffer, "/dev/view%02d", i);
	    fb_fd = open(buffer, O_RDWR);
	    if (fb_fd < 0 && errno != EBUSY) {
	    	FatalError("open_framebuffer: failed to open %s (%s)\n",
	    	       buffer, strerror(errno));
		fb_fd = -1;
		return;
	    } else if (fb_fd > 0) {
		xf86Info.screenFd = fb_fd;
		return;
	    }
	}
	FatalError("open_framebuffer: Ran out of views.\n");
    }
}

static void close_framebuffer(void)
{
    if (fb_fd != -1) {
	close(fb_fd);
	fb_fd = -1;
    }
}

static pointer MapVidMem(int ScreenNum, unsigned long Size)
{
    pointer base;

    open_framebuffer();
    base = (pointer)mmap((caddr_t)0, Size, PROT_READ | PROT_WRITE,
			 MAP_SHARED, fb_fd, (off_t)0);
    if ((long)base == -1)
	FatalError("MapVidMem: Could not mmap framebuffer (%s)\n",
		   strerror(errno));
    return(base);
}

static void UnMapVidMem(int ScreenNum, unsigned long Size)
{
    munmap(fbdevVirtBase, Size);
    close_framebuffer();
}

static void fbdevUpdateColormap(ScreenPtr pScreen, int dex, int count,
				unsigned char *rmap, unsigned char *gmap,
				unsigned char *bmap)
{
    int		i;
    colormap_t	*cmap = &fb_fix.colormap;

    if (!xf86VTSema)
	/* Switched away from server vt, do nothing. */
	return;
    
#if 0
    while(count--) {
	cmap->entry[dex] = (rmap[dex] << 16 | gmap[dex] << 8 | bmap[dex]);
	dex++;
    }
#else
    cmap->first = dex;
    cmap->size = count;
    
    for (i = 0; i < count; i++, dex++) {
	cmap->entry[i] = (rmap[dex] << 16 | gmap[dex] << 8 | bmap[dex]);
    }
#endif
    if (ioctl(fb_fd, VIOCSCMAP, cmap) < 0)
	FatalError("fbdevUpdateColormap: VIOCSCMAP failed (%s)\n",
			   strerror(errno));
}

static int fbdevListInstalledColormaps(ScreenPtr pScreen, Colormap *pmaps)
{
    *pmaps = InstalledMaps[pScreen->myNum]->mid;
    return(1);
}

static void fbdevStoreColors(ColormapPtr pmap, int ndef, xColorItem *pdefs)
{
    unsigned char rmap[256], gmap[256], bmap[256];
    register int i;
    register int first = -256;
    register int priv = -256;
    register int count = 0;
    register int bitshift = colorbitshift;

    if (pmap != InstalledMaps[pmap->pScreen->myNum])
	return;

    while (ndef--) {
	i = pdefs->pixel;
	if (i != priv + 1) {
	    if (count)
		fbdevUpdateColormap(pmap->pScreen, first, count, rmap, gmap, bmap);
	    first = i;
	    count = 0;
	}
	priv = i;
	rmap[i] = pdefs->red >> bitshift;
	gmap[i] = pdefs->green >> bitshift;
	bmap[i] = pdefs->blue >> bitshift;
	pdefs++;
	count++;
    }
    if (count)
	fbdevUpdateColormap(pmap->pScreen, first, count, rmap, gmap, bmap);
}

static void fbdevInstallColormap(ColormapPtr pmap)
{
    ColormapPtr oldmap = InstalledMaps[pmap->pScreen->myNum];
    int entries = pmap->pVisual->ColormapEntries;
    Pixel ppix[256];
    xrgb prgb[256];
    xColorItem defs[256];
    int i;

    if (pmap == oldmap)
	return;

    if (oldmap != NOMAPYET)
	WalkTree(pmap->pScreen, TellLostMap, &oldmap->mid);

    InstalledMaps[pmap->pScreen->myNum] = pmap;

    for (i = 0 ; i < entries; i++)
	ppix[i]=i;
    QueryColors(pmap, entries, ppix, prgb);

    for (i = 0 ; i < entries; i++) {
	defs[i].pixel = ppix[i];
	defs[i].red = prgb[i].red;
	defs[i].green = prgb[i].green;
	defs[i].blue = prgb[i].blue;
    }

    fbdevStoreColors(pmap, entries, defs);

    WalkTree(pmap->pScreen, TellGainedMap, &pmap->mid);

    return;
}

static void fbdevUninstallColormap(ColormapPtr pmap)
{
    ColormapPtr defColormap;

    if (pmap != InstalledMaps[pmap->pScreen->myNum])
	return;
    defColormap = (ColormapPtr)LookupIDByType(pmap->pScreen->defColormap,
    					      RT_COLORMAP);
    if (defColormap == InstalledMaps[pmap->pScreen->myNum])
	return;
    (*pmap->pScreen->InstallColormap)(defColormap);
}

/*
 *  fbdevPrintIdent -- Prints out identifying strings for drivers included in
 *		       the server
 */

static void fbdevPrintIdent(void)
{
    ErrorF("   %s: Server for frame buffer device\n", fbdevInfoRec.name);
    ErrorF("   (Patchlevel %s): mfb", fbdevInfoRec.patchLevel);
#ifdef CONFIG_IPLAN2p2
    ErrorF(", iplan2p2");
#endif
#ifdef CONFIG_IPLAN2p4
    ErrorF(", iplan2p4");
#endif
#ifdef CONFIG_IPLAN2p8
    ErrorF(", iplan2p8");
#endif
#ifdef CONFIG_CFB16
    ErrorF(", cfb16");
#endif
    ErrorF("\n");
}


static Bool fbdevLookupMode(DisplayModePtr target)
{
    DisplayModePtr p;
    Bool found_mode = FALSE;

    for (p = fbdevInfoRec.monitor->Modes; p != NULL; p = p->next)
	if (!strcmp(p->name, target->name)) {
	    target->Clock          = p->Clock;
	    target->HDisplay       = p->HDisplay;
	    target->HSyncStart     = p->HSyncStart;
	    target->HSyncEnd       = p->HSyncEnd;
	    target->HTotal         = p->HTotal;
	    target->VDisplay       = p->VDisplay;
	    target->VSyncStart     = p->VSyncStart;
	    target->VSyncEnd       = p->VSyncEnd;
	    target->VTotal         = p->VTotal;
	    target->Flags          = p->Flags;
	    target->SynthClock     = p->SynthClock;
	    target->CrtcHDisplay   = p->CrtcHDisplay;
	    target->CrtcHSyncStart = p->CrtcHSyncStart;
	    target->CrtcHSyncEnd   = p->CrtcHSyncEnd;
	    target->CrtcHTotal     = p->CrtcHTotal;
	    target->CrtcVDisplay   = p->CrtcVDisplay;
	    target->CrtcVSyncStart = p->CrtcVSyncStart;
	    target->CrtcVSyncEnd   = p->CrtcVSyncEnd;
	    target->CrtcVTotal     = p->CrtcVTotal;
	    target->CrtcHAdjusted  = p->CrtcHAdjusted;
	    target->CrtcVAdjusted  = p->CrtcVAdjusted;
	    if (fbdevValidMode(target)) {
		found_mode = TRUE;
		break;
	    }
	}
    return(found_mode);
}


/*
 *  fbdevProbe -- Probe and initialize the hardware driver
 */

static Bool fbdevProbe(void)
{
    DisplayModePtr pMode, pEnd;

    open_framebuffer();

    if (ioctl(fb_fd, VIOCGSIZE, &vs))
	FatalError("fbdevProbe: unable to get screen params (%s)\n",
		   strerror(errno));

    pMode = fbdevInfoRec.modes;
    if (pMode == NULL)
	FatalError("No modes supplied in XF86Config\n");

    if (!strcmp(pMode->name, "default")) {
	ErrorF("%s %s: Using default frame buffer video mode\n", XCONFIG_GIVEN,
	       fbdevInfoRec.name);
	fbdevInfoRec.depth = vs.depth;
	fbdevInfoRec.bitsPerPixel = fbdevInfoRec.depth;
    } else {
	ErrorF("%s %s: Using XF86Config video mode database\n", XCONFIG_GIVEN,
	       fbdevInfoRec.name);
	UseModeDB = TRUE;
	pEnd = NULL;
	fbdevInfoRec.bitsPerPixel = fbdevInfoRec.depth;
	do {
	    DisplayModePtr pModeSv;

	    pModeSv = pMode->next;
	    if (!fbdevLookupMode(pMode))
		xf86DeleteMode(&fbdevInfoRec, pMode);
	    else {
		/*
		 *  Successfully looked up this mode.  If pEnd isn't
		 *  initialized, set it to this mode.
		 */
		if (pEnd == (DisplayModePtr)NULL)
		    pEnd = pMode;
	    }
	    pMode = pModeSv;
	} while (pMode != pEnd);
#if 0
	fbdevInfoRec.SwitchMode = fbdevSwitchMode;
#endif
    }

    return(TRUE);
}

static void fbdevRestoreColors(ScreenPtr pScreen)
{
    ColormapPtr pmap = InstalledMaps[pScreen->myNum];
    int entries;
    Pixel ppix[256];
    xrgb prgb[256];
    xColorItem defs[256];
    int i;

    if (!pmap)
	pmap = (ColormapPtr) LookupIDByType(pScreen->defColormap, RT_COLORMAP);
    entries = pmap->pVisual->ColormapEntries;
    if (entries) {
	for (i = 0 ; i < entries; i++)
	    ppix[i] = i;
	QueryColors(pmap, entries, ppix, prgb);

	for (i = 0 ; i < entries; i++) {
	    defs[i].pixel = ppix[i];
	    defs[i].red = prgb[i].red;
	    defs[i].green = prgb[i].green;
	    defs[i].blue = prgb[i].blue;
	}

	fbdevStoreColors(pmap, entries, defs);
    }
}

static Bool fbdevSaveScreen(ScreenPtr pScreen, int on)
{
    switch (on) {
	case SCREEN_SAVER_ON:
	    {
		unsigned char map[256];
		int i, numentries;

		numentries = 1 << fb_fix.bm.depth;

		for (i = 0; i < numentries; i++)
		    map[i] = 0;
		fbdevUpdateColormap(pScreen, 0, numentries, map, map, map);
		return(TRUE);
	    }

	case SCREEN_SAVER_OFF:
	    fbdevRestoreColors(pScreen);
	    return(TRUE);
    }
    return(FALSE);
}

static Bool fbdevSaveScreenDummy(ScreenPtr pScreen, int on)
{
    return(FALSE);
}

/*
 *  fbdevScreenInit -- Attempt to find and initialize a framebuffer
 *		       Most of the elements of the ScreenRec are filled in.
 *		       The video is enabled for the frame buffer...
 *
 *  Arguments:	scr_index	: The index of pScreen in the ScreenInfo
 *		pScreen		: The Screen to initialize
 *		argc		: The number of the Server's arguments.
 *		argv		: The arguments themselves. Don't change!
 */

static Bool fbdevScreenInit(int scr_index, ScreenPtr pScreen, int argc,
			    char **argv)
{
    int displayResolution = 75;	/* default to 75dpi */
    int dxres,dyres;
    extern int monitorResolution;
    struct view_size *var = &vs;
    static unsigned short bw[] = {
	0xffff, 0x0000
    };
    int black, white;
    DisplayModePtr mode;
    int bpp, xsize, ysize;
    char *fbtype;
    Bool NoColormap = FALSE;

    open_framebuffer();

    /*
     *  Take display resolution from the -dpi flag if specified
     */

    if (monitorResolution)
	displayResolution = monitorResolution;

    dxres = displayResolution;
    dyres = displayResolution;

    mode = fbdevInfoRec.modes;
    if (!UseModeDB) {
	fbdev2xfree(var, mode);
	mode->name = "default";
	fbdevInfoRec.virtualX = var->x;
	fbdevInfoRec.virtualY = var->y;
    } else
	xfree2fbdev(mode, var);

    if (ioctl(fb_fd, VIOCSSIZE, var))
	FatalError("fbdevScreenInit: unable to set screen params (%s)\n",
		   strerror(errno));

    if (ioctl(fb_fd, VIOCGSIZE, &fb_fix.vs))
	FatalError("ioctl(fd, VIOCGSIZE, ...)");

    if (ioctl(fb_fd, VIOCGBMAP, &fb_fix.bm))
	 FatalError("ioctl(fd, VIOCGBMAP, ...)");
    
    fb_fix.colormap.first = 0;
    fb_fix.colormap.size = 1 << fb_fix.bm.depth;
    fb_fix.colormap.entry = colorentry;

    if (ioctl(fb_fd, VIOCGCMAP, &fb_fix.colormap))
	FatalError("ioctl(fd, VIOCGCMAP, ...)");

    if (ioctl(fb_fd, VIOCDISPLAY, 0))
	FatalError("ioctl(fd, VIOCDISPLAY ...)");

    switch (fb_fix.colormap.red_mask)
    {
	case 0x0f:
		colorbitshift = 12;
		break;
	case 0x3f:
		colorbitshift = 10;
		break;
	default:
		colorbitshift = 8;
    }

    fb_fix.smem_len = fb_fix.bm.bytes_per_row * fb_fix.bm.rows;
    fbdevInfoRec.videoRam = fb_fix.smem_len>>10;

    if (xf86Verbose) {
	ErrorF("%s %s: Video memory: %dk\n", XCONFIG_PROBED, fbdevInfoRec.name,
	       fbdevInfoRec.videoRam);
	ErrorF("%s %s: Width %d, Height %d, Depth %d\n", XCONFIG_PROBED,
	       fbdevInfoRec.name, fb_fix.vs.width, fb_fix.vs.height,
	       fb_fix.vs.depth);
    }

    fbdevVirtBase = MapVidMem(scr_index, fb_fix.smem_len);
    

    bpp = fb_fix.bm.depth;
    xsize = fb_fix.vs.width;
    ysize = fb_fix.vs.height;

    switch (bpp) {
	case 1:
		if (xf86FlipPixels) {
			black = 1;
			white = 0;
    			NoColormap = TRUE;
		} else {
			black = 0;
			white = 1;
		}
		if ((xsize == 1280) && (ysize == 960)) {
			int temp;
			temp = white; white = black; black = temp;
    			NoColormap = TRUE;
		}
		pScreen->blackPixel = black;
		pScreen->whitePixel = white;
		fbtype = "mfb";
		mfbScreenInit(pScreen, fbdevVirtBase, xsize, ysize, dxres,
			      dyres, xsize);
		fbdevPrivateIndexP = NULL;
		fbdevBitBlt = mfbDoBitblt;
		CreateDefColormap = mfbCreateDefColormap;
		break;
#ifdef CONFIG_IPLAN2p2
	case 2:
		fbtype = "iplan2p2";
		ipl2p2ScreenInit(pScreen, fbdevVirtBase, xsize,
				 ysize, dxres, dyres, xsize);
		fbdevPrivateIndexP=&ipl2p2ScreenPrivateIndex;
		fbdevBitBlt=ipl2p2DoBitblt;
		CreateDefColormap=iplCreateDefColormap;
		break;
#endif

#ifdef CONFIG_IPLAN2p4
	case 4:
		fbtype = "iplan2p4";
		ipl2p4ScreenInit(pScreen, fbdevVirtBase, xsize,
				 ysize, dxres, dyres, xsize);
		fbdevPrivateIndexP=&ipl2p4ScreenPrivateIndex;
		fbdevBitBlt=ipl2p4DoBitblt;
		CreateDefColormap=iplCreateDefColormap;
		break;
#endif

#ifdef CONFIG_IPLAN2p8
	case 8:
		fbtype = "iplan2p8";
		ipl2p8ScreenInit(pScreen, fbdevVirtBase, xsize,
			    	 ysize, dxres, dyres, xsize);
		fbdevPrivateIndexP=&ipl2p8ScreenPrivateIndex;
		fbdevBitBlt=ipl2p8DoBitblt;
		CreateDefColormap=iplCreateDefColormap;
		break;
#endif

#ifdef CONFIG_CFB16
	case 16:
		fbtype = "cfb16";
		cfb16ScreenInit(pScreen, fbdevVirtBase, xsize, ysize,
				dxres, dyres, xsize);
		fbdevPrivateIndexP = &cfb16ScreenPrivateIndex;
		fbdevBitBlt = cfb16DoBitblt;
		CreateDefColormap = cfbCreateDefColormap;
		break;
#endif
	default:
		FatalError("Unsupported bitmap depth %d\n", bpp);
	}

    ErrorF("%s %s: Using %s driver\n", XCONFIG_PROBED, fbdevInfoRec.name,
	   fbtype);
    pScreen->CloseScreen = fbdevCloseScreen;
    pScreen->SaveScreen = (SaveScreenProcPtr)fbdevSaveScreenDummy;

    if ((bpp <=8) && !NoColormap) {
        pScreen->SaveScreen = (SaveScreenProcPtr)fbdevSaveScreen;
	pScreen->InstallColormap = fbdevInstallColormap;
	pScreen->UninstallColormap = fbdevUninstallColormap;
	pScreen->ListInstalledColormaps = fbdevListInstalledColormaps;
	pScreen->StoreColors = fbdevStoreColors;
    }
    miDCInitialize(pScreen, &xf86PointerScreenFuncs);

    if (!(*CreateDefColormap)(pScreen))
	return(FALSE);

    savepScreen = pScreen;

    return(TRUE);
}

/*
 *  fbdevEnterLeaveVT -- Grab/ungrab the current VT completely.
 */

static void fbdevEnterLeaveVT(Bool enter, int screen_idx)
{
    BoxRec pixBox;
    RegionRec pixReg;
    DDXPointRec pixPt;
    PixmapPtr pspix;
    ScreenPtr pScreen = savepScreen;

    if (!xf86Resetting && !xf86Exiting) {
	pixBox.x1 = 0; pixBox.x2 = pScreen->width;
	pixBox.y1 = 0; pixBox.y2 = pScreen->height;
	pixPt.x = 0; pixPt.y = 0;
	(pScreen->RegionInit)(&pixReg, &pixBox, 1);
	if (fbdevPrivateIndexP)
	    pspix = (PixmapPtr)pScreen->devPrivates[*fbdevPrivateIndexP].ptr;
	else
	    pspix = (PixmapPtr)pScreen->devPrivate;
    }

    if (enter) {

	/*
	 *  point pspix back to fbdevVirtBase, and copy the dummy buffer to the
	 *  real screen.
	 */
	if (!xf86Resetting)
	    if (pspix->devPrivate.ptr != fbdevVirtBase && ppix) {
		pspix->devPrivate.ptr = fbdevVirtBase;
		(*fbdevBitBlt)(&ppix->drawable, &pspix->drawable, GXcopy,
			       &pixReg, &pixPt, ~0);
	    }
	if (ppix) {
	    (pScreen->DestroyPixmap)(ppix);
	    ppix = NULL;
	}

	if (!xf86Resetting) {
	    /* Update the colormap */
	    ColormapPtr pmap = InstalledMaps[pScreen->myNum];
	    int entries;
	    Pixel ppix[256];
	    xrgb prgb[256];
	    xColorItem defs[256];
	    int i;

	    if (!pmap)
		pmap = (ColormapPtr) LookupIDByType(pScreen->defColormap,
						    RT_COLORMAP);
	    entries = pmap->pVisual->ColormapEntries;
	    if (entries) {
		for (i = 0 ; i < entries; i++)
		ppix[i] = i;
		QueryColors (pmap, entries, ppix, prgb);

		for (i = 0 ; i < entries; i++) {
		    defs[i].pixel = ppix[i];
		    defs[i].red = prgb[i].red;
		    defs[i].green = prgb[i].green;
		    defs[i].blue = prgb[i].blue;
		}

		fbdevStoreColors(pmap, entries, defs);
	    }
	}
    } else {
	/*
	 *  Create a dummy pixmap to write to while VT is switched out.
	 *  Copy the screen to that pixmap
	 */
	if (!xf86Exiting) {
	    ppix = (pScreen->CreatePixmap)(pScreen, pScreen->width,
	    				   pScreen->height, pScreen->rootDepth);
	    if (ppix) {
		(*fbdevBitBlt)(&pspix->drawable, &ppix->drawable, GXcopy,
			       &pixReg, &pixPt, ~0);
		pspix->devPrivate.ptr = ppix->devPrivate.ptr;
	    }
	}
	if(ioctl(fb_fd, VIOCREMOVE, 0))
	    FatalError("ioctl(fd, VIOCREMOVE ...)");

	UnMapVidMem(screen_idx, fb_fix.smem_len);
    }
}
 
/*
 *  fbdevCloseScreen -- Called to ensure video is enabled when server exits.
 */

static Bool fbdevCloseScreen(int screen_idx, ScreenPtr screen)
{
    /*
     *  Hmm... The server may shut down even if it is not running on the
     *  current vt. Let's catch this case here.
     */
    xf86Exiting = TRUE;
    if (xf86VTSema)
	fbdevEnterLeaveVT(LEAVE, screen_idx);
    else if (ppix) {
	/*
	 *  7-Jan-94 CEG: The server is not running on the current vt.
	 *  Free the screen snapshot taken when the server vt was left.
	 */
	(savepScreen->DestroyPixmap)(ppix);
	ppix = NULL;
    }
    return(TRUE);
}

/*
 *  fbdevValidMode -- Check whether a mode is valid. If necessary, values will
 *		      be rounded up by the Frame Buffer Device
 */

static Bool fbdevValidMode(DisplayModePtr mode)
{
    return(1);
}

/*
 *  fbdevSwitchMode -- Change the video mode `on the fly'
 */

static Bool fbdevSwitchMode(DisplayModePtr mode)
{
ErrorF("fbdevSwitchMode called\n");
#if 0
    struct view_size var = vs;
    ScreenPtr pScreen = savepScreen;
    BoxRec pixBox;
    RegionRec pixReg;
    DDXPointRec pixPt;
    PixmapPtr pspix, ppix;
    Bool res = FALSE;

    xfree2fbdev(mode, &var);
    var.x = fbdevInfoRec.frameX0;
    var.y = fbdevInfoRec.frameY0;

    /* Save the screen image */
    pixBox.x1 = 0; pixBox.x2 = pScreen->width;
    pixBox.y1 = 0; pixBox.y2 = pScreen->height;
    pixPt.x = 0; pixPt.y = 0;
    (pScreen->RegionInit)(&pixReg, &pixBox, 1);
    if (fbdevPrivateIndexP)
	pspix = (PixmapPtr)pScreen->devPrivates[*fbdevPrivateIndexP].ptr;
    else
	pspix = (PixmapPtr)pScreen->devPrivate;
    ppix = (pScreen->CreatePixmap)(pScreen, pScreen->width, pScreen->height,
    				   pScreen->rootDepth);
    if (ppix)
	(*fbdevBitBlt)(&pspix->drawable, &ppix->drawable, GXcopy, &pixReg,
		       &pixPt, ~0);

    if (!ioctl(fb_fd, VIOCSSIZE, &var)) {
	/* Restore the colormap */
	fbdevRestoreColors(pScreen);
	/* restore the screen image */
	if (ppix)
	    (*fbdevBitBlt)(&ppix->drawable, &pspix->drawable, GXcopy, &pixReg,
	    		   &pixPt, ~0);
	vs.width = var.width;
	vs.height = var.height;
	res = TRUE;
    }
    if (ppix)
	(pScreen->DestroyPixmap)(ppix);

    return(res);
#endif
    return(0);
}


/*
 *  Convert timings between the XFree style and the Frame Buffer Device style
 */

static void xfree2fbdev(DisplayModePtr mode, struct view_size *var)
{
    var->width = mode->HDisplay;
    var->height = mode->VDisplay;
    var->depth = fbdevInfoRec.bitsPerPixel;
    var->x = mode->HDisplay;
    var->y = mode->VDisplay;
}

static void fbdev2xfree(struct view_size *var, DisplayModePtr mode)
{
    mode->Clock = 0;
    mode->HDisplay = var->width;
    mode->HSyncStart = 0;
    mode->HSyncEnd = 0;
    mode->HTotal = 0;
    mode->VDisplay = var->height;
    mode->VSyncStart = 0;
    mode->VSyncEnd = 0;
    mode->VTotal = 0;
    mode->Flags = 0;
    mode->SynthClock = mode->Clock;
    mode->CrtcHDisplay = mode->HDisplay;
    mode->CrtcHSyncStart = mode->HSyncStart;
    mode->CrtcHSyncEnd = mode->HSyncEnd;
    mode->CrtcHTotal = mode->HTotal;
    mode->CrtcVDisplay = mode->VDisplay;
    mode->CrtcVSyncStart = mode->VSyncStart;
    mode->CrtcVSyncEnd = mode->VSyncEnd;
    mode->CrtcVTotal = mode->VTotal;
    mode->CrtcHAdjusted = FALSE;
    mode->CrtcVAdjusted = FALSE;
}
