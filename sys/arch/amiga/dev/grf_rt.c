#include "grf.h"
#if NGRF > 0

/* Graphics routines for the Retina board, 
   using the NCR 77C22E+ VGA controller. */

#include "sys/param.h"
#include "sys/errno.h"
#include "grfioctl.h"
#include "grfvar.h"
#include "grf_rtreg.h"
#include "../include/cpu.h"
#include "device.h"

extern caddr_t ZORRO2ADDR;

/* NOTE: this driver for the MacroSystem Retina board was only possible,
         because MacroSystem provided information about the pecularities
         of the board. THANKS! Competition in Europe among gfx board 
         manufacturers is rather tough, so Lutz Vieweg, who wrote the
         initial driver, has made an agreement with MS not to document
         the driver source (see also his Copyright disclaimer below).
         -> ALL comments after 
	 -> "/* -------------- START OF CODE -------------- * /"
	 -> have been added by myself (mw) from studying the publically
	 -> available "NCR 77C22E+" Data Manual
	 
	 Lutz' original driver source (without any of my comments) is
	 available on request. */


/* This code offers low-level routines to access the Retina graphics-board
   manufactured by MS MacroSystem GmbH from within NetBSD for the Amiga.
   No warranties for any kind of function at all - this code may crash
   your hardware and scratch your harddisk.
   Use at your own risk.
   Freely distributable.
   
   Written by Lutz Vieweg 07/93
   
   Thanks to MacroSystem for providing me with the neccessary information
   to create theese routines. The sparse documentation of this code
   results from the agreements between MS and me.
*/

extern unsigned char kernel_font_width, kernel_font_height;
extern unsigned char kernel_font_lo, kernel_font_hi;
extern unsigned char kernel_font[];


#define MDF_DBL 1      
#define MDF_LACE 2     
#define MDF_CLKDIV2 4  


/* standard-palette definition */

unsigned char NCRStdPalette[16*3] = {
/*   R   G   B  */
	  0,  0,  0,
	192,192,192,
	128,  0,  0,  
	  0,128,  0,
	  0,  0,128,
	128,128,  0,  
	  0,128,128,  
	128,  0,128,
	 64, 64, 64, /* the higher 8 colors have more intensity for  */
	255,255,255, /* compatibility with standard attributes       */
	255,  0,  0,  
	  0,255,  0,
	  0,  0,255,
	255,255,  0,  
	  0,255,255,  
	255,  0,255  
};


/* The following structures are examples for monitor-definitions. To make one
   of your own, first use "DefineMonitor" and create the 8-bit monitor-mode of
   your dreams. Then save it, and make a structure from the values provided in
   the file DefineMonitor stored - the labels in the comment above the
   structure definition show where to put what value.
   
   Then you'll need to adapt your monitor-definition to the font you want to
   use. Be FX the width of the font, then the following modifications have to
   be applied to your values:
   
   HBS = (HBS * 4) / FX
   HSS = (HSS * 4) / FX
   HSE = (HSE * 4) / FX
   HBE = (HBE * 4) / FX
   HT  = (HT  * 4) / FX

   Make sure your maximum width (MW) and height (MH) are even multiples of
   the fonts' width and height.
*/

#if 0
/* horizontal 31.5 kHz */

/*                                      FQ     FLG    MW   MH   HBS HSS HSE HBE  HT  VBS  VSS  VSE  VBE   VT  */
   struct MonDef MON_640_512_60  = { 50000000,  28,  640, 512,   81, 86, 93, 98, 95, 513, 513, 521, 535, 535,
   /* Depth,           PAL, TX,  TY,    XY,FontX, FontY,    FontData,  FLo,  Fhi */
          4, NCRStdPalette, 80,  64,  5120,    8,     8, kernel_font,   32,  255};      

/* horizontal 38kHz */

   struct MonDef MON_768_600_60  = { 75000000,  28,  768, 600,   97, 99,107,120,117, 601, 615, 625, 638, 638,
          4, NCRStdPalette, 96,  75,  7200,    8,     8, kernel_font,   32,  255};      

/* horizontal 64kHz */

   struct MonDef MON_768_600_80  = { 50000000, 24,  768, 600,   97,104,112,122,119, 601, 606, 616, 628, 628,
          4, NCRStdPalette, 96,  75,  7200,    8,     8, kernel_font,   32,  255};      

#if 0
   struct MonDef MON_1024_768_80 = { 90000000, 24, 1024, 768,  129,130,141,172,169, 769, 770, 783, 804, 804,
          4, NCRStdPalette,128,  96, 12288,    8,     8, kernel_font,   32,  255};      
#else
   struct MonDef MON_1024_768_80 = { 90000000, 0, 1024, 768,  257,258,278,321,320, 769, 770, 783, 804, 804,
          4, NCRStdPalette,128,  96, 12288,    8,     8, kernel_font,   32,  255};      
#endif

   struct MonDef MON_1024_1024_59= { 90000000, 24, 1024,1024,  129,130,141,173,170,1025,1059,1076,1087,1087,
          4, NCRStdPalette,128, 128, 16384,    8,     8, kernel_font,   32,  255};      

/* WARNING: THE FOLLOWING MONITOR MODES EXCEED THE 90-MHz LIMIT THE PROCESSOR
            HAS BEEN SPECIFIED FOR. USE AT YOUR OWN RISK (AND THINK ABOUT
            MOUNTING SOME COOLING DEVICE AT THE PROCESSOR AND RAMDAC)!     */

   struct MonDef MON_1280_1024_60= {110000000,  24, 1280,1024,  161,162,176,211,208,1025,1026,1043,1073,1073,
          4, NCRStdPalette,160, 128, 20480,    8,     8, kernel_font,   32,  255};      

/* horizontal 75kHz */

   struct MonDef MON_1280_1024_69= {120000000,  24, 1280,1024,  161,162,175,200,197,1025,1026,1043,1073,1073, 
          4, NCRStdPalette,160, 128, 20480,    8,     8, kernel_font,   32,  255};      

#else

struct MonDef monitor_defs[] = {
/* horizontal 31.5 kHz */

   { 50000000,  28,  640, 512,   81, 86, 93, 98, 95, 513, 513, 521, 535, 535,
          4, NCRStdPalette, 80,  64,  5120,    8,     8, kernel_font,   32,  255},

/* horizontal 38kHz */

   { 75000000,  28,  768, 600,   97, 99,107,120,117, 601, 615, 625, 638, 638,
          4, NCRStdPalette, 96,  75,  7200,    8,     8, kernel_font,   32,  255},

/* horizontal 64kHz */

   { 50000000, 24,  768, 600,   97,104,112,122,119, 601, 606, 616, 628, 628,
          4, NCRStdPalette, 96,  75,  7200,    8,     8, kernel_font,   32,  255},



   { 90000000, 24, 1024, 768,  129,130,141,172,169, 769, 770, 783, 804, 804,
          4, NCRStdPalette,128,  96, 12288,    8,     8, kernel_font,   32,  255},

   { 100000000, 24, 1024, 768,  129,130,141,172,169, 769, 770, 783, 804, 804,
          4, NCRStdPalette,128,  96, 12288,    8,     8, kernel_font,   32,  255},

   { 110000000, 24, 1024, 768,  129,130,141,172,169, 769, 770, 783, 804, 804,
          4, NCRStdPalette,128,  96, 12288,    8,     8, kernel_font,   32,  255},

   { 120000000, 24, 1024, 768,  129,130,141,172,169, 769, 770, 783, 804, 804,
          4, NCRStdPalette,128,  96, 12288,    8,     8, kernel_font,   32,  255},

   { 13000000, 24, 1024, 768,  129,130,141,172,169, 769, 770, 783, 804, 804,
          4, NCRStdPalette,128,  96, 12288,    8,     8, kernel_font,   32,  255},

   { 72000000, 24, 1024, 768,  129,130,141,172,169, 769, 770, 783, 804, 804,
          4, NCRStdPalette,128,  96, 12288,    8,     8, kernel_font,   32,  255},

   { 75000000, 24, 1024, 768,  129,130,141,172,169, 769, 770, 783, 804, 804,
          4, NCRStdPalette,128,  96, 12288,    8,     8, kernel_font,   32,  255},




   { 90000000, 24, 1024, 768,  129,130,139,161,160, 769, 770, 783, 804, 804,
          4, NCRStdPalette,128,  96, 12288,    8,     8, kernel_font,   32,  255},

   { 90000000, 24, 1024,1024,  129,130,141,173,170,1025,1059,1076,1087,1087,
          4, NCRStdPalette,128, 128, 16384,    8,     8, kernel_font,   32,  255},

/* WARNING: THE FOLLOWING MONITOR MODES EXCEED THE 90-MHz LIMIT THE PROCESSOR
            HAS BEEN SPECIFIED FOR. USE AT YOUR OWN RISK (AND THINK ABOUT
            MOUNTING SOME COOLING DEVICE AT THE PROCESSOR AND RAMDAC)!     */

   {110000000,  24, 1280,1024,  161,162,176,211,208,1025,1026,1043,1073,1073,
          4, NCRStdPalette,160, 128, 20480,    8,     8, kernel_font,   32,  255},

/* horizontal 75kHz */

   {120000000,  24, 1280,1024,  161,162,175,200,197,1025,1026,1043,1073,1073, 
          4, NCRStdPalette,160, 128, 20480,    8,     8, kernel_font,   32,  255},

};

static const char *monitor_descr[] = {
  "80x64 (640x512) 31.5kHz",
  "96x75 (768x600) 38kHz",
  "96x75 (768x600) 64kHz",
  "128x96 (1024x768) 64kHz",
  "128x128 (1024x1024) 64kHz",
  "160x128 (1280x1024) 64kHz ***EXCEEDS CHIP LIMIT!!!***",
  "160x128 (1280x1024) 75kHz ***EXCEEDS CHIP LIMIT!!!***",
};

int retina_mon_max = sizeof (monitor_defs)/sizeof (monitor_defs[0]);

/* patchable */
int retina_default_mon = 2;
    
#endif


static struct MonDef *current_mon;

/* -------------- START OF CODE -------------- */


static const long FQTab[16] =
{ 25175000,  28322000,  36000000,  65000000,
  44900000,  50000000,  80000000,  75000000,
  56644000,  63000000,  72000000, 130000000,
  90000000, 100000000, 110000000, 120000000 };


/*--------------------------------------------------*/
/*--------------------------------------------------*/

#if 0
static struct MonDef *default_monitor = &DEFAULT_MONDEF;
#endif


static int rt_load_mon (struct grf_softc *gp, struct MonDef *md)
{
	struct grfinfo *gi = &gp->g_display;
	volatile unsigned char *ba;
	volatile unsigned char *fb;
	short FW, clksel, HDE, VDE;

	for (clksel = 15; clksel; clksel--) {
		if (FQTab[clksel] == md->FQ) break;
	}
	if (clksel < 0) return 0;
	
	ba = gp->g_regkva;;
	fb = gp->g_fbkva;
	
	switch (md->FX) {
	case 4:
		FW = 0;
		break;
	case 7:
		FW = 1;
		break;
	case 8:
		FW = 2;
		break;
	case 9:
		FW = 3;
		break;
	case 10:
		FW = 4;
		break;
	case 11:
		FW = 5;
		break;
	case 12:
		FW = 6;
		break;
	case 13:
		FW = 7;
		break;
	case 14:
		FW = 8;
		break;
	case 15:
		FW = 9;
		break;
	case 16:
		FW = 11;
		break;
	default:
		return 0;
		break;
	};
	
	HDE = (md->MW+md->FX-1)/md->FX;
	VDE = md->MH-1;
	
	/* hmm... */
	fb[0x8000] = 0;

		/* enable extension registers */
	WSeq (ba, SEQ_ID_EXTENDED_ENABLE,	0x05);

#if 0
	/* program the clock oscillator */	
	vgaw (ba, GREG_MISC_OUTPUT_W, 0xe3 | ((clksel & 3) * 0x04));
	vgaw (ba, GREG_FEATURE_CONTROL_W, 0x00);
	
	/* XXXX according to the NCR specs, this register should be set to 1
	   XXXX before doing the MISC_OUTPUT setting and CLOCKING_MODE
	   XXXX setting. */
	WSeq (ba, SEQ_ID_RESET, 		0x03);  

	WSeq (ba, SEQ_ID_CLOCKING_MODE, 	0x01 | ((md->FLG & MDF_CLKDIV2)/ MDF_CLKDIV2 * 8));  
	WSeq (ba, SEQ_ID_MAP_MASK, 		0x0f);  
	WSeq (ba, SEQ_ID_CHAR_MAP_SELECT, 	0x00);
		/* odd/even write select + extended memory */  
	WSeq (ba, SEQ_ID_MEMORY_MODE, 	0x06);
	/* XXXX I think this order of setting RESET is wrong... */
	WSeq (ba, SEQ_ID_RESET, 		0x01);  
	WSeq (ba, SEQ_ID_RESET, 		0x03);  
#else
	WSeq (ba, SEQ_ID_RESET, 		0x01);  

		/* set font width + rest of clocks */
	WSeq (ba, SEQ_ID_EXT_CLOCK_MODE,	0x30 | (FW & 0x0f) | ((clksel & 4) / 4 * 0x40) );  
		/* another clock bit, plus hw stuff */  
	WSeq (ba, SEQ_ID_MISC_FEATURE_SEL,	0xf4 | (clksel & 8) );  

	/* program the clock oscillator */	
	vgaw (ba, GREG_MISC_OUTPUT_W, 		0xe3 | ((clksel & 3) * 0x04));
	vgaw (ba, GREG_FEATURE_CONTROL_W, 	0x00);
	
	WSeq (ba, SEQ_ID_CLOCKING_MODE, 	0x01 | ((md->FLG & MDF_CLKDIV2)/ MDF_CLKDIV2 * 8));  
	WSeq (ba, SEQ_ID_MAP_MASK, 		0x0f);  
	WSeq (ba, SEQ_ID_CHAR_MAP_SELECT, 	0x00);
		/* odd/even write select + extended memory */  
	WSeq (ba, SEQ_ID_MEMORY_MODE, 		0x06);
	WSeq (ba, SEQ_ID_RESET, 		0x03);  
#endif
	
		/* monochrome cursor */
	WSeq (ba, SEQ_ID_CURSOR_CONTROL,	0x00);  
		/* bank0 */
	WSeq (ba, SEQ_ID_PRIM_HOST_OFF_HI,	0x00);  
	WSeq (ba, SEQ_ID_PRIM_HOST_OFF_LO,	0x00);  
	WSeq (ba, 0x1a , 0x00);  /* these are reserved, really set them to 0 ??? */
	WSeq (ba, 0x1b , 0x00);  /* these are reserved, really set them to 0 ??? */
		/* bank0 */
	WSeq (ba, SEQ_ID_SEC_HOST_OFF_HI,	0x00);  
	WSeq (ba, SEQ_ID_SEC_HOST_OFF_LO,	0x00);  
		/* 1M-chips + ena SEC + ena EMem + rw PrimA0/rw Sec/B0 */
	WSeq (ba, SEQ_ID_EXTENDED_MEM_ENA,	0x3 | 0x4 | 0x10 | 0x40);
#if 0
		/* set font width + rest of clocks */
	WSeq (ba, SEQ_ID_EXT_CLOCK_MODE,	0x30 | (FW & 0x0f) | ((clksel & 4) / 4 * 0x40) );  
#endif
		/* no ext-chain4 + no host-addr-bit-16 */
	WSeq (ba, SEQ_ID_EXT_VIDEO_ADDR,	0x00);  
		/* no packed/nibble + no 256bit gfx format */
	WSeq (ba, SEQ_ID_EXT_PIXEL_CNTL,	0x00);
		/* AT-interface */
	WSeq (ba, SEQ_ID_BUS_WIDTH_FEEDB,	0x06);  
		/* see fg/bg color expansion */
	WSeq (ba, SEQ_ID_COLOR_EXP_WFG,		0x01); 
	WSeq (ba, SEQ_ID_COLOR_EXP_WBG,		0x00);
	WSeq (ba, SEQ_ID_EXT_RW_CONTROL,	0x00);
#if 0
		/* another clock bit, plus hw stuff */  
	WSeq (ba, SEQ_ID_MISC_FEATURE_SEL,	0xf4 | (clksel & 8) );  
#endif
		/* don't tristate PCLK and PIX */
	WSeq (ba, SEQ_ID_COLOR_KEY_CNTL,	0x40 ); 
		/* reset CRC circuit */
	WSeq (ba, SEQ_ID_CRC_CONTROL,		0x00 ); 
		/* set RAS/CAS swap */
	WSeq (ba, SEQ_ID_PERF_SELECT,		0x20);  
	
	WCrt (ba, CRT_ID_END_VER_RETR,		(md->VSE & 0xf ) | 0x20); 
	WCrt (ba, CRT_ID_HOR_TOTAL,		md->HT   & 0xff);
	WCrt (ba, CRT_ID_HOR_DISP_ENA_END,	(HDE-1)  & 0xff);
	WCrt (ba, CRT_ID_START_HOR_BLANK,	md->HBS  & 0xff);
	WCrt (ba, CRT_ID_END_HOR_BLANK,		(md->HBE & 0x1f) | 0x80);
	
	WCrt (ba, CRT_ID_START_HOR_RETR,	md->HSS  & 0xff);
	WCrt (ba, CRT_ID_END_HOR_RETR,		(md->HSE & 0x1f) | ((md->HBE & 0x20)/ 0x20 * 0x80));
	WCrt (ba, CRT_ID_VER_TOTAL,		(md->VT  & 0xff));
	WCrt (ba, CRT_ID_OVERFLOW,		(( (md->VSS  & 0x200) / 0x200 * 0x80) 
						 | ((VDE     & 0x200) / 0x200 * 0x40) 
						 | ((md->VT  & 0x200) / 0x200 * 0x20)
						 | 				0x10
						 | ((md->VBS & 0x100) / 0x100 * 8   )
						 | ((md->VSS & 0x100) / 0x100 * 4   ) 
						 | ((VDE     & 0x100) / 0x100 * 2   ) 
						 | ((md->VT  & 0x100) / 0x100       )));
	WCrt (ba, CRT_ID_PRESET_ROW_SCAN,	0x00);
	
	WCrt (ba, CRT_ID_MAX_SCAN_LINE, 	((  (md->FLG & MDF_DBL)/ MDF_DBL * 0x80) 
						 | 				   0x40 
						 | ((md->VBS & 0x200)/0x200	 * 0x20) 
						 | ((md->FY-1) 			 & 0x1f)));
	
	WCrt (ba, CRT_ID_CURSOR_START,		(md->FY & 0x1f) - 2);
	WCrt (ba, CRT_ID_CURSOR_END,		(md->FY & 0x1f) - 1);
	
	WCrt (ba, CRT_ID_START_ADDR_HIGH,	0x00);
	WCrt (ba, CRT_ID_START_ADDR_LOW,	0x00);
	
	WCrt (ba, CRT_ID_CURSOR_LOC_HIGH,	0x00);
	WCrt (ba, CRT_ID_CURSOR_LOC_LOW,	0x00);
	
	WCrt (ba, CRT_ID_START_VER_RETR,	md->VSS    & 0xff);
	WCrt (ba, CRT_ID_END_VER_RETR,		(md->VSE   & 0x0f) | 0x80 | 0x20); 
	WCrt (ba, CRT_ID_VER_DISP_ENA_END,	VDE        & 0xff);
	WCrt (ba, CRT_ID_OFFSET,		(HDE / 2)  & 0xff);
	WCrt (ba, CRT_ID_UNDERLINE_LOC,		(md->FY-1) & 0x1f);
	WCrt (ba, CRT_ID_START_VER_BLANK,	md->VBS    & 0xff);
	WCrt (ba, CRT_ID_END_VER_BLANK,		md->VBE    & 0xff);
		/* byte mode + wrap + select row scan counter + cms */
	WCrt (ba, CRT_ID_MODE_CONTROL,		0xe3); 
	WCrt (ba, CRT_ID_LINE_COMPARE,		0xff); 
	
		/* enable extended end bits + those bits */
	WCrt (ba, CRT_ID_EXT_HOR_TIMING1,	(           				 0x20 
						 | ((md->FLG & MDF_LACE)  / MDF_LACE   * 0x10) 
						 | ((md->HT  & 0x100) / 0x100          * 0x01) 
						 | (((HDE-1) & 0x100) / 0x100 	       * 0x02) 
						 | ((md->HBS & 0x100) / 0x100 	       * 0x04) 
						 | ((md->HSS & 0x100) / 0x100 	       * 0x08)));
	             
	WCrt (ba, CRT_ID_EXT_START_ADDR,	(((HDE / 2) & 0x100)/0x100 * 16)); 
	
	WCrt (ba, CRT_ID_EXT_HOR_TIMING2, 	(  ((md->HT  & 0x200)/ 0x200  	       * 0x01) 
						 | (((HDE-1) & 0x200)/ 0x200 	       * 0x02) 
						 | ((md->HBS & 0x200)/ 0x200 	       * 0x04)
						 | ((md->HSS & 0x200)/ 0x200 	       * 0x08) 
						 | ((md->HBE & 0xc0) / 0x40  	       * 0x10)
						 | ((md->HSE & 0x60) / 0x20  	       * 0x40)));
	             
	WCrt (ba, CRT_ID_EXT_VER_TIMING,	(  ((md->VSE & 0x10) / 0x10  	       * 0x80) 
						 | ((md->VBE & 0x300)/ 0x100 	       * 0x20) 
						 |				         0x10 
						 | ((md->VSS & 0x400)/ 0x400 	       * 0x08) 
						 | ((md->VBS & 0x400)/ 0x400 	       * 0x04) 
						 | ((VDE     & 0x400)/ 0x400 	       * 0x02) 
						 | ((md->VT  & 0x400)/ 0x400           * 0x01)));
	 
	WGfx (ba, GCT_ID_SET_RESET,		0x00); 
	WGfx (ba, GCT_ID_ENABLE_SET_RESET,	0x00); 
	WGfx (ba, GCT_ID_COLOR_COMPARE,		0x00); 
	WGfx (ba, GCT_ID_DATA_ROTATE,		0x00); 
	WGfx (ba, GCT_ID_READ_MAP_SELECT,	0x00); 
	WGfx (ba, GCT_ID_GRAPHICS_MODE,		0x00); 
	WGfx (ba, GCT_ID_MISC,			0x04);
	WGfx (ba, GCT_ID_COLOR_XCARE,		0xff); 
	WGfx (ba, GCT_ID_BITMASK,		0xff); 
	
	/* reset the Attribute Controller flipflop */
	vgar (ba, GREG_STATUS1_R);
	WAttr (ba, ACT_ID_PALETTE0,		0x00);     
	WAttr (ba, ACT_ID_PALETTE1,		0x01);  
	WAttr (ba, ACT_ID_PALETTE2,		0x02);  
	WAttr (ba, ACT_ID_PALETTE3,		0x03);  
	WAttr (ba, ACT_ID_PALETTE4,		0x04);  
	WAttr (ba, ACT_ID_PALETTE5,		0x05);  
	WAttr (ba, ACT_ID_PALETTE6,		0x06);  
	WAttr (ba, ACT_ID_PALETTE7,		0x07);  
	WAttr (ba, ACT_ID_PALETTE8,		0x08);  
	WAttr (ba, ACT_ID_PALETTE9,		0x09);  
	WAttr (ba, ACT_ID_PALETTE10,		0x0a);  
	WAttr (ba, ACT_ID_PALETTE11,		0x0b);  
	WAttr (ba, ACT_ID_PALETTE12,		0x0c);  
	WAttr (ba, ACT_ID_PALETTE13,		0x0d);  
	WAttr (ba, ACT_ID_PALETTE14,		0x0e);  
	WAttr (ba, ACT_ID_PALETTE15,		0x0f);  
	
	vgar (ba, GREG_STATUS1_R);
	WAttr (ba, ACT_ID_ATTR_MODE_CNTL,	0x08);  
	
	WAttr (ba, ACT_ID_OVERSCAN_COLOR,	0x00);  
	WAttr (ba, ACT_ID_COLOR_PLANE_ENA,	0x0f);  
	WAttr (ba, ACT_ID_HOR_PEL_PANNING,	0x00);  
	WAttr (ba, ACT_ID_COLOR_SELECT,	0x00);  
	
	vgar (ba, GREG_STATUS1_R);
		/* I have *NO* idea what strobing reg-0x20 might do... */
	vgaw (ba, ACT_ADDRESS_W, 0x20); 
	
	WCrt (ba, CRT_ID_MAX_SCAN_LINE,		( ((md->FLG & MDF_DBL)/ MDF_DBL * 0x80) 
						|	                          0x40 
						| ((md->VBS & 0x200)/0x200	* 0x20) 
						| ((md->FY-1) 			& 0x1f))); 
	

	/* not it's time for guessing... */

	vgaw (ba, VDAC_REG_D, 	   0x02); 
	
		/* if this does what I think it does, it selects DAC 
		   register 0, and writes the palette in subsequent
		   registers, thus it works similar to the WD33C93 
		   select/data mechanism */
	vgaw (ba, VDAC_REG_SELECT, 0x00);
	
	{ 
		
		short x = 15;
		const unsigned char * col = md->PAL;
		do {
			
			vgaw (ba, VDAC_REG_DATA, *col++);
			vgaw (ba, VDAC_REG_DATA, *col++);
			vgaw (ba, VDAC_REG_DATA, *col++);
			
			
		} while (x--);
		
	}


	/* now load the font into maps 2 (and 3 for fonts wider than 8 pixels) */
	{
		
		/* first set the whole font memory to a test-pattern, so we 
		   can see if something that shouldn't be drawn IS drawn.. */
		{
			unsigned char * c = fb;
			long x;
			Map(2);
			
			for (x = 0; x < 65536; x++) {
				*c++ = (x & 1)? 0xaa : 0x55;
			}
		}
		
		{
			unsigned char * c = fb;
			long x;
			Map(3);
			
			for (x = 0; x < 65536; x++) {
				*c++ = (x & 1)? 0xaa : 0x55;
			}
		}
		
		{
		  /* ok, now position at first defined character, and
		     copy over the images */
		  unsigned char * c = fb + md->FLo * 32;
		  const unsigned char * f = md->FData;
		  unsigned short z;
		
		  Map(2);
		  for (z = md->FLo; z <= md->FHi; z++) {
			
			short y = md->FY-1;
			if (md->FX > 8){
				do {
					*c++ = *f;
					f += 2;
				} while (y--);
			}
			else {
				do {
					*c++ = *f++;
				} while (y--);
			}
			
			c += 32-md->FY;
			
		  }
		
		  if (md->FX > 8) {
			unsigned short z;
		
			Map(3);
			c = fb + md->FLo*32;
			f = md->FData+1;
			for (z = md->FLo; z <= md->FHi; z++) {
				
				short y = md->FY-1;
				do {
					*c++ = *f;
					f += 2;
				} while (y--);
				
				c += 32-md->FY;
				
			}
		  }
		}
		
	}
	
		/* select map 0 */
	WGfx (ba, GCT_ID_READ_MAP_SELECT,	0);
		/* allow writes into maps 0 and 1 */
	WSeq (ba, SEQ_ID_MAP_MASK,		3);
		/* select extended chain4 addressing:
		    !A0/!A1	map 0	character to be displayed
		    !A1/ A1	map 1	attribute of that character
		     A0/!A1	map 2	not used (masked out, ignored)
		     A0/ A1 	map 3	not used (masked out, ignored) */
	WSeq (ba, SEQ_ID_EXT_VIDEO_ADDR,	RSeq(ba, SEQ_ID_EXT_VIDEO_ADDR) | 0x02);
	
	{
		/* position in display memory */
		unsigned short * c = (unsigned short *) fb;
		
		/* fill with blank, white on black */
		const unsigned short fill_val = 0x2010;
		short x = md->XY;
		do {
			*c = fill_val;
			c += 2;
		} while (x--);
		
		/* I won't comment this :-)) */
		c = (unsigned short *) fb;
		c += (md->TX-6)*2;
		{
		  unsigned short init_msg[6] = {0x520a, 0x450b, 0x540c, 0x490d, 0x4e0e, 0x410f};
		  unsigned short * f = init_msg;
		  x = 5;
		  do {
			*c = *f++;
			c += 2;
	 	  } while (x--);
	 	}
		
		
	}
	

	gi->gd_regaddr  = (caddr_t) md; /* XXX */
	gi->gd_regsize  = 0;

	gi->gd_fbaddr   = (long)fb - (long)ZORRO2ADDR + (long)ZORRO2BASE;
	gi->gd_fbsize   = 64*1024;	/* larger, but that's whats mappable */
  
	gi->gd_colors   = 1 << md->DEP;
	gi->gd_planes   = md->DEP;
  
	gi->gd_fbwidth  = md->MW;
	gi->gd_fbheight = md->MH;
	gi->gd_fbx	= 0;
	gi->gd_fby	= 0;
	gi->gd_dwidth   = md->TX * md->FX;
	gi->gd_dheight  = md->TY * md->FY;
	gi->gd_dx	= 0;
	gi->gd_dy	= 0;
  
	/* initialized, works, return 1 */
	return 1;
}

int rt_init (struct grf_softc *gp, struct amiga_device *ad, struct amiga_hw *ahw)
{
  /* if already initialized, fail */
  if (gp->g_regkva)
    return 0;

  gp->g_regkva = ahw->hw_kva;
  gp->g_fbkva  = ahw->hw_kva + 64*1024;
  
  /* don't let them patch it out of bounds */
  if ((unsigned)retina_default_mon >= retina_mon_max)
    retina_default_mon = 0;

  current_mon = monitor_defs + retina_default_mon;

  return rt_load_mon (gp, current_mon);
}

static int 
rt_getvmode (gp, vm)
     struct grf_softc *gp;
     struct grfvideo_mode *vm;
{
  struct MonDef *md;

  if (vm->mode_num && vm->mode_num > retina_mon_max)
    return EINVAL;

  if (! vm->mode_num)
    vm->mode_num = (current_mon - monitor_defs) + 1;

  md = monitor_defs + (vm->mode_num - 1);
  strncpy (vm->mode_descr, monitor_descr + (vm->mode_num - 1), 
	   sizeof (vm->mode_descr));
  vm->pixel_clock  = md->FQ;
  vm->disp_width   = md->MW;
  vm->disp_height  = md->MH;
  vm->depth        = md->DEP;
  vm->hblank_start = md->HBS;
  vm->hblank_stop  = md->HBE;
  vm->hsync_start  = md->HSS;
  vm->hsync_stop   = md->HSE;
  vm->htotal       = md->HT;
  vm->vblank_start = md->VBS;
  vm->vblank_stop  = md->VBE;
  vm->vsync_start  = md->VSS;
  vm->vsync_stop   = md->VSE;
  vm->vtotal       = md->VT;

  return 0;
}


static int 
rt_setvmode (gp, mode)
     struct grf_softc *gp;
     unsigned mode;
{
  struct MonDef *md;

  if (!mode || mode > retina_mon_max)
    return EINVAL;

  current_mon = monitor_defs + (mode - 1);
  return rt_load_mon (gp, current_mon) ? 0 : EINVAL;
}


/*
 * Change the mode of the display.
 * Right now all we can do is grfon/grfoff.
 * Return a UNIX error number or 0 for success.
 */
rt_mode(gp, cmd, arg)
	register struct grf_softc *gp;
	int cmd;
	void *arg;
{
  /* implement these later... */

  switch (cmd)
    {
    case GM_GRFON:
      return 0;
      
    case GM_GRFOFF:
      return 0;
      
    case GM_GRFCONFIG:
      return 0;

    case GM_GRFGETVMODE:
      return rt_getvmode (gp, (struct grfvideo_mode *) arg);

    case GM_GRFSETVMODE:
      return rt_setvmode (gp, *(unsigned *) arg);

    case GM_GRFGETNUMVM:
      *(int *)arg = retina_mon_max;
      return 0;

    default:
      break;
    }
    
  return EINVAL;
}

#endif	/* NGRF */
