/*
 * Driver for custom chips display.
 */

/* this is a mere collection of data, the registers itself are in the
   custom chip area */

struct ccfb {
  int		disp_width;
  int		disp_height;
  int		disp_x, disp_y;		/* this defines the dimension and
  					   relative position of the display. */
  int		disp_z;			/* display depth */

  u_char	*fb;			/* frame buffer, ie. block in chipmem
					   containing bitplane(s) */
  int		fb_width;
  int		fb_height;		/* dimension of the framebuffer. Can
  					   be larger than the display! */
  int		fb_x, fb_y;		/* offset of the framebuffer relative
  					   to the display (disp_*) values */
  int		fb_z;			/* frame buffer depth */

  u_short	col[16];		/* color palette */
  
  u_short	bplstart_off;		/* offset in copperlist where the bitplane
  					   start is set. This is used for smooth
  					   scrolling of oversized framebuffers */
  u_short       *cop1, *cop2;           /* both copperlists */
};


/* these are the initial values for changeable parameters: */
#define	DEF_DISP_WIDTH		640
#define DEF_DISP_HEIGHT		400
#define DEF_DISP_X		258	/* "" */
#define DEF_DISP_Y		88	/* hardware preferred values.. */
#define DEF_FB_X		0
#define DEF_FB_Y		0
#define DEF_COL0		0x123
#define DEF_COL1		0xccc
	
/* these are currently not changeable easily (would require reallocation
   of display memory and rebuild of copperlists. Do this later perhaps) */
#define DEF_FB_WIDTH		1024
#define DEF_FB_HEIGHT		1024
#define DEF_FB_Z		1
#define DEF_DISP_Z		1
