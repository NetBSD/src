/*	$NetBSD: main.c,v 1.61.54.1 2008/01/02 21:47:59 bouyer Exp $	*/

/*-
 * Copyright (c) 1999, 2000 Shin Takemura.
 * All rights reserved.
 *
 * This software is part of the PocketBSD.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include <pbsdboot.h>
#include <commctrl.h>
#include <res/resource.h>

/*
 * If you modify this program and update pbsdboot.uu,
 * change version string which is coded in main.c
 * appropriately.
 * 
 * The version string is in format:
 *
 *   Version A.B.C YYYY.MM.DD
 *
 * in where:
 *
 *   A: Don't change this.
 *   B: Increment this number if you change program's behavior,
 *      fix some bugs or add new features.
 *   C: Increment this number if you change/add some
 *      parameters, constants, windows' resources.
 *   YYYY.MM.DD: date
 */
TCHAR *version_string = 
	TEXT("PocketBSD boot loader\r\n")
	TEXT("Version 1.17.5 2001.05.05\r\n")
#if ( _WIN32_WCE < 200 )
	TEXT("Compiled for WinCE 1.01\r\n")
#else
	TEXT("Compiled for WinCE 2.00\r\n")
#endif
	TEXT("\r\n")
	TEXT("Copyright(C) 1999 Shin Takemura,\r\n")
	TEXT("All rights reserved.\r\n")
	TEXT("\r\n")
	TEXT("http://www.NetBSD.org/ports/hpcmips/\r\n");

/*-----------------------------------------------------------------------------

  type difinitions

-----------------------------------------------------------------------------*/
enum {
	UPDATE_DLGBOX,
	UPDATE_DATA,
};

struct fb_type {
	int type;
	TCHAR *name;
};



struct fb_setting {
	TCHAR *name;
	int type;
	int width, height, linebytes;
	long addr;
	unsigned long platid_cpu, platid_machine;
};

/*-----------------------------------------------------------------------------

  variable declarations

-----------------------------------------------------------------------------*/
HINSTANCE  hInst = NULL;
HWND		hDlgMain;
HWND		hBack;
HWND		hWndCB = NULL;
HWND		hDlgLoad = NULL;
unsigned int	dlgStatus;
int		user_define_idx;
int		osversion;
BOOL booting = FALSE;
int how_long_to_boot = -1;

/*-----------------------------------------------------------------------------

  data

-----------------------------------------------------------------------------*/
TCHAR szAppName[ ] = TEXT("PocketBSD boot");
TCHAR szTitle[ ]   = TEXT("Welcome to PocketBSD!");
int errno;

/*
 * Wince_conf  identify executable binary file.
 */
#if ( _WIN32_WCE < 200 )
static char *wince_conf = "Compiled for WinCE 1.01";
#else
static char *wince_conf = "Compiled for WinCE 2.00";
#endif

#define IDD_TIMER 300

struct fb_type fb_types[] = {
	{ BIFB_D2_M2L_3,	TEXT(BIFBN_D2_M2L_3)	},
	{ BIFB_D2_M2L_3x2,	TEXT(BIFBN_D2_M2L_3x2)	},
	{ BIFB_D2_M2L_0,	TEXT(BIFBN_D2_M2L_0)	},
	{ BIFB_D2_M2L_0x2,	TEXT(BIFBN_D2_M2L_0x2)	},
	{ BIFB_D4_M2L_F,	TEXT(BIFBN_D4_M2L_F)	},
	{ BIFB_D4_M2L_Fx2,	TEXT(BIFBN_D4_M2L_Fx2)	},
	{ BIFB_D4_M2L_0,	TEXT(BIFBN_D4_M2L_0)	},
	{ BIFB_D4_M2L_0x2,	TEXT(BIFBN_D4_M2L_0x2)	},
	{ BIFB_D8_00,		TEXT(BIFBN_D8_00)	},
	{ BIFB_D8_FF,		TEXT(BIFBN_D8_FF)	},
	{ BIFB_D16_0000,	TEXT(BIFBN_D16_0000)	},
	{ BIFB_D16_FFFF,	TEXT(BIFBN_D16_FFFF)	},
};

int fb_size[] = {
	160, 240, 320, 400, 480, 600, 640,
	768, 800, 1024, 1150, 1280, 1600
};

int boot_times[] = {
	30,25,20,15,10,5
};

int fb_bpl[] = {
	40, 80, 128, 160, 240, 256, 320,
	384, 400, 480, 512, 600, 640, 768, 800, 1024, 1150, 1280, 1600
};

struct fb_setting fb_settings[] = {
	/*
	 * You must choose fb_type to make the screen looks like:
	 *   black-on-white on monochrome or gray scale screen
	 *   white-on-black on color screen
	 * without 'reverse video' on the properties daialog.
	 *
	 * 'black-on-white' means that Foreground color is black and 
	 * background color is white.
	 */
	{ NULL, BIFB_D2_M2L_3,
		320, 240, 80, 0xa000000,
		PLATID_UNKNOWN, PLATID_UNKNOWN },
	{ TEXT("FreeStyle"), BIFB_D2_M2L_3,
		320, 240, 80, 0xa000000,
		PLATID_CPU_MIPS_VR_41XX, PLATID_MACH_EVEREX_FREESTYLE_AXX },
	{ TEXT("FreeStyle(Small Font)"), BIFB_D2_M2L_3x2,
		640, 240, 80, 0xa000000,
		PLATID_CPU_MIPS_VR_41XX, PLATID_MACH_EVEREX_FREESTYLE_AXX },
	{ TEXT("MobileGear MC-CS11"), BIFB_D2_M2L_0,
		480, 240, 256, 0xa000000,
		PLATID_CPU_MIPS_VR_4102, PLATID_MACH_NEC_MCCS_11 },
	{ TEXT("MobileGear MC-CS12"), BIFB_D2_M2L_0,
		480, 240, 256, 0xa000000,
		PLATID_CPU_MIPS_VR_4102, PLATID_MACH_NEC_MCCS_12 },
	{ TEXT("MobileGear MC-CS13"), BIFB_D2_M2L_0,
		480, 240, 256, 0xa000000,
		PLATID_CPU_MIPS_VR_4102, PLATID_MACH_NEC_MCCS_13 },
	{ TEXT("Mobile Pro 700"), BIFB_D2_M2L_0,
		640, 240, 256, 0xa000000,
		PLATID_CPU_MIPS_VR_4102, PLATID_MACH_NEC_MCR_MPRO700 },
	{ TEXT("MobileGearII MC-R300"), BIFB_D2_M2L_0,
		640, 240, 256, 0xa000000,
		PLATID_CPU_MIPS_VR_4111, PLATID_MACH_NEC_MCR_300 },
	{ TEXT("MobileGearII for DoCoMo"), BIFB_D2_M2L_0,
		640, 240, 256, 0xa000000,
		PLATID_CPU_MIPS_VR_4111, PLATID_MACH_NEC_MCR_FORDOCOMO },
	{ TEXT("MobileGearII MC-R320"), BIFB_D2_M2L_0,
		640, 240, 160, 0xa000000,
		PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_320 },
	{ TEXT("MobileGearII MC/R330"), BIFB_D2_M2L_0,
		640, 240, 160, 0xa000000,
		PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_330 },
	{ TEXT("MobileGearII MC/R430"), BIFB_D16_0000,
		640, 240, 1280, 0xa180100,
		PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_430 },
	{ TEXT("MobileGearII MC-R500"), BIFB_D8_00,
		640, 240, 1024, 0x13000000,
		PLATID_CPU_MIPS_VR_4111, PLATID_MACH_NEC_MCR_500 },
	{ TEXT("Mobile Pro 750c"), BIFB_D8_00,
		640, 240, 1024, 0x13000000,
		PLATID_CPU_MIPS_VR_4111, PLATID_MACH_NEC_MCR_500A },
	{ TEXT("MobileGearII MC-R510"), BIFB_D8_00,
		640, 240, 1024, 0xa000000,
		PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_510 },
	{ TEXT("NEC MC-R510(15bit color)"), BIFB_D16_0000,
		640, 240, 1600, 0xa000000,
		PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_510 },
	{ TEXT("MobileGearII MC-R520"), BIFB_D16_0000,
		640, 240, 1600, 0xa000000,
		PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_520 },
	{ TEXT("NEC MC/R530(256 colors)"), BIFB_D8_00,
		640, 240, 640, 0xa1d4c00,
		PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_530 },
	{ TEXT("MobileGearII MC/R530"), BIFB_D16_0000,
		640, 240, 1280, 0xa180100,
		PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_530 },
	{ TEXT("MobileGearII MC/R550"), BIFB_D16_0000,
		640, 240, 1280, 0xa180100,
		PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_530 },
	{ TEXT("DoCoMo sigmarion"), BIFB_D16_0000,
		640, 240, 1280, 0xa000000,
		PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_SIGMARION },
	{ TEXT("Mobile Pro 770"), BIFB_D16_0000,
		640, 240, 1600, 0xa000000,
		PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_520A },
	{ TEXT("Mobile Pro 780"), BIFB_D16_0000,
		640, 240, 1280, 0xa180100,
		PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_530A },
	{ TEXT("MobileGearII MC-R700"), BIFB_D16_0000,
		800, 600, 1600, 0xa000000,
		PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_700 },
	{ TEXT("Mobile Pro 800"), BIFB_D16_0000,
		800, 600, 1600, 0xa000000,
		PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_700A },
	{ TEXT("MobileGearII MC/R730"), BIFB_D16_0000,
		800, 600, 1600, 0xa0ea600,
		PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_730 },
	{ TEXT("Mobile Pro 880"), BIFB_D16_0000,
		800, 600, 1600, 0xa0ea600,
		PLATID_CPU_MIPS_VR_4121, PLATID_MACH_NEC_MCR_730A },
	{ TEXT("Tripad PV-6000"), BIFB_D8_00,
		640, 480, 640, 0xa000000,
		PLATID_CPU_MIPS_VR_4111, PLATID_MACH_SHARP_TRIPAD_PV6000 },
	{ TEXT("Vadem Clio C-1000"), BIFB_D8_00,
		640, 480, 640, 0xa000000,
		PLATID_CPU_MIPS_VR_4111, PLATID_MACH_SHARP_TRIPAD_PV6000 },
	{ TEXT("Vadem Clio C-1050"), BIFB_D16_FFFF,
		640, 480, 1280, 0xa200000,
		PLATID_CPU_MIPS_VR_4121, PLATID_MACH_SHARP_TRIPAD_PV6000 },
	{ TEXT("E-55"), BIFB_D2_M2L_0,
		240, 320, 256, 0xa000000,
		PLATID_CPU_MIPS_VR_4111, PLATID_MACH_CASIO_CASSIOPEIAE_E55 },
	{ TEXT("E-55(Small Font)"), BIFB_D2_M2L_0x2,
		480, 320, 256, 0xa000000,
		PLATID_CPU_MIPS_VR_4111, PLATID_MACH_CASIO_CASSIOPEIAE_E55 },
	{ TEXT("E-100"), BIFB_D16_FFFF,
		240, 320, 512, 0xa200000,
		PLATID_CPU_MIPS_VR_4121, PLATID_MACH_CASIO_CASSIOPEIAE_E100 },
	{ TEXT("E-500"), BIFB_D16_FFFF,
		240, 320, 512, 0xa200000,
		PLATID_CPU_MIPS_VR_4121, PLATID_MACH_CASIO_CASSIOPEIAE_E500 },
	{ TEXT("PocketPostPet"), BIFB_D16_FFFF,
		320, 240, 1024, 0xa200000,
		PLATID_CPU_MIPS_VR_4121, PLATID_MACH_CASIO_POCKETPOSTPET_POCKETPOSTPET },
	{ TEXT("INTERTOP CX300"), BIFB_D8_00,
		640, 480, 640, 0xa000000,
		PLATID_CPU_MIPS_VR_4121, PLATID_MACH_FUJITSU_INTERTOP_IT300 },
	{ TEXT("INTERTOP CX300(16bpp)"), BIFB_D16_0000,
		640, 480, 1280, 0xa000000,
		PLATID_CPU_MIPS_VR_4121, PLATID_MACH_FUJITSU_INTERTOP_IT300 },
	{ TEXT("INTERTOP CX310"), BIFB_D8_00,
		640, 480, 640, 0xa000000,
		PLATID_CPU_MIPS_VR_4121, PLATID_MACH_FUJITSU_INTERTOP_IT310 },
	{ TEXT("PenCentra 130"), BIFB_D8_00,
		640, 480, 640, 0x10201e00,
		PLATID_CPU_MIPS_VR_4121, PLATID_MACH_FUJITSU_PENCENTRA_130 },
	{ TEXT("IBM WorkPad z50"), BIFB_D16_0000,
		640, 480, 1280, 0xa000000,
		PLATID_CPU_MIPS_VR_4121, PLATID_MACH_IBM_WORKPAD_26011AU },
	{ TEXT("Philips Nino 312"), BIFB_D2_M2L_0,
		240, 320, 0, 0,
		PLATID_CPU_MIPS_TX_3912, PLATID_MACH_PHILIPS_NINO_312 },
	{ TEXT("Compaq C-series 810"), BIFB_D2_M2L_0,
		640, 240, 0, 0,
		PLATID_CPU_MIPS_TX_3912, PLATID_MACH_COMPAQ_C_810 },
	{ TEXT("Compaq C-series 2010c"), BIFB_D8_00,
		640, 240, 0, 0,
		PLATID_CPU_MIPS_TX_3912, PLATID_MACH_COMPAQ_C_2010 },
	{ TEXT("Compaq C-series 2015c"), BIFB_D8_00,
		640, 240, 0, 0,
		PLATID_CPU_MIPS_TX_3912, PLATID_MACH_COMPAQ_C_2015 },
	{ TEXT("Compaq PRESARIO 213"), BIFB_D8_00,
		320, 240, 0, 0,
		PLATID_CPU_MIPS_VR_4111, PLATID_MACH_COMPAQ_PRESARIO_213 },
	{ TEXT("Compaq Aero 1530"), BIFB_D4_M2L_F,
		320, 240, 160, 0x0a000000,
		PLATID_CPU_MIPS_VR_4111, PLATID_MACH_COMPAQ_AERO_1530 },
	{ TEXT("Aero1530(Small Font)"), BIFB_D4_M2L_Fx2,
		640, 240, 160, 0x0a000000,
		PLATID_CPU_MIPS_VR_4111, PLATID_MACH_COMPAQ_AERO_1530 },
	{ TEXT("Victor InterLink MP-C101"), BIFB_D16_0000,
		640, 480, 0, 0,
		PLATID_CPU_MIPS_TX_3922, PLATID_MACH_VICTOR_INTERLINK_MPC101},
	{ TEXT("Sharp Telios HC-AJ1/AJ2"), BIFB_D16_0000,
		800, 600, 0, 0,
		PLATID_CPU_MIPS_TX_3922, PLATID_MACH_SHARP_TELIOS_HCAJ1},
	{ TEXT("Sharp Telios HC-VJ1C (Japanese)"), BIFB_D16_0000,
		800, 480, 0, 0,
		PLATID_CPU_MIPS_TX_3922, PLATID_MACH_SHARP_TELIOS_HCVJ1C_JP},
	{ TEXT("Sharp Mobilon HC-4100/4500"), BIFB_D2_M2L_0, /* XXX 4bit greyscale */
		640, 240, 0, 0,
		PLATID_CPU_MIPS_TX_3912, PLATID_MACH_SHARP_MOBILON_HC4100},
	{ TEXT("Sharp HC-1200"), BIFB_D2_M2L_0, /* XXX 4bit greyscale */
		640, 240, 0, 0,
		PLATID_CPU_MIPS_TX_3912, PLATID_MACH_SHARP_MOBILON_HC1200},
};

#define ARRAYSIZEOF(a)	(sizeof(a)/sizeof(*(a)))

#ifdef UNDER_CE
	/* 'memory card' in HANKAKU KANA */
#define UNICODE_MEMORY_CARD \
	TEXT('\\'), 0xff92, 0xff93, 0xff98, TEXT(' '), 0xff76, 0xff70, \
	0xff84, 0xff9e
TCHAR unicode_memory_card[] = { UNICODE_MEMORY_CARD,  TEXT('\\'), 0 };
TCHAR unicode_memory_card1[] = { UNICODE_MEMORY_CARD, TEXT('1'),TEXT('\\'),0 };
TCHAR unicode_memory_card2[] = { UNICODE_MEMORY_CARD, TEXT('2'),TEXT('\\'),0 };
#endif

#define LANGID_DEFAULT MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT)
struct path_s path_list[] = {
	{ TEXT("/"),
	  LANGID_DEFAULT, 0 },
	{ TEXT("2:/"),
	  LANGID_DEFAULT, 0 },
	{ TEXT("\\"),
	  LANGID_DEFAULT, 0 },
	{ TEXT("\\My Documents\\"),
	  LANGID_DEFAULT, 0 },
	{ TEXT("\\Storage Card\\"),
	  LANGID_DEFAULT, PATH_SAVE },
	{ TEXT("\\Storage Card1\\"),
	  LANGID_DEFAULT, PATH_SAVE },
	{ TEXT("\\Storage Card2\\"),
	  LANGID_DEFAULT, PATH_SAVE },
#ifdef UNDER_CE
	{ unicode_memory_card,
	  MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT), PATH_SAVE },
	{ unicode_memory_card1,
	  MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT), PATH_SAVE },
	{ unicode_memory_card2,
	  MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT), PATH_SAVE },
#endif
};
int path_list_items = ARRAYSIZEOF(path_list);

#ifdef ADDITIONAL_KERNELS
TCHAR* kernel_list[] = {

};
int kernel_list_items = ARRAYSIZEOF(kernel_list);
#endif

/*-----------------------------------------------------------------------------

  function prototypes

-----------------------------------------------------------------------------*/
BOOL CALLBACK MainDlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void SetBootInfo(struct bootinfo *bi, struct fb_setting *fbs);
void wstrcpy(TCHAR* dst, TCHAR* src);
int reverse_fb_type(int type);

/*-----------------------------------------------------------------------------

  function definitions

-----------------------------------------------------------------------------*/
void wstrcpy(TCHAR* dst, TCHAR* src)
{
	while (*src) {
		*dst++ = *src++;
	}
	*dst = *src;
}

int reverse_fb_type(int type)
{
	int i;
	struct {
		int type0, type1;
	} types[] = {
		{ BIFB_D2_M2L_3,	BIFB_D2_M2L_0	},
		{ BIFB_D2_M2L_3x2,	BIFB_D2_M2L_0x2	},
		{ BIFB_D4_M2L_F,	BIFB_D4_M2L_0	},
		{ BIFB_D4_M2L_Fx2,	BIFB_D4_M2L_0x2	},
		{ BIFB_D8_FF,		BIFB_D8_00		},
		{ BIFB_D16_FFFF,	BIFB_D16_0000,	},
	};

	for (i = 0; i < ARRAYSIZEOF(types); i++) {
		if (types[i].type0 == type) {
			return (types[i].type1);
		}
		if (types[i].type1 == type) {
			return (types[i].type0);
		}
	}
	debug_printf(TEXT("reverse_fb_type(): unknown type %d\n"), type);
	return (type);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPTSTR lpCmdLine, int nCmdShow )
{
	MSG          msg;
	WNDCLASS     wc;
	HDC hdc;
	int i, idx,width,height;

#if ( 200 <= _WIN32_WCE )
	OSVERSIONINFO	osverinfo;

	osverinfo.dwOSVersionInfoSize = sizeof(osverinfo);
	if (!GetVersionEx(&osverinfo)) {
		msg_printf(MSG_ERROR,
			   TEXT("Error"),
			   TEXT("GetVersionEx() failed"));
	}
	osversion = osverinfo.dwMajorVersion * 100 + osverinfo.dwMinorVersion;
#else
	osversion = 100;
#endif

	/*
	 * create log file for debugging
	 */
	for (i = 0; i < path_list_items; i++) {
		TCHAR filenamebuf[1024];
		if (!(path_list[i].flags & PATH_SAVE)) {
			continue;
		}
		wsprintf(filenamebuf, TEXT("%s%s"),
		    path_list[i].name, LOGNAME);
		if (set_debug_log(filenamebuf) == 0) {
			msg_printf(MSG_INFO,
			    TEXT("Debug"),
			    TEXT("%s was created"), LOGNAME);
			break;
		}
	}

	debug_printf(TEXT("%s"), version_string);
	debug_printf(TEXT("Compiled for %d, Runtime OS version %d\n"),
		     _WIN32_WCE, osversion);

	wc.style          = (UINT)NULL;
	wc.lpfnWndProc    = (WNDPROC) WndProc;
	wc.cbClsExtra     = 0;
	wc.cbWndExtra     = 0;
	wc.hInstance      = hInstance;
	wc.hIcon          = NULL;
	wc.hCursor        = NULL;
	wc.hbrBackground  = (HBRUSH) GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName   = NULL;
	wc.lpszClassName  = whoami;

	RegisterClass(&wc);

	InitCommonControls();   // Initialize common controls - command bar
	hInst = hInstance;      // Save handle to create command bar

	hardware_test();


	hBack = CreateWindowEx(0,
				  szAppName,
				  szTitle,
				  WS_VISIBLE,
				  CW_USEDEFAULT,
				  CW_USEDEFAULT,
				  CW_USEDEFAULT,
				  CW_USEDEFAULT,
				  NULL,
				  NULL,
				  hInstance,
				  NULL);


	hdc = GetDC(0);
	width = GetDeviceCaps(hdc,HORZRES);
	height = GetDeviceCaps(hdc,VERTRES);
	ReleaseDC(0,hdc);

	if(width > height){
		hDlgMain = CreateDialog(hInstance,MAKEINTRESOURCE(IDD_MAIN_320X240),hBack,MainDlgProc); 
	}
	else{
		hDlgMain = CreateDialog(hInstance,MAKEINTRESOURCE(IDD_MAIN_240X320),hBack,MainDlgProc);
	}

	SetFocus(GetDlgItem(hDlgMain, IDC_BOOT));
	SetForegroundWindow(hDlgMain);

	/*
	 *  load preferences
	 */
	pref_init(&pref);
	if (pref_load(path_list, path_list_items) == 0) {
		stat_printf(TEXT("%s is loaded."), where_pref_load_from);

		fb_settings[0].type = pref.fb_type;
		fb_settings[0].width = pref.fb_width;
		fb_settings[0].height = pref.fb_height;
		fb_settings[0].linebytes = pref.fb_linebytes;
		fb_settings[0].addr = pref.fb_addr;
		fb_settings[0].platid_cpu = pref.platid_cpu;
		fb_settings[0].platid_machine = pref.platid_machine;
	} else {
		TCHAR tmpbuf[PATHBUFLEN];
		wsprintf(tmpbuf, TEXT("%s%S"), path_list[0].name, "netbsd");
		stat_printf(TEXT("preferences not loaded."));

		pref.setting_idx = 1;
		pref.fb_type = fb_settings[0].type;
		pref.fb_width = fb_settings[0].width;
		pref.fb_height = fb_settings[0].height;
		pref.fb_linebytes = fb_settings[0].linebytes;
		pref.fb_addr = fb_settings[0].addr;
		pref.platid_cpu = fb_settings[0].platid_cpu;
		pref.platid_machine = fb_settings[0].platid_machine;
		wstrcpy(pref.setting_name, TEXT("User defined"));
		wstrcpy(pref.kernel_name, tmpbuf);
		wstrcpy(pref.options, TEXT(""));
		pref.check_last_chance = FALSE;
		pref.load_debug_info = FALSE;
		pref.serial_port = FALSE;
		pref.reverse_video = FALSE;
	}
	fb_settings[0].name = pref.setting_name;

	/*
	 *  initialize kernel file name list.
	 */
	for (i = 0; i < path_list_items; i++) {
		if (path_list[i].langid == LANGID_DEFAULT ||
		    path_list[i].langid == GetSystemDefaultLangID()) {
			TCHAR tmpbuf[1024];
			wsprintf(tmpbuf, TEXT("%s%S"),
			    path_list[i].name, "netbsd");
			SendDlgItemMessage(hDlgMain, IDC_KERNEL,
			    CB_ADDSTRING, 0, (LPARAM)tmpbuf);
		}
	}
#ifdef ADDITIONAL_KERNELS
	for (i = 0; i < kernel_list_items; i++) {
		SendDlgItemMessage(hDlgMain, IDC_KERNEL, CB_ADDSTRING, 0,
				   (LPARAM)kernel_list[i]);
	}
#endif
	/*
	SendDlgItemMessage(hDlgMain, IDC_KERNEL, CB_SETCURSEL, 0,
			   (LPARAM)NULL);
	*/
	SetDlgItemText(hDlgMain, IDC_KERNEL, pref.kernel_name);
	SetDlgItemText(hDlgMain, IDC_OPTIONS, pref.options);

	/*
	 *  Frame Buffer setting names.
	 */
	for (i = 0; i < ARRAYSIZEOF(fb_settings); i++) {
		idx = SendDlgItemMessage(hDlgMain, IDC_FBSELECT, CB_ADDSTRING,
					 0, (LPARAM)fb_settings[i].name);
		SendDlgItemMessage(hDlgMain, IDC_FBSELECT,
				   CB_SETITEMDATA, idx, (LPARAM)i);
		if (i == 0) {
			user_define_idx = idx;
		}
	}
	SendDlgItemMessage(hDlgMain, IDC_FBSELECT, CB_SETCURSEL,
			   pref.setting_idx, (LPARAM)NULL);

	if(pref.autoboot){
		if(pref.boot_time > 0){/* 0 can't use */
			booting = TRUE;
			how_long_to_boot = pref.boot_time;
			SetTimer(hDlgMain,IDD_TIMER,1000,NULL);
			stat_printf(
				TEXT("autoboot after %d second,tap or hit any key to interrupt"),
				pref.boot_time);
		}
	}
	/*
	 *  Map window and message loop
	 */
	ShowWindow(hDlgMain, SW_SHOW);
	UpdateWindow(hDlgMain);


	while (GetMessage(&msg, NULL, 0, 0)) {
	//	if (osversion < 211 ||
		if(booting){
			if(msg.message == WM_KEYDOWN || msg.message == WM_LBUTTONDOWN){
				booting = FALSE;
				how_long_to_boot = -1;
				KillTimer(hDlgMain,IDD_TIMER);
				stat_printf(TEXT("interrupt"));
				continue;
			}
		}
			
		if(!IsDialogMessage(hDlgMain, &msg)) {
		
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return(msg.wParam);
}

BOOL CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {

	case WM_INITDIALOG:
		return (1);

	case WM_PALETTECHANGED:
		palette_check(hWnd);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			dlgStatus = IDCANCEL;
			break;
		}
		break;
	default:
		return (0);
	}
}

BOOL CALLBACK DlgProc2(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_INITDIALOG:
		SetDlgItemText(hWnd, IDC_ABOUT_EDIT, version_string);
		return (1);

	case WM_PALETTECHANGED:
		palette_check(hWnd);
		break;

	case WM_COMMAND:
	
		switch (LOWORD(wParam)) {
		case IDC_ABOUT_EDIT:
			switch (HIWORD(wParam)) {
			case EN_SETFOCUS:
				//SendDlgItemMessage(hWnd, IDC_ABOUT_EDIT, EM_SETSEL, -1, 0);
				SetFocus(GetDlgItem(hWnd, IDC_ABOUT_BITMAP));
				break;
			}
			break;

		case IDCANCEL:
			EndDialog(hWnd, LOWORD(wParam));
			return (1);
		}
		break;
	default:
		return (0);
	}
}



BOOL CALLBACK PropDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND hwnd;
	TCHAR tempbuf[PATHBUFLEN];
	static BOOL autop;
	int i;
	switch (message) {
	case WM_INITDIALOG:
		autop = pref.autoboot;

		SendDlgItemMessage(hWnd, IDC_PAUSE, BM_SETCHECK,
			   pref.check_last_chance, 0);
		SendDlgItemMessage(hWnd, IDC_DEBUG, BM_SETCHECK,
			   pref.load_debug_info, 0);
		SendDlgItemMessage(hWnd, IDC_COMM, BM_SETCHECK,
			   pref.serial_port, 0);
		SendDlgItemMessage(hWnd, IDC_REVERSEVIDEO, BM_SETCHECK,
			   pref.reverse_video, 0);
		SendDlgItemMessage(hWnd,IDC_AUTOBOOT,BM_SETCHECK,pref.autoboot,0);

		for (i = 0; i < ARRAYSIZEOF(boot_times); i++) {
			wsprintf(tempbuf, TEXT("%d"), boot_times[i]);
			SendDlgItemMessage(hWnd, IDC_BOOT_TIME, CB_ADDSTRING,
					   0, (LPARAM)tempbuf);			
		}

		if(pref.boot_time){
			wsprintf(tempbuf,TEXT("%d"),pref.boot_time);
			SetDlgItemText(hWnd,IDC_BOOT_TIME,tempbuf);
		}
		else{
			wsprintf(tempbuf, TEXT("%d"), boot_times[0]);
			SendDlgItemMessage(hWnd, IDC_BOOT_TIME, CB_ADDSTRING,
					   0, (LPARAM)tempbuf);	
		}

		
		hwnd = GetDlgItem(hWnd,IDC_BOOT_TIME);		
		EnableWindow(hwnd,pref.autoboot);			
	
		return (1);

	case WM_PALETTECHANGED:
		palette_check(hWnd);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_AUTOBOOT:
			autop = !autop;
			SendDlgItemMessage(hWnd,IDC_AUTOBOOT,BM_SETCHECK,
				autop,0);

			hwnd = GetDlgItem(hWnd,IDC_BOOT_TIME);		
			EnableWindow(hwnd,autop);	
			
			break;
		case IDPROPOK:
			if (SendDlgItemMessage(hWnd, IDC_PAUSE,
					       BM_GETCHECK, 0, 0) ==
								BST_CHECKED) {
				pref.check_last_chance = TRUE;
			} else {
				pref.check_last_chance = FALSE;
			}

			if (SendDlgItemMessage(hWnd, IDC_DEBUG,
					       BM_GETCHECK, 0, 0) ==
								BST_CHECKED) {
				pref.load_debug_info = TRUE;
			} else {
				pref.load_debug_info = FALSE;
			}

			if (SendDlgItemMessage(hWnd, IDC_COMM,
					       BM_GETCHECK, 0, 0) ==
								BST_CHECKED) {
				pref.serial_port = TRUE;
			} else {
				pref.serial_port = FALSE;
			}

			if (SendDlgItemMessage(hWnd, IDC_REVERSEVIDEO,
					       BM_GETCHECK, 0, 0) ==
								BST_CHECKED) {
				pref.reverse_video = TRUE;
			} else {
				pref.reverse_video = FALSE;
			}
			
			if (SendDlgItemMessage(hWnd, IDC_AUTOBOOT,
					       BM_GETCHECK, 0, 0) ==
								BST_CHECKED) {
				pref.autoboot = TRUE;
			} else {
				pref.autoboot = FALSE;
			}
			GetDlgItemText(hWnd,IDC_BOOT_TIME,tempbuf,PATHBUFLEN);
			pref.boot_time = _wtoi(tempbuf);

			EndDialog(hWnd, 0);
			return (1);
		case IDCANCEL:
			EndDialog(hWnd,0);
			return(1);
		}
		break;
	default:
		return (0);
	}
}

void
SetBootInfo(struct bootinfo *bi, struct fb_setting *fbs)
{
	TIME_ZONE_INFORMATION tz;

	GetTimeZoneInformation(&tz);
	memset(bi, 0, sizeof(struct bootinfo));
	bi->length = sizeof(struct bootinfo);
	bi->reserved = 0;
	bi->magic = BOOTINFO_MAGIC;
	bi->fb_addr = (unsigned char*)(fbs->addr + 0xA0000000);
	bi->fb_type = fbs->type;
	bi->fb_line_bytes = fbs->linebytes;
	bi->fb_width = fbs->width;
	bi->fb_height = fbs->height;
	bi->platid_cpu = fbs->platid_cpu;
	bi->platid_machine = fbs->platid_machine;
	bi->timezone = tz.Bias;

	debug_printf(TEXT("fb setting: %s fb_type=%d 0x%X %dx%d %d\n"),
		     fbs->name,
		     bi->fb_type, bi->fb_addr,
		     bi->fb_width, bi->fb_height, bi->fb_line_bytes);
	debug_printf(TEXT("timezone: %02ld:00\n"), (bi->timezone / 60));
}


void
UpdateFbDlg(HWND hWnd, struct fb_setting *fbs, int direction)
{
	int i;
	TCHAR tmpbuf[PATHBUFLEN];
	int type, width, height, linebytes;
	long addr;

	switch (direction) {
	case UPDATE_DLGBOX:
		SetDlgItemText(hWnd, IDC_FB_NAME, fbs->name);

		for (i = 0; i < ARRAYSIZEOF(fb_types); i++) {
			if (fb_types[i].type == fbs->type) break;
		}
		if (ARRAYSIZEOF(fb_types) <= i) {
			MessageBox(NULL, TEXT("Unknown FrameBuffer type."),
				   szAppName, MB_OK);
			return;
		}
		debug_printf(TEXT("UpdateFbDlg(%s)\n"), fbs->name);
		i = SendDlgItemMessage(hWnd, IDC_FB_TYPE, CB_FINDSTRINGEXACT,
				       0, (LPARAM)fb_types[i].name);
		SendDlgItemMessage(hWnd, IDC_FB_TYPE, CB_SETCURSEL, i, 0);
		
		wsprintf(tmpbuf, TEXT("%X"), fbs->addr);
		SetDlgItemText(hWnd, IDC_FB_ADDR, tmpbuf);
		wsprintf(tmpbuf, TEXT("%d"), fbs->width);
		SetDlgItemText(hWnd, IDC_FB_WIDTH, tmpbuf);
		wsprintf(tmpbuf, TEXT("%d"), fbs->height);
		SetDlgItemText(hWnd, IDC_FB_HEIGHT, tmpbuf);
		wsprintf(tmpbuf, TEXT("%d"), fbs->linebytes);
		SetDlgItemText(hWnd, IDC_FB_LINEBYTES, tmpbuf);
		wsprintf(tmpbuf, TEXT("%08X"), fbs->platid_cpu);
		SetDlgItemText(hWnd, IDC_FB_CPU, tmpbuf);
		wsprintf(tmpbuf, TEXT("%08X"), fbs->platid_machine);
		SetDlgItemText(hWnd, IDC_FB_MACHINE, tmpbuf);
		break;
	case UPDATE_DATA:
		GetDlgItemText(hWnd, IDC_FB_NAME, fbs->name, PATHBUFLEN);
		type = SendDlgItemMessage(hWnd, IDC_FB_TYPE,
					  CB_GETCURSEL, 0, 0);
		type = SendDlgItemMessage(hWnd, IDC_FB_TYPE,
					  CB_GETITEMDATA, type, 0);
		GetDlgItemText(hWnd, IDC_FB_WIDTH, tmpbuf, sizeof(tmpbuf));
		width = _tcstol(tmpbuf, NULL, 10);
		GetDlgItemText(hWnd, IDC_FB_HEIGHT, tmpbuf, sizeof(tmpbuf));
		height = _tcstol(tmpbuf, NULL, 10);
		GetDlgItemText(hWnd, IDC_FB_LINEBYTES, tmpbuf, sizeof(tmpbuf));
		linebytes = _tcstol(tmpbuf, NULL, 10);
		GetDlgItemText(hWnd, IDC_FB_ADDR, tmpbuf, sizeof(tmpbuf));
		addr = _tcstoul(tmpbuf, NULL, 16);
		GetDlgItemText(hWnd, IDC_FB_CPU, tmpbuf, sizeof(tmpbuf));
		fbs->platid_cpu = _tcstoul(tmpbuf, NULL, 16);
		GetDlgItemText(hWnd, IDC_FB_MACHINE, tmpbuf, sizeof(tmpbuf));
		fbs->platid_machine = _tcstoul(tmpbuf, NULL, 16);
		fbs->type = type;
		fbs->addr = addr;
		fbs->width = width;
		fbs->height = height;
		fbs->linebytes = linebytes;

		debug_printf(TEXT("type=%d  %dx%d  %d bytes/line %08x %08x\n"),
			     type, width, height, linebytes,
			     fbs->platid_cpu,
			     fbs->platid_machine);
		break;
	default:
		debug_printf(TEXT("UpdateFbDlg(): internal error!\n"));
		break;
	}
}

BOOL CALLBACK FbDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int idx, i;
	TCHAR tmpbuf[100];

	switch (message) {
	case WM_INITDIALOG:
		{
		UDACCEL uda;
		for (i = 0; i < ARRAYSIZEOF(fb_settings); i++) {
			idx = SendDlgItemMessage(hWnd, IDC_FB_NAME,
						 CB_ADDSTRING, 0,
						 (LPARAM)fb_settings[i].name);
			SendDlgItemMessage(hWnd, IDC_FB_NAME,
					   CB_SETITEMDATA, idx, (LPARAM)i);
		}
		for (i = 0; i < ARRAYSIZEOF(fb_size); i++) {
			wsprintf(tmpbuf, TEXT("%d"), fb_size[i]);
			SendDlgItemMessage(hWnd, IDC_FB_WIDTH, CB_ADDSTRING,
					   0, (LPARAM)tmpbuf);
			SendDlgItemMessage(hWnd, IDC_FB_HEIGHT, CB_ADDSTRING,
					   0, (LPARAM)tmpbuf);
		}
		for (i = 0; i < ARRAYSIZEOF(fb_bpl); i++) {
			wsprintf(tmpbuf, TEXT("%d"), fb_bpl[i]);
			SendDlgItemMessage(hWnd, IDC_FB_LINEBYTES,
					   CB_ADDSTRING, 0,
					   (LPARAM)tmpbuf);
		}
		for (i = 0; i < ARRAYSIZEOF(fb_types); i++) {
			idx = SendDlgItemMessage(hWnd, IDC_FB_TYPE,
						 CB_ADDSTRING, 0,
						 (LPARAM)fb_types[i].name);
			SendDlgItemMessage(hWnd, IDC_FB_TYPE, CB_SETITEMDATA,
					   idx, (LPARAM)fb_types[i].type);
		}
		UpdateFbDlg(hWnd, &fb_settings[0], UPDATE_DLGBOX);

		uda.nSec = 1;
		uda.nInc = 0x100;
		/*
		SendDlgItemMessage(hWnd, IDC_FB_ADDRSPIN, UDM_SETACCEL,
				   0, (LPARAM)&uda);
		*/
		/*
		SendDlgItemMessage(hWnd, IDC_FB_ADDRSPIN, UDM_SETRANGE,
		                   0, MAKELPARAM(UD_MAXVAL, UD_MINVAL));
		*/
		}
		return (1);

	case WM_PALETTECHANGED:
		palette_check(hWnd);
		break;

	case WM_VSCROLL:
		if ((HWND)lParam == GetDlgItem(hWnd, IDC_FB_ADDRSPIN)) {
			long addr;
			switch (LOWORD(wParam)) {
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				GetDlgItemText(hWnd, IDC_FB_ADDR, tmpbuf, 100);
				addr = _tcstoul(tmpbuf, NULL, 16);
				if (50 < HIWORD(wParam)) {
					addr -= 0x400;
				} else {
					addr += 0x400;
				}
				SendDlgItemMessage(hWnd, IDC_FB_ADDRSPIN,
						   UDM_SETPOS, 0,
						   MAKELPARAM(50, 0));
				wsprintf(tmpbuf, TEXT("%X"), addr);
				SetDlgItemText(hWnd, IDC_FB_ADDR, tmpbuf);
				return (1);
			}
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_FB_NAME:
			switch (HIWORD(wParam)) {
			case CBN_SELCHANGE:
				idx = SendDlgItemMessage(hWnd, IDC_FB_NAME,
							 CB_GETCURSEL, 0, 0);
				i = SendDlgItemMessage(hWnd, IDC_FB_NAME,
						       CB_GETITEMDATA, idx, 0);
				if (0 <= i && i < ARRAYSIZEOF(fb_settings)) {
					fb_settings[0] = fb_settings[i];
					UpdateFbDlg(hWnd, &fb_settings[0],
						    UPDATE_DLGBOX);
				}
				return (1);
			}
			break;
		case IDOK:
			UpdateFbDlg(hWnd, &fb_settings[0], UPDATE_DATA);

			EndDialog(hWnd, IDOK);
			return (1);

		case IDCANCEL:
			EndDialog(hWnd, IDCANCEL);
			return (1);
		}
		break;
	}
	return (0);
}


BOOL SerialPort(BOOL on)
{
	static HANDLE hPort = INVALID_HANDLE_VALUE;
	BOOL res = (hPort != INVALID_HANDLE_VALUE);

	if (on != res) {
		if (on) {
			hPort = CreateFile(TEXT("COM1:"),
					   GENERIC_READ | GENERIC_WRITE,
					   0, NULL, OPEN_EXISTING,
					   0,
					   NULL);
			debug_printf(TEXT("serial port ON\n"));
			if ( hPort == INVALID_HANDLE_VALUE ) {
				debug_printf(TEXT("open failed\n"));
			} else {
#if 0
				DWORD Len;
				BYTE x = 'X';
				WriteFile (hPort, &x, 1, &Len, 0);
				WriteFile (hPort, &x, 1, &Len, 0);
				WriteFile (hPort, &x, 1, &Len, 0);
				WriteFile (hPort, &x, 1, &Len, 0);
#endif
			}
		} else {
			debug_printf(TEXT("serial port OFF\n"));
			CloseHandle(hPort);
			hPort = INVALID_HANDLE_VALUE;
		}
	}

	return (res);
}


BOOL CheckCancel(int progress)
{
	MSG msg;

	if (0 <= progress) {
		SendDlgItemMessage(hDlgLoad, IDC_PROGRESS, 
				   PBM_SETPOS, (WPARAM)progress, (LPARAM)NULL);
	} else {
		if (pref.check_last_chance) {
			if (msg_printf(MB_YESNO | MB_ICONHAND,
			    TEXT("Last chance..."),
			    TEXT("Push OK to boot.")) != IDYES) {
				dlgStatus = IDCANCEL;
			}
		}
		palette_set(hDlgLoad);
		if (palette_succeeded == -1) {
			msg_printf(MSG_ERROR,
			    TEXT("Warning"),
			    TEXT("Sorry, palette failed"));
		}
	}

	/*
	 *  Put WM_TIMER in my message queue.
	 *  (WM_TIMER has lowest priority.)
	 */
	SetTimer(hDlgLoad, 1, 1, NULL);

	/*
	 *  I tried PeekMessage() but it does not work.
	 */
	while (GetMessage(&msg, NULL, 0, 0)) {
		if (msg.hwnd == hDlgLoad && msg.message == WM_TIMER) {
			break;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (dlgStatus != 0);
}

BOOL BootKernel(int directboot)
{
	int argc;
	int i;
	TCHAR wkernel_name[PATHBUFLEN];
	TCHAR woptions[PATHBUFLEN];
	
	char options[PATHBUFLEN*2], kernel_name[PATHBUFLEN*2];
	
	platid_t platid;
	
	char *p, *argv[32];
	struct bootinfo bi;

	if (GetDlgItemText(hDlgMain, IDC_KERNEL, wkernel_name,
		sizeof(wkernel_name)) == 0) {      
		MessageBox (NULL, TEXT("Kernel name required"),
			szAppName, MB_OK);
		return FALSE;
	}				
	GetDlgItemText(hDlgMain, IDC_OPTIONS,
		woptions, sizeof(woptions));
	if (wcstombs(options, woptions, sizeof(options)) < 0 ||
		wcstombs(kernel_name, wkernel_name,
			     sizeof(kernel_name)) < 0) {
		MessageBox (NULL, TEXT("invalid character"),
			szAppName, MB_OK);
		return FALSE;
	}
	
	argc = 0;
	argv[argc++] = kernel_name;
	p = options;
	while (*p) {
		while (*p == ' ' || *p == '\t') {
			p++;
		}
		if (*p == '\0') 
			return FALSE;
		if (ARRAYSIZEOF(argv) <= argc) {
			MessageBox (NULL,
				TEXT("too many options"),
				szAppName, MB_OK);
			argc++;
			break;
		} else {
			argv[argc++] = p;
		}
		while (*p != ' ' && *p != '\t' && *p != '\0') {
			p++;
		}
		if (*p == '\0') {
			break;
		} else {
			*p++ = '\0';
		}
	}
	if (ARRAYSIZEOF(argv) < argc) {
		return FALSE;
	}
	
	
	EnableWindow(hDlgMain, FALSE);
	
	if (directboot || (MessageBox (hDlgMain,
		TEXT("Data in memory will be lost.\nAre you sure?"),
		szAppName,
		MB_YESNO | MB_DEFBUTTON2 | MB_ICONHAND) == IDYES)) {
		booting = FALSE;
		dlgStatus = 0;
		hDlgLoad =
			CreateDialog(hInst,
					       MAKEINTRESOURCE(IDD_LOAD),
						   hDlgMain, DlgProc);
		ShowWindow(hDlgLoad, SW_SHOWNORMAL);
		BringWindowToTop(hDlgLoad);
		
		/*
		*  save settings.
		*/
		pref.fb_type		= fb_settings[0].type;
		pref.fb_width		= fb_settings[0].width;
		pref.fb_height		= fb_settings[0].height;
		pref.fb_linebytes	= fb_settings[0].linebytes;
		pref.fb_addr		= fb_settings[0].addr;
		pref.platid_cpu		= fb_settings[0].platid_cpu;
		pref.platid_machine	= fb_settings[0].platid_machine;
		wstrcpy(pref.kernel_name, wkernel_name);
		wstrcpy(pref.options, woptions);
		
		pref_save(path_list, path_list_items);
		
		SetBootInfo(&bi, &fb_settings[pref.setting_idx]);
		debug_printf(TEXT("Args: "));
		for (i = 0; i < argc; i++) {
			debug_printf(TEXT("'%S' "), argv[i]);
		}
		debug_printf(TEXT("\n"));
		debug_printf(TEXT("Bootinfo: fb_type=%d 0x%X %dx%d %d\n"),
			bi.fb_type, bi.fb_addr,
			bi.fb_width, bi.fb_height,
			bi.fb_line_bytes);
		
		if (pref.serial_port) {
			SerialPort(TRUE);
		}
		if (pref.reverse_video) {
			bi.fb_type = reverse_fb_type(bi.fb_type);
		}
		/* 
		* Set system information
		*/
		platid.dw.dw0 = bi.platid_cpu;
		platid.dw.dw1 = bi.platid_machine;
		if (set_system_info(&platid)) {
		/*
		*  boot !
			*/
			pbsdboot(wkernel_name, argc, argv, &bi);
		}
		/*
		*  Not return.
		*/
		
		if (pref.serial_port) {
			SerialPort(FALSE);
		}
		
		DestroyWindow(hDlgLoad);
	}
	EnableWindow(hDlgMain, TRUE);

	return FALSE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message,
						 WPARAM wParam, LPARAM lParam ){
	switch (message) {
	case WM_CREATE:
		palette_init(hWnd);
		break;

	case WM_PALETTECHANGED:
		palette_check(hWnd);
		break;

	 case WM_QUERYNEWPALETTE:
		return(TRUE);

	case WM_CLOSE:
	        sndPlaySound(TEXT("Close"), SND_NODEFAULT | SND_ASYNC);

		DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
	        PostQuitMessage(0);
		break;

	default:
        	return (DefWindowProc(hWnd, message, wParam, lParam));

	}
	return 0;
}

BOOL CALLBACK MainDlgProc(HWND hWnd, UINT message,
                          WPARAM wParam, LPARAM lParam )
{
	int i, idx;

	switch (message) {
	case WM_CREATE:
		sndPlaySound(TEXT("OpenProg"), SND_NODEFAULT | SND_ASYNC);
		hWndCB = CommandBar_Create(hInst, hWnd, 1);
		CommandBar_AddAdornments(hWndCB, STD_HELP, (DWORD)NULL);
		break;
	case WM_PAINT:
		{
		HDC          hdc;
		PAINTSTRUCT  ps;

		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		}
		break;

	case WM_PALETTECHANGED:
		palette_check(hWnd);
		break;

	case WM_HELP:
		/*
		MessageBox (NULL, TEXT("HELP NOT AVAILABLE"),
			    szAppName, MB_OK);
		*/
		DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUT), hWnd, DlgProc2);
        break;


	case WM_TIMER:
		if(!booting){/* ignore! */
			KillTimer(hWnd,IDD_TIMER);
			stat_printf(TEXT("interrupt2"));
			return (1);
		}
		if(how_long_to_boot > 0){
			how_long_to_boot--;
			stat_printf(
				TEXT("autoboot after %d second,tap or hit any key to interrupt"),
				how_long_to_boot);
		}
		else{
			KillTimer(hWnd,IDD_TIMER);
			BootKernel(1);
		}
		break;
	
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_BOOT:
			BootKernel(0);
			break;
		case IDC_FBSETTING:
			if (DialogBox(hInst, MAKEINTRESOURCE(IDD_FB),
				      hDlgMain, FbDlgProc) == IDOK) {
				/* User defined */
				pref.setting_idx = 0;
				SendDlgItemMessage(hDlgMain, IDC_FBSELECT,
						   CB_DELETESTRING,
						   (WPARAM)user_define_idx, 0);
				SendDlgItemMessage(hDlgMain, IDC_FBSELECT,
						   CB_INSERTSTRING,
						   (WPARAM)user_define_idx,
						   (LPARAM)fb_settings[0].name);
				SendDlgItemMessage(hWnd, IDC_FBSELECT,
						   CB_SETCURSEL, 0, 0);
			}
			break;
		case IDC_PROPERTY:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_PROP), hWnd, PropDlgProc);
			break;
		case IDC_FBSELECT:
			switch (HIWORD(wParam)) {
			case CBN_SELCHANGE:
				idx = SendDlgItemMessage(hWnd, IDC_FBSELECT,
							 CB_GETCURSEL, 0, 0);
				i = SendDlgItemMessage(hWnd, IDC_FBSELECT,
						       CB_GETITEMDATA, idx, 0);
				if (0 <= i && i < ARRAYSIZEOF(fb_settings)) {
					debug_printf(TEXT("fb_setting=%d\n"), i);
					pref.setting_idx = i;
				}
				break;
			}
			break;
		}
		break;

	case WM_HIBERNATE:
		MessageBox(NULL, TEXT("MEMORY IS LOW"), szAppName, MB_OK);
		//Additional code to handle a low memory situation

	case WM_CLOSE:
	        sndPlaySound(TEXT("Close"), SND_NODEFAULT | SND_ASYNC);

		DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
	        PostQuitMessage(0);
		break;

	}

	return (0);
}


