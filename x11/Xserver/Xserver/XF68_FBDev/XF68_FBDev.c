/* $XConsortium: XF68_FBDev.c /main/1 1996/09/21 11:17:40 kaleb $ */




/* $XFree86: xc/programs/Xserver/hw/xfree68/common/XF68_FBDev.c,v 3.1 1996/12/27 06:52:00 dawes Exp $ */
#include "X.h"
#include "os.h"

#define _NO_XF86_PROTOTYPES

#include "xf86.h"
#include "xf86_Config.h"


extern ScrnInfoRec fbdevInfoRec;

ScrnInfoPtr xf86Screens[] = {
	&fbdevInfoRec,
};

int xf86MaxScreens = sizeof(xf86Screens)/sizeof(ScrnInfoPtr);

int xf86ScreenNames[] = {
	FBDEV,
	-1
};

int fbdevValidTokens[] = {
	STATICGRAY,
	GRAYSCALE,
	STATICCOLOR,
	PSEUDOCOLOR,
	TRUECOLOR,
	DIRECTCOLOR,
	MODES,
	VIEWPORT,
	VIRTUAL,
	-1
};

#include "xf86ExtInit.h"
