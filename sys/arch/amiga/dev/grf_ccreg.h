/*
 * Driver for custom chips display.
 */

/* this is a mere collection of data, the registers itself are in the
   custom chip area */

struct ccfb {
  int disp_width;
  int disp_height;
  int disp_x, disp_y;		/* this defines the dimension and
				   relative position of the display. */
  int disp_z;			/* display depth */

  u_char *fb;			/* frame buffer, ie. block in chipmem
				   containing bitplane(s) */
  int fb_width;
  int fb_height;		/* dimension of the framebuffer. Can
				   be larger than the display! */
  int fb_planesize;		/* size of each plane, in bytes */
  int fb_x, fb_y;		/* offset of the framebuffer relative
				   to the display (disp_*) values */
  int fb_z;			/* frame buffer depth */

/* these are added by mykes */
#if 0
  /* DON'T PUT THOSE HERE! They're fully calculatable from the above
     values, and would introduce data redundancy */

  /* 
   * diwstrt, etc. are for direct poke into copper list
   * there should be an ioctl to modify these values
   */
  u_short diwstrt, diwstop;
  u_short ddfstrt, ddfstop;	/* initializers for hardware regs */
#endif
  /*
   * 32 word color palette... also poked into copper list
   * color 0 is background color
   * color 1 is text color
   * color 2 is intended for cursor color (not working yet)
   * color 3 is intended for selection color (not working yet)
   * color 10 (hex) is sprite pointer color 0 (transparent)
   * color 11 is sprite pointer color 1
   * color 12 is sprite pointer color 2
   * color 13 is sprite pointer color 3
   */
  u_short col[32];		/* color palette */
  /*
   * beepSample is chipram of 20 bytes/10 words.  It is a sine wave
   * sample for creating the beep sound.
   */
  char *beepSample;		/* pointer to beep audio sample */
  /*
   * the beep can have a variable (set by ioctl()) pitch/period.
   * the ioctl() handler routine must stuff any new period into
   * the hardware, too!
   */
  u_short beepPeriod;		/* audio hardware period for sample */
  /*
   * the beep can have a variable volume (set by ioctl()).  the
   * ioctl() handler routine must stuff any new volume into the
   * hardware, too!
   */
  u_short beepVolume;		/* audio hardware volume for sample */
  /*
   * At console init time, the copper list is created, and so is the
   * sample buffer (in chip).  The init routine starts the audio hardware
   * playing the sample in aud0 and aud1 (stereo L+R), but turns the
   * volume OFF for both channels.  When a beep is started, beepTimer
   * is set to beepTime (set beepTime with ioctl() to override default).
   * the vbl handler cc_vbl() counts down beepTimer and when it hits
   * zero, it turns off the volume again.  Cheap, but it works and no
   * interrupts are needed :-)
   */
  u_short beepTimer, beepTime;	/* timer and timer value for audio beep */
  /*
   * there is a builtin facility for displaybeep, which is just a screen
   * flash.  It is unimplemented, but the fields are defined here.  THe
   * idea is for an ioctl() to be used to enable beep and/or display
   * beep (or neither).  As with beepTime/beepTimer above, just set
   * dbeepTime with the ioctl() handler.
   */
  u_short dbeepTimer, dbeepTime; /* timer and timer value for visual beep */
  /*
   * two copper lists are needed, one for even, one for odd frame of
   * the display.  There is a nice copper list structure defined
   * in grf_cc.c so the ioctl() function can poke values into the
   * copper lists with ease.  Things to poke are colors, horizontal
   * offset (i.e horizontal offset to plane pointers), and screen
   * positioning (diwstrt, etc.).
   */
  u_short *cop1, *cop2;		/* both copperlists */
  /*
   * this is the mouse pointer.  It is 4+16*4+4 bytes in CHIP ram.
   * the first 4 bytes are control bytes for the sprite DMA hardware,
   * and are used by the cc_vbl() interrupt handler to move the
   * mouse.  The next 16*4 words define the sprite, as in the
   * hardware rkm: plane0 word/plane1 word format for 16 tall.
   * the last 4 bytes are 2 null words, as required for sprite DMA.
   * there is a cheap trick played with these null words!  The other
   * 7 sprites point to these NULL words (any ZERO word in chip
   * works, and these are dependable to be NULL).  THe intent for
   * the 16*4 words is for an ioctl() call to be usable to alter
   * the sprite pointer image - just copy 16*4 words into the pointer
   * structure in chip ram!  There is a handy POINTER struct defined
   * in grf_cc.c.
   */
  u_short *pointer;		/* sprite pointer memory */
  /*
   * mouseH and mouseV are the actual mouse counter register values.
   * you need to keep track of them so you can do deltas from this frame's
   * values to last frame's.  lastMouseH and lastMouseV are used for
   * the mouse blanker.  ANY movement of the mouse unblanks the mouse
   * (and if the screen is blanked, it is unblanked too).  You need
   * to detect counter change and not position change - the mouse could
   * be stuck in a corner and not moving :-)
   */
  u_char mouseH, mouseV;	/* mouse horiz, vert for delta */
  u_char lastMouseH, lastMouseV; /* last values (for blanker) */
  /*
   * mouseX and mouseY are the actual mouse coordinates on the screen
   * (in pixels).  There is some incomplete calculations in the source
   * (grf_cc.c) for positioning the mouse - they don't factor in a 
   * variable diwstrt, etc.  This should be fixed :-)  mouseb1 and mouseb2
   * are the mouse button values.  !0 means mouse button is down!
   * An ioctl() can either move the mouse position (change mouseX,mouseY)
   * or it can read the mouse position and buttons.
   */
  short mouseX, mouseY;			/* mouse coords */
  u_char mouseb1, mouseb2, mouseb3;	/* mouse buttons */
  /*
   * There's two joystick ports on the Amiga.  joy0 is the joy port
   * that the mouse is NOT normally plugged into.  joy1 is the joy
   * port the mouse IS normally plugged into.  There are #defines
   * for the bits for the joysticks below.  Note that if a mouse
   * is plugged in, joy1 bits are not joystick bits (garbage :-)
   * There is no facility for the grf_cc.c routine to use a mouse
   * in either port - just the normal mouseport that amigaos uses.
   * this should be enhanced (why not? :-)
   */
  u_char joy0, joy1;		/* joysticks */
  /*
   * The console (grf_cc.c) features a mouseblanker and a screenblanker.
   * If you don't move the mouse for mouseTime 60ths of a second, the
   * mouse will be blanked (sprite DMA turned off).  If you move the
   * mouse, the DMA is turned on again (if the screen is blanked, it
   * is turned on also).  If you don't hit a key or move the mouse for
   * screenTime 60ths of a second, the screen will blank (raster DMA
   * is turned off and color 0 set to BLACK.  If you hit a key or move
   * the mouse, it will be unblanked.  The screenBlank/mouseBlank
   * variables are the timers.  They are inited with screenTime/mouseTime.
   * the cc_vbl() routine counts these timers down, and when they hit
   * zero, the DMA is turned off.  The timers are restarted (full time
   * to count down again) when the mouse is moved or key is hit (key
   * hit for screen blanker only).  screenTime and mouseTime are intended
   * for use with ioctl() handler.  Thus the timer times can be set by
   * the ioctl handler ;-)
   */
  u_short screenTime, mouseTime; /* timer counts for screen/mouse blankers */
  u_short screenBlank, mouseBlank; /* timers: screen, mouse blankers */
  /*
   * this below is unfinished, really.  instead of two bytes, one of pad,
   * it should be 16 bits of flags (u_short).  One of the flags should
   * be the enable flag.  I'm too lazy to fix it right now :-)  The
   * enable flag is checked for blanking purposes.  If the flag is off,
   * the blankers are disabled.  Typical use is for an addon graphics
   * card (i.e. custom chipset graphics not in use).  The other flags
   * should be used to enable beep, blankers, and so forth.
   */
  u_char enableFlag, pad;	/* flag: true if display enabled */
};

/* bits for joy0, joy1 of ccfb */
#define JOYLEFT (1<<0)
#define JOYRIGHT (1<<1)
#define JOYUP (1<<2)
#define JOYDOWN (1<<3)
#define JOYBUTTON (1<<4)

/* mouse/screen  blanker default timer times */
#define DEF_MOUSE (60*5)	/* 5 seconds */
#define DEF_SCREEN (60*5*60)	/* 5 minutes */

/* these are the initial values for changeable parameters: */
#ifdef BORING_DEFAULTS
#define	DEF_DISP_WIDTH		640
#define DEF_DISP_HEIGHT		400
#define DEF_DISP_X		258	/* "" */
#define DEF_DISP_Y		88	/* hardware preferred values.. */
#else
/* these make use of possible overscan, giving you a much larger
   console */
#define DEF_DISP_WIDTH		704
#define DEF_DISP_HEIGHT		464
#define DEF_DISP_X		194
#define DEF_DISP_Y		44
#endif
#define DEF_FB_X		0
#define DEF_FB_Y		0
/* mtk: if it were up to me, I'd delete all of the above */
/* mykes: NOPE! These are enough to regenerate the very hardware
          specific diw* values, so drop those! */

/* 4 colors for text/cursor, etc., 4 colors for sprite/pointer */
#define DEF_COL0		0xaaa
#define DEF_COL1		0x000
#define DEF_COL2		0x68b
#define DEF_COL3		0xfff
#define DEF_COL10		0xf00
#define DEF_COL11		0x0f0
#define DEF_COL12		0x00f
#define DEF_COL13		0x0ff

/* 
 * display mode is a constant for now.  You always get hires interlace 
 * and two planes.  ECS modes may make sense at a later date, as do
 * AGA and beyond.  First we need AGA machines with 030's so BSD will
 * run on them.  Then we need docs on the hardware regs :-)
 */
#if 0
/* don't do these hardwired. Besides, VIEWMODE changes if number of planes
   changes, no hardwire either */

#define VIEWMODE 0xa204		/* 2 planes, hires, interlace */
#define DEF_DIWSTRT 0x2c81
#define DEF_DIWSTOP 0xf4c1
#define DEF_DDFSTRT 0x003c
#define DEF_DDFSTOP 0x00d4
#endif

/* default audio values */
#define DEF_PERIOD 200		/* default period value for audio (beep) */
#define DEF_VOLUME 64		/* default volume value for audio (beep) */
#define DEF_ABEEP 10		/* default timer for audio beep */
/* default display beep values (not implemented yet) */
#define DEF_DBEEP 10		/* default timer for display beep */

/* these are currently not changeable easily (would require reallocation
   of display memory and rebuild of copperlists. Do this later perhaps) */

/* note: mtk, it was easier than you think :-) */
/* note: mykes, yeah because you left them at 640x400... */
#define DEF_FB_WIDTH		1024
#define DEF_FB_HEIGHT		1024
#define DEF_FB_Z		2
#define DEF_DISP_Z		1	/* only 1, the curses plane is
					   treated as overlay plane */

/* 
 * these macros are used for separating hi and lo word of addresses.
 * the copper list needs them separated - the hardware regs are
 * words :-)
 */
#define HIADDR(x) (u_short)((((unsigned long)(x))>>16)&0xffff)
#define LOADDR(x) (u_short)(((unsigned long)(x))&0xffff)

/* copper instructions */
#define COP_MOVE(cl, reg, val)  \
	do { (cl)[0] = CUSTOM_OFS(reg); (cl)[1] = val; } while (0)
#define COP_WAIT(cl, ypos)	\
	do { (cl)[0] = ((ypos) << 8) + 1; (cl)[1] = 0xff00; } while (0)
#define COP_END(cl)		\
	do { (cl)[0] = 0xffff; (cl)[1] = 0xfffe; } while (0)
  
