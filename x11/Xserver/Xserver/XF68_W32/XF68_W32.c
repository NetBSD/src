/* $XFree86: xc/programs/Xserver/hw/xfree86/common/XF86_W32.c,v 3.2 1996/02/04 09:05:58 dawes Exp $ */





/* $XConsortium: XF86_W32.c /main/3 1995/11/12 19:20:57 kaleb $ */

#include "X.h"
#include "os.h"

#define _NO_XF86_PROTOTYPES

#include "xf86.h"
#include "xf86_Config.h"

extern ScrnInfoRec vga256InfoRec;

ScrnInfoPtr xf86Screens[] = 
{
  &vga256InfoRec,
};

int  xf86MaxScreens = sizeof(xf86Screens) / sizeof(ScrnInfoPtr);

int xf86ScreenNames[] =
{
  ACCEL,
  -1
};

int vga256ValidTokens[] =
{
  STATICGRAY,
  GRAYSCALE,
  STATICCOLOR,
  PSEUDOCOLOR,
  TRUECOLOR,
  DIRECTCOLOR,
  CHIPSET,
  CLOCKS,
  DISPLAYSIZE,
  MODES,
  OPTION,
  VIDEORAM,
  VIEWPORT,
  VIRTUAL,
  SPEEDUP,
  NOSPEEDUP,
  CLOCKPROG,
  IOBASE,
  MEMBASE,
  -1
};

#include "xf86ExtInit.h"
