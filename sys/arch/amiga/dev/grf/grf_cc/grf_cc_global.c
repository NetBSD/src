/*
 * Copyright (c) 1994 Christian E. Hopps
 * All rights reserved.
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
 *      This product includes software developed by Christian E. Hopps.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id: grf_cc_global.c,v 1.3 1994/01/30 08:25:04 chopps Exp $
 */

#include "grf_cc_priv.h"

/* the custom thips monitor */
monitor_t *cc_monitor;

cop_t std_copper_list[] = {
    { CI_WAIT (0, 12), 0xfffe },
#if defined (GRF_ECS)
    { R_BEAMCON0, 0x0000 },
    { R_BPLCON3, 0x0020 },			  /* enable border blank */
#endif
    /* bit plane pointers */
    { R_BPL0PTH, 0x0000 },    { R_BPL0PTL, 0x0000 },
    { R_BPL1PTH, 0x0000 },    { R_BPL1PTL, 0x0000 },
    { R_BPL2PTH, 0x0000 },    { R_BPL2PTL, 0x0000 },
    { R_BPL3PTH, 0x0000 },    { R_BPL3PTL, 0x0000 },
    { R_BPL4PTH, 0x0000 },    { R_BPL4PTL, 0x0000 },
    { R_BPL5PTH, 0x0000 },    { R_BPL5PTL, 0x0000 },
    { R_BPL6PTH, 0x0000 },    { R_BPL6PTL, 0x0000 },
    { R_BPL7PTH, 0x0000 },    { R_BPL7PTL, 0x0000 },
    /* view specific stuff. */
    { R_BPL1MOD, 0x0000 },
    { R_BPL2MOD, 0x0000 },
    { R_DIWSTRT, 0xffff },
    { R_BPLCON0, 0x0000 },
    { R_DIWSTOP, 0x0000 },
#if defined (GRF_ECS)
    { R_DIWHIGH, 0x0000 },
#endif 
    { R_DDFSTRT, 0x0000 },
    { R_DDFSTOP, 0x0000 },
    { R_BPLCON1, 0x0000 },
    /* colors */
    { R_COLOR00, 0x0779 },    { R_COLOR01, 0x0000 },    { R_COLOR02, 0x0FFF },    { R_COLOR03, 0x068B },
    { R_COLOR04, 0x000f },    { R_COLOR05, 0x0f0f },    { R_COLOR06, 0x00ff },    { R_COLOR07, 0x0fff },
    { R_COLOR08, 0x0620 },    { R_COLOR09, 0x0e50 },    { R_COLOR0A, 0x09f1 },    { R_COLOR0B, 0x0eb0 },
    { R_COLOR0C, 0x055f },    { R_COLOR0D, 0x092f },    { R_COLOR0E, 0x00f8 },    { R_COLOR0F, 0x0ccc },
    { R_COLOR10, 0x0e44 },    { R_COLOR11, 0x0e44 },    { R_COLOR12, 0x0000 },    { R_COLOR13, 0x0eec },
    { R_COLOR14, 0x0444 },    { R_COLOR15, 0x0555 },    { R_COLOR16, 0x0666 },    { R_COLOR17, 0x0777 },
    { R_COLOR18, 0x0888 },    { R_COLOR19, 0x0999 },    { R_COLOR1A, 0x0aaa },    { R_COLOR1B, 0x0bbb },
    { R_COLOR1C, 0x0ccc },    { R_COLOR1D, 0x0ddd },    { R_COLOR1E, 0x0eee },    { R_COLOR1F, 0x0fff },
    { R_COP1LCH, 0x0000 },    { R_COP1LCL, 0x0000 },
    { 0xffff, 0xfffe },       { 0xffff, 0xfffe }  /* COPEND, COPEND */
};

/* standard custom chips copper list. */
int std_copper_list_len = sizeof (std_copper_list) / sizeof (cop_t);
int std_copper_list_size = sizeof (std_copper_list);


#if defined (GRF_A2024)
cop_t std_dlace_copper_list[] = {
    { CI_WAIT(0,12), 0xfffe },				  /* WAIT (0, 12) */
#if defined (GRF_ECS)
    { R_BEAMCON0, 0x0000 },
    { R_BPLCON3, 0x0020 },			  /* enable border blank */
#endif
    /* colors */
    { R_COLOR00, 0x0000  }, { R_COLOR01, 0x0000  }, { R_COLOR02, 0x0000 },  { R_COLOR03, 0x0000  },
    { R_COLOR04, 0x0000  }, { R_COLOR05, 0x0000  }, { R_COLOR06, 0x0000 }, { R_COLOR07, 0x0000 },
    { R_COLOR08, 0x0000  }, { R_COLOR09, 0x0000  }, { R_COLOR0A, 0x0000}, { R_COLOR0B, 0x0000 },
    { R_COLOR0C, 0x0000 }, { R_COLOR0D, 0x0000 }, { R_COLOR0E, 0x0000}, { R_COLOR0F, 0x0000 },
    { R_COLOR10, 0x0009 }, { R_COLOR11, 0x0009 }, { R_COLOR12, 0x0001 }, { R_COLOR13, 0x0809 },
    { R_COLOR14, 0x0009 }, { R_COLOR15, 0x0009 }, { R_COLOR16, 0x0001 }, { R_COLOR17, 0x0809 },
    { R_COLOR18, 0x0008 }, { R_COLOR19, 0x0008 }, { R_COLOR1A, 0x0000 }, { R_COLOR1B, 0x0808 },
    { R_COLOR1C, 0x0089 }, { R_COLOR1D, 0x0089 }, { R_COLOR1E, 0x0081 }, { R_COLOR1F, 0x0889 },
    /* set the registers up. */
    { R_DIWSTRT, 0xffff },
    { R_BPLCON0, 0x0000 },
    { R_DIWSTOP, 0x0000 },
#if defined (GRF_ECS)
    { R_DIWHIGH, 0x0000 },
#endif 
    { R_DDFSTRT, 0x0000 },
    { R_DDFSTOP, 0x0000 },
    { R_BPLCON1, 0x0000 },
    /* view specific stuff. */
    { R_BPL1MOD, 0x0000 },
    { R_BPL2MOD, 0x0000 },
    /* bit plane pointers */
    { R_BPL0PTH, 0x0000 },    { R_BPL0PTL, 0x0000 },
    { R_BPL1PTH, 0x0000 },    { R_BPL1PTL, 0x0000 },
    { R_BPL2PTH, 0x0000 },    { R_BPL2PTL, 0x0000 },
    { R_BPL3PTH, 0x0000 },    { R_BPL3PTL, 0x0000 },
    { R_COP1LCH, 0x0000 },    { R_COP1LCL, 0x0000 },
    { 0xffff, 0xfffe },       { 0xffff, 0xfffe }  /* COPEND, COPEND */
};
int std_dlace_copper_list_len = sizeof (std_dlace_copper_list) / sizeof (cop_t);
int std_dlace_copper_list_size = sizeof (std_dlace_copper_list);

cop_t std_a2024_copper_list[] = {
    { CI_WAIT(0,12), 0xfffe },				  /* WAIT (0, 12) */
#if defined (GRF_ECS)
    { R_BEAMCON0, 0x0000 },
#endif
    /* hedley card init setup section */
    { R_COLOR00, 0x0f00 },
    { R_BPL0PTH, 0x0000 }, { R_BPL0PTL, 0x0000 }, /* init plane of 1's with first set for centering */
    { R_DIWSTRT, 0x1561 }, { R_DIWSTOP, 0x16d1 },
#if defined (GRF_ECS)
    { R_DIWHIGH, 0x2000 },
#endif
    { R_DDFSTRT, 0x0040 }, { R_DDFSTOP, 0x00d0 },
    { R_BPLCON0, 0x9200 },
    /* actual data that will be latched by hedley card. */
    { R_COLOR01, 0x0001 },			  /* Stuff1 */
    { CI_WAIT(126,21), 0xfffe  }, { R_COLOR01, 0x0001 },	  /*  Display Quadrent */
    { CI_WAIT(158,21), 0xfffe  }, { R_COLOR01, 0x08f0 },	  /*  Stuff */
    { CI_WAIT(190,21), 0xfffe  }, { R_COLOR01, 0x0ff1 },	  /*  Stuff2 */
    { CI_WAIT(0,22), 0xfffe  },			  
    { R_COLOR00, 0x0000 }, { R_BPLCON0, 0x0000 }, 
    { CI_WAIT(0,43), 0xfffe  },			  
    /* set the registers up. */
    { R_COLOR00, 0x0009 }, { R_COLOR01, 0x0001 }, { R_COLOR02, 0x0008 }, { R_COLOR03, 0x0000 },
    { R_COLOR04, 0x0809 }, { R_COLOR05, 0x0801 }, { R_COLOR06, 0x0808 }, { R_COLOR07, 0x0800 },
    { R_COLOR08, 0x0089 }, { R_COLOR09, 0x0081 }, { R_COLOR0A, 0x0088 }, { R_COLOR0B, 0x0080 },
    { R_COLOR0C, 0x0889 }, { R_COLOR0D, 0x0881 }, { R_COLOR0E, 0x0888 }, { R_COLOR0F, 0x0880 },
    { R_COLOR10, 0x0009 }, { R_COLOR11, 0x0009 }, { R_COLOR12, 0x0001 }, { R_COLOR13, 0x0809 },
    { R_COLOR14, 0x0009 }, { R_COLOR15, 0x0009 }, { R_COLOR16, 0x0001 }, { R_COLOR17, 0x0809 },
    { R_COLOR18, 0x0008 }, { R_COLOR19, 0x0008 }, { R_COLOR1A, 0x0000 }, { R_COLOR1B, 0x0808 },
    { R_COLOR1C, 0x0089 }, { R_COLOR1D, 0x0089 }, { R_COLOR1E, 0x0081 }, { R_COLOR1F, 0x0889 },
    /* window size. */
    { R_DIWSTRT, 0x2c81 }, { R_BPLCON0, 0x0000 }, { R_DIWSTOP, 0xf481 },
    /* datafetch */
    { R_DDFSTRT, 0x0038 }, { R_DDFSTOP, 0x00b8 },
    { R_BPLCON1, 0x0000 },
    { R_BPL1MOD, 0x00bc }, { R_BPL2MOD, 0x00bc },
    /* bitplanes */
    { R_BPL0PTH, 0x0000 }, { R_BPL0PTL, 0x0000 },
    { R_BPL1PTH, 0x0000 }, { R_BPL1PTL, 0x0000 },
    { R_BPL2PTH, 0x0000 }, { R_BPL2PTL, 0x0000 },
    { R_BPL3PTH, 0x0000 }, { R_BPL3PTL, 0x0000 }, 
#if defined (GRF_ECS)
    { R_DIWHIGH, 0x2000 },
#endif
    { R_COP1LCH, 0x0000 }, { R_COP1LCL, 0x0000 },
    { 0xffff, 0xfffe }, { 0xffff, 0xfffe }  /* COPEND, COPEND */
};
int std_a2024_copper_list_len = sizeof (std_a2024_copper_list) / sizeof (cop_t);
int std_a2024_copper_list_size = sizeof (std_a2024_copper_list);

cop_t std_pal_a2024_copper_list[] = {
    { CI_WAIT(0,20), 0xfffe },				  /* WAIT (0, 12) */
#if defined (GRF_ECS)
    { R_BEAMCON0, STANDARD_PAL_BEAMCON },
#endif
    /* hedley card init setup section */
    { R_COLOR00, 0x0f00 },
    { R_BPL0PTH, 0x0000 }, { R_BPL0PTL, 0x0000 }, /* init plane of 1's with first set for centering */
    { R_DIWSTRT, 0x1d61 }, { R_DIWSTOP, 0x1ed1 },
#if defined (GRF_ECS)
    { R_DIWHIGH, 0x2000 },
#endif
    { R_DDFSTRT, 0x0040 }, { R_DDFSTOP, 0x00d0 },
    { R_BPLCON0, 0x9200 },
    /* actual data that will be latched by hedley card. */
    { R_COLOR01, 0x0001 },			  /* Stuff1 */
    { CI_WAIT(126,29), 0xfffe  }, { R_COLOR01, 0x0001 },	  /*  Display Quadrent */
    { CI_WAIT(158,29), 0xfffe  }, { R_COLOR01, 0x08f0 },	  /*  Stuff */
    { CI_WAIT(190,29), 0xfffe  }, { R_COLOR01, 0x0ff1 },	  /*  Stuff2 */
    { CI_WAIT(0,30), 0xfffe  },			  
    { R_COLOR00, 0x0000 }, { R_BPLCON0, 0x0000 },
    { CI_WAIT(0,43), 0xfffe  },

    /* set the registers up. */
    { R_COLOR00, 0x0009 }, { R_COLOR01, 0x0001 }, { R_COLOR02, 0x0008 }, { R_COLOR03, 0x0000 },
    { R_COLOR04, 0x0809 }, { R_COLOR05, 0x0801 }, { R_COLOR06, 0x0808 }, { R_COLOR07, 0x0800 },
    { R_COLOR08, 0x0089 }, { R_COLOR09, 0x0081 }, { R_COLOR0A, 0x0088 }, { R_COLOR0B, 0x0080 },
    { R_COLOR0C, 0x0889 }, { R_COLOR0D, 0x0881 }, { R_COLOR0E, 0x0888 }, { R_COLOR0F, 0x0880 },
    { R_COLOR10, 0x0009 }, { R_COLOR11, 0x0009 }, { R_COLOR12, 0x0001 }, { R_COLOR13, 0x0809 },
    { R_COLOR14, 0x0009 }, { R_COLOR15, 0x0009 }, { R_COLOR16, 0x0001 }, { R_COLOR17, 0x0809 },
    { R_COLOR18, 0x0008 }, { R_COLOR19, 0x0008 }, { R_COLOR1A, 0x0000 }, { R_COLOR1B, 0x0808 },
    { R_COLOR1C, 0x0089 }, { R_COLOR1D, 0x0089 }, { R_COLOR1E, 0x0081 }, { R_COLOR1F, 0x0889 },
    /* window size. */
    { R_DIWSTRT, 0x2c81 }, { R_BPLCON0, 0x0000 }, { R_DIWSTOP, 0x2c81 },
    /* datafetch */
    { R_DDFSTRT, 0x0038 }, { R_DDFSTOP, 0x00b8 },
    { R_BPLCON1, 0x0000 },
    { R_BPL1MOD, 0x00bc }, { R_BPL2MOD, 0x00bc },
    /* bitplanes */
    { R_BPL0PTH, 0x0000 }, { R_BPL0PTL, 0x0000 },
    { R_BPL1PTH, 0x0000 }, { R_BPL1PTL, 0x0000 },
    { R_BPL2PTH, 0x0000 }, { R_BPL2PTL, 0x0000 },
    { R_BPL3PTH, 0x0000 }, { R_BPL3PTL, 0x0000 }, 
#if defined (GRF_ECS)
    { R_DIWHIGH, 0x2100 },
#endif
    { R_COP1LCH, 0x0000 }, { R_COP1LCL, 0x0000 },
    { 0xffff, 0xfffe }, { 0xffff, 0xfffe }  /* COPEND, COPEND */
};
int std_pal_a2024_copper_list_len = sizeof (std_pal_a2024_copper_list) / sizeof (cop_t);
int std_pal_a2024_copper_list_size = sizeof (std_pal_a2024_copper_list);

#endif /* GRF_A2024 */






