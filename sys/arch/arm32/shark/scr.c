/*	$NetBSD: scr.c,v 1.9 2001/05/02 10:32:14 scw Exp $	*/

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
**++
** 
**  FACILITY:
**
**    Driver for smart card
**
**  ABSTRACT:
**
**    The driver provides access to a Smart Card for the DNARD.  
**
**    There is no Smart Card silicon.  Several i/o pins
**    connect to the pads on the Smart Card, and the driver is 
**    is responsible for driving the signals in accordance with 
**    ISO 7816-3 (the Smart Card spec)
**
**    This driver puts a high load on the system due to the need
**    to interrupt at a high rate (up to 50 Khz) during bit detection.
**     
**
**    The driver is dived into the standard top half ioctl, and bottom
**    half interupt.  The interrupt is FIQ, which requires its own stack.  
**    disable_interrupts and restore_interrupts must be used to protect from
**    a FIQ.  Since splxxx functions do not use this, the bottom half cannot
**    use any standard functions (ie like wakeup, timeout, etc.  
**    Thus the communication from the bottom half
**    to the top half uses a "done" bit called masterDone.  This bit
**    is set by the master state machine when all bottom half work is 
**    complete.  The top half checks/sleeps on this masterDone bit.
**
**    The FIQ is driven by Timer 2 (T2)in the sequoia.  All times are
**    referenced to T2 counts.
**
**    The bottom half is done as a several linked state machines.  
**    The top level machine is the maserSM (ie master State Machine).  This 
**    machine calls mid level protocol machines, ie ATRSM (Answer To Reset 
**    State Machine), t0SendSM (T=0 Send State Machine), and t0RecvSM (T=0 Recv 
**    State Machine).  These mid level protocol machines in turn call low level
**    bit-bashing machines, ie coldResetSM, t0SendByteSM, T0RecvByteSM.
**
**    Smart Cards are driven in a command/response mode.  Ie you issue a command
**    to the Smart Card and it responds.  This command/response mode is reflected
**    in the structure of the driver.  Ie the ioctl gets a command, it 
**    gives it to the bottom half to execute and goes to sleep.  The bottom half
**    executes the command and gets the response to from the card and then
**    notifies the top half that it has completed.  Commands usually complete
**    in under a second.  
**
** 
**
**  AUTHORS:
**
**    E. J. Grohn
**    Digital Equipment Corporation.
**
**  CREATION DATE:
**
**    27-July-97
**
**--
*/

/*
**
**  INCLUDE FILES
**
*/

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
/* #include <sys/select.h> */
/* #include <sys/tty.h> */
#include <sys/proc.h>
/* #include <sys/user.h> */
#include <sys/conf.h>
/* #include <sys/file.h> */
/* #include <sys/uio.h> */
#include <sys/kernel.h>
/* #include <sys/syslog.h> */
#include <sys/types.h>
#include <sys/device.h>
#include <dev/isa/isavar.h>
#include <machine/cpufunc.h>


/* SCR_DEBUG is the master switch for turning on debugging */        
//#define SCR_DEBUG 1     
#ifdef SCR_DEBUG
    #define KERNEL_DEBUG 
    #ifdef DDB
        #define DEBUGGER printf("file = %s, line = %d\n",__FILE__,__LINE__);Debugger()        
    #else
        #define DEBUGGER panic("file = %s, line = %d\n",__FILE__,__LINE__);
    #endif
#else
    #define DEBUGGER
#endif


#include <machine/kerndebug.h>
//#include <machine/intr.h>
#include <dev/ic/i8253reg.h>
#include <arch/arm32/shark/hat.h>
#include <arm32/shark/sequoia.h>
#include <machine/scrio.h>




/*
**
**  MACRO DEFINITIONS
**
*/


#define scr_lcr scr_cfcr

/* 
** Macro to extract the minor device number from the device Identifier 
*/
#define SCRUNIT(x)      (minor(x))

/* 
** Macros to clear/set/test bit flags. 
*/
#define SET(t, f)       (t) |= (f)
#define CLR(t, f)       (t) &= ~(f)
#define ISSET(t, f)     ((t) & (f))
#define ISCLR(t, f)     ( ((t) & (f)) == 0)


/*
** some macros to assist in debugging
*/
#ifdef SCR_DEBUG
    #define KERNEL_DEBUG
    #define ASSERT(f)	        do { if (!(f)) { DEBUGGER;} }while(0)
    #define TOGGLE_TEST_PIN()   scrToggleTestPin()
    #define INVALID_STATE_CMD(sc,state,cmd)  invalidStateCmd(sc,state,cmd,__LINE__); 
#else
    #define ASSERT(f)
    #define TOGGLE_TEST_PIN()
    //#define INVALID_STATE_CMD(sc,state,cmd)  panic("scr: invalid state/cmd, sc = %X, state = %X, cmd = %X, line = %d\n",sc,state,cmd,__LINE__);
    #define INVALID_STATE_CMD(sc,state,cmd)  sc->bigTrouble = TRUE;

#endif

#ifndef FALSE
    #define FALSE 0
#endif

#ifndef TRUE
    #define TRUE 1
#endif





/* 
** The first and last bytes of the debug control variables is reserved for
** the standard KERN_DEBUG_xxx macros, so we can tailor the middle two bytes
*/
#define SCRPROBE_DEBUG_INFO			0x00000100
#define SCRATTACH_DEBUG_INFO		0x00000200
#define SCROPEN_DEBUG_INFO			0x00000400
#define SCRCLOSE_DEBUG_INFO			0x00000800
#define SCRREAD_DEBUG_INFO			0x00001000
#define SCRWRITE_DEBUG_INFO			0x00002000
#define SCRIOCTL_DEBUG_INFO			0x00004000
#define MASTER_SM_DEBUG_INFO		0x00008000
#define COLD_RESET_SM_DEBUG_INFO	0x00010000
#define ATR_SM_DEBUG_INFO			0x00020000
#define T0_RECV_BYTE_SM_DEBUG_INFO	0x00040000	
#define T0_SEND_BYTE_SM_DEBUG_INFO	0x00080000	
#define T0_RECV_SM_DEBUG_INFO		0x00100000	
#define T0_SEND_SM_DEBUG_INFO		0x00200000	


int scrdebug =  //SCRPROBE_DEBUG_INFO	    |
                //SCRATTACH_DEBUG_INFO	|
                //SCROPEN_DEBUG_INFO		|
                //SCRCLOSE_DEBUG_INFO		|
                //SCRREAD_DEBUG_INFO		|
                //SCRWRITE_DEBUG_INFO		|
                //SCRIOCTL_DEBUG_INFO		|
                //MASTER_SM_DEBUG_INFO	|
                //COLD_RESET_SM_DEBUG_INFO|
                //ATR_SM_DEBUG_INFO		|
                //T0_RECV_BYTE_SM_DEBUG_INFO |
                //T0_SEND_BYTE_SM_DEBUG_INFO  |
                //T0_RECV_SM_DEBUG_INFO		|
                //T0_SEND_SM_DEBUG_INFO		|
                0;






/*
** the bottom half of the driver is done as several linked state machines
** below are all the states of the machines, and the commands that are 
** sent to each machine
*/

/* commands to Master State Machine from ioctl */
#define 	mcOn				0x0100		/* ioctl on */
#define		mcT0DataSend		0x0102		/* ioctl send */
#define		mcT0DataRecv		0x0103		/* ioctl recv */

/* commands to Master State Machine from lower state machines */
#define		mcColdReset			0x0105		/* cold reset finished  */
#define		mcATR				0x0106		/* ATR has finished */
#define		mcT0Send			0x0108		/* T0 send finished */
#define		mcT0Recv			0x010a		/* T0 recv finished */

/* states in Master state machine (ms = Master State) */
#define		msIdleOff		    0x0200      /* in idle state, card powered off */    
#define		msColdReset		    0x0201      /* turning on power, clock, reset */
#define		msATR			    0x0202      /* getting ATR sequence from card */
#define		msIdleOn		    0x0203      /* idle, put card powered on */
#define		msT0Send		    0x0204      /* sending T0 data */
#define		msT0Recv		    0x0205      /* recving T0 data */




/* commands to T0 send state machine  */
#define 	t0scStart			0x0300      /* start  */
#define		t0scTWorkWaiting	0x0301      /* work waiting timeout */

/* states in T0 send state machine */
#define 	t0ssIdle			0x0400      /* idle state */
#define		t0ssSendHeader		0x0401		/* send 5 header bytes */
#define		t0ssRecvProcedure	0x0402      /* wait for procedure byte */
#define		t0ssSendByte		0x0403      /* send 1 byte */
#define		t0ssSendData		0x0404      /* send all bytes */
#define		t0ssRecvSW1			0x0405      /* wait for sw1 */
#define		t0ssRecvSW2			0x0406      /* wait for sw2 */





/* commands to T0 recv state machine */     
#define 	t0rcStart			0x0500      /* start */
#define		t0rcTWorkWaiting	0x0501      /* work waiting timeout */

/* states in T0 recv state machine */
#define 	t0rsIdle			0x0600      /* idle state */
#define		t0rsSendHeader		0x0601		/* send 5 header bytes */
#define		t0rsRecvProcedure	0x0602      /* wait for procedure byte */
#define		t0rsRecvByte		0x0603      /* recv 1 byte */
#define		t0rsRecvData		0x0604      /* recv all bytes */
#define		t0rsRecvSW1			0x0605      /* wait for sw1 */
#define		t0rsRecvSW2			0x0606      /* wait for sw2 */



/* commands to Cold Reset state machine */
#define		crcStart		    0x0900      /* start */
#define		crcT2			    0x0902		/* timeout T2 ISO 7816-3, P6, Figure 2 */

/* states in cold reset state machine */
#define		crsIdle			    0x0a00      /* idle */
#define		crsT2Wait		    0x0a01      /* wait for T2 ISO 7816-3.P6. Figure 2 */





/* commands to Answer To Reset (ATR) state machine */
#define		atrcStart			0x0b00      /* start */
#define		atrcT3				0x0b04      /* got T3 timeout */
#define		atrcTWorkWaiting	0x0b05      /* work waiting timeout */

/* states in Answer To Reset (ATR) state machine */
#define		atrsIdle		    0x0c00      /* idle */
#define		atrsTS			    0x0c01		/* looking for TS, (initial bytes)	*/
#define		atrsT0			    0x0c02		/* looking for T0, (format bytes)	*/
#define		atrsTABCD		    0x0c03		/* looking for TAx (interface bytes)*/
#define		atrsTK			    0x0c04		/* looking for TK  (history bytes)		*/
#define		atrsTCK			    0x0c05		/* looking for TCK (check bytes		*/


/* commands to T0 Recv Byte state machine */
#define		t0rbcStart			0x0d00      /* start */
#define		t0rbcAbort			0x0d01      /* abort */
#define		t0rbcTFindStartEdge	0x0d02		/* start bit edge search */
#define		t0rbcTFindStartMid	0x0d03		/* start bit mid search */
#define		t0rbcTClockData		0x0d04		/* data bit search */
#define		t0rbcTErrorStart	0x0d05		/* start to send error  */
#define		t0rbcTErrorStop		0x0d06		/* stop sending error	*/ 

/* states in T0 Recv Byte state machine */
#define		t0rbsIdle			0x0e00      /* idle */    
#define		t0rbsFindStartEdge  0x0e01		/* looking for start bit */
#define		t0rbsFindStartMid   0x0e02		/* looking for start bit */
#define		t0rbsClockData		0x0e03		/* looking for data bits */
#define		t0rbsSendError		0x0e04		/* output error bit */




/* commands to T0 Send Byte state machine */
#define		t0sbcStart			0x0f00      /* start the machine */
#define		t0sbcAbort			0x0f01      /* abort the machine */
#define		t0sbcTGuardTime		0x0f02		/* guard time finished */
#define		t0sbcTClockData		0x0f03		/* clock time finished     */
#define		t0sbcTError			0x0f04		/* start to send error  */
#define		t0sbcTResend		0x0f05		/* if parity error, then wait unfill we can re-send */



/* states in T0 Send Byte state machine */
#define		t0sbsIdle			0x1000      /* idle */
#define		t0sbsWaitGuardTime	0x1001		/* wait for guard time to finish */
#define		t0sbsClockData		0x1002		/* clocking out data & parity */
#define		t0sbsWaitError		0x1003		/* waiting for error indicator */
#define		t0sbsWaitResend		0x1004		/* waiting to start re-send if error */




/* 
** generic middle level state machine commands 
** sent by T0 Send Byte & T0 recv Byte to T0 Send and T0 Recv
*/
#define		gcT0RecvByte		0x1100      /* receive finished */
#define		gcT0RecvByteErr		0x1101      /* receive got error */                        
#define		gcT0SendByte		0x1102      /* send finished */
#define		gcT0SendByteErr		0x1103      /* send got error */






/* 
**
** below are definitions associated with Smart Card
**
*/ 


/* 
** Frequency of clock sent to card
** NCI's card is running at 1/2 freq, so in debug we can make
** use of this to toggle more debug signals and still be within
** interrupt time budget 
*/
#ifdef  SCR_DEBUG
    #define CARD_FREQ_DEF			(3579000/2)
#else
    #define CARD_FREQ_DEF			(3579000) 
#endif



/* byte logic level and msb/lsb coding */
#define CONVENTION_UNKOWN		        0
#define CONVENTION_INVERSE		        1
#define CONVENTION_DIRECT		        2    
#define CONVENIONT_INVERSE_ID			0x3f
#define CONVENTION_DIRECT_FIX			0x3b
#define CONVENTION_DIRECT_ID			0x23


/* macros that help us set the T2 count for bit bashing */
#define CLK_COUNT_START	        (((372   * TIMER_FREQ) / sc->cardFreq) /5)
#define CLK_COUNT_DATA	        (((372   * TIMER_FREQ) / sc->cardFreq)   )
#define START_2_DATA            5

/* default settings to use if not specified in ATR */
#define N_DEFAULT			    0		/* guard time default */
#define Fi_DEFAULT			    372		/* clock rate conversion default */			
#define Di_DEFAULT			    1		/* bit rate adjustment factor */
#define Wi_DEFAULT			    10		/* waiting time */


/* table for clock rate adjustment in ATR */
int FI2Fi[16] = {372, 372, 558, 744,1116,1488,1860,   0,
                   0, 512, 768,1024,1536,2048,   0,   0};

/* table for bit rate adjustment   in ATR*/
int DI2Di[16] = {  0,   1,   2,   4,   8,   16, 32,   0,
                  12,  20,   0,   0,   0,    0,  0,   0};

/* values of atrY in the ATR sequence*/
#define ATR_Y_TA	0x10
#define ATR_Y_TB	0x20
#define ATR_Y_TC	0x40
#define ATR_Y_TD	0x80

/* T0,T1,etc  information in ATR sequence*/
#define PROTOCOL_T0 0x0001			/* bit 0 for T0 */
#define PROTOCOL_T1 0x0002			/* bit 1 for T1 */
#define PROTOCOL_T2 0x0004			/* bit 2 for T2*/
#define PROTOCOL_T3 0x0008			/* bit 3 for T3*/
/* etc  */


/* timeouts for various places - see ISO 7816-3 */
#define T_t2					((300   * TIMER_FREQ) / sc->cardFreq)	  
#define T_t3					((40000 * (TIMER_FREQ/1024)) / (sc->cardFreq/1024)) 
#define T_WORK_WAITING			(((960 * sc->Wi * sc->Fi ) / (sc->cardFreq/1024))  * (TIMER_FREQ/1024))
#define PARITY_ERROR_MAX 3		/* maximum parity errors on 1 byte before giving up  */

/* 
** its possible for the HAT to wedge.  If that happens, all timing is sick, so 
** we use timeout below (driven of system sleeps) as a "watchdog" 
*/
#define MAX_FIQ_TIME     5      /* maximum time we are willing to run the FIQ */ 


/* used to decode T0 commands */
#define CMD_BUF_INS_OFF		    1	    /* offset to INS in header */
#define CMD_BUF_DATA_LEN_OFF    4	    /* offset to data length in header */


/*
**
** DATA STRUCTURES
**
*/                                     
typedef unsigned char BYTE;

/* our soft c structure */
struct scr_softc 
{
    struct device       dev;
    int                 open;

    /* configuration information */
    int     status;                 /* status to be returned */
    int     cardFreq;               /* freq supplied to card */
    int     convention;             /* ie direct or inverse */
    int     protocolType;           /* bit 0 indicates T0, bit 1 indicates T1,etc */
    int     N;                      /* guard time  */
    int     Fi;                     /* clock rate */
    int     Di;                     /* bit rate adjustment */
    int     Wi;                     /* work waiting time */
    int     clkCountStartRecv;      /* count for clock start bits on recv */
    int     clkCountDataRecv;       /* count for clock data  bits on recv*/
    int     clkCountDataSend;       /* count for clock data  bits on send */

    /* state machines */
    int     masterS ;
    int     t0RecvS;
    int     t0SendS;
    int     coldResetS;
    int     ATRS;
    int     t0RecvByteS;
    int     t0SendByteS;

    /* extra stuff kept for t0send state machine */
    int     commandCount;           /* number of command bytes sent */
    int     dataCount;              /* number of data bytes send/recv */
    int     dataMax;                /* max number of data bytes to send/recv */

    /* extra stuff kept for t0RecvByteS, t0SendByteS machines */
    void    (*t0ByteParent) __P((struct scr_softc *,int));  /* state machine that is controlling this SM */
    int     shiftBits;              /* number of bits shifted	*/
    BYTE    shiftByte;              /* intermediate value of bit being shifted */
    BYTE    dataByte;               /* actual value of byte */
    int     shiftParity;            /* value of parity */
    int     shiftParityCount;       /* number of retries due to parity error */

    /* extra stuff kept for ATR machine */
    int     atrY;       /* indicates if TA,TB,TC,TD is to follow */
    int     atrK;       /* number of historical characters*/
    int     atrKCount;  /* progressive could of historical characters*/
    int     atrTABCDx;  /* the 'i' in TA(i), TB(i), TC(i) and TD(i) */
    int     atrFi;      /* value of Fi */
    int     atrDi;      /* value of Di */

    int masterDone; /* flag used by bottom half to tell top half its done */
    int bigTrouble; /* david/jim, remove this when the dust settles  */

    /* pointers used by ioctl */        
    ScrOn * pIoctlOn;   /* pointer to on ioctl data */
    ScrT0 * pIoctlT0;   /* pointer to T0 ioctl data */
};

/* number of devices */
static int devices = 0;

/* used as reference for tsleep */
static int tsleepIdent;   


/* 
** only 1 device is using the hat at any one time
** variable below must be acquired using splhigh before using the hat
*/
static int hatLock = FALSE;     




/* 
** data structures associated with our timeout queue that we run for the 
** bottom half of the driver 
*/

/* timeout callout structure */
typedef struct callout_t
{
    struct callout_t *c_next;                       /* next callout in queue */
    struct scr_softc *c_sc;                         /* soft c */
    int     c_arg;                                  /* function argument */
    void    (*c_func) __P((struct scr_softc*,int)); /* function to call */
    int     c_time;                                 /* ticks to the event */
}Callout;

/* actual callout array */
#define  SCR_CLK_CALLOUT_COUNT 10       
static Callout  scrClkCalloutArray[SCR_CLK_CALLOUT_COUNT];

/* callout lists */
static Callout *scrClkCallFree;                     /* free queue */
static Callout  scrClkCallTodo;                     /* todo queue */

/* 
** information kept for the clock/FIQ that drives our timeout queue
*/
static int scrClkEnable = 0;                   /* true if clock enabled */
static void myHatWedge(int nFIQs);             /* callback that informs us if FIQ has wedged */
static int scrClkCount;                        /* number used to set t2 that drives FIQ */

#define HATSTACKSIZE 1024                       /* size of stack used during a FIQ */
static unsigned char hatStack[HATSTACKSIZE];   /* actual stack used during a FIQ */












/*
**
**  FUNCTIONAL PROTOTYPES
**
*/

/*
**
** functions in top half of driver 
**
*/

/* configure routines */
int     scrprobe    __P((struct device *, void *, void *));
void    scrattach   __P((struct device *, struct device *, void *));

/* driver entry points routines */
int     scropen     __P((dev_t dev, int flag, int mode, struct proc *p));
int     scrclose    __P((dev_t dev, int flag, int mode, struct proc *p));
int     scrread     __P((dev_t dev, struct uio *uio, int flag));
int     scrwrite    __P((dev_t dev, struct uio *uio, int flag));
int     scrioctl    __P((dev_t dev, u_long cmd, caddr_t data, int flag, struct proc  *p));
void    scrstop     __P((struct tty *tp, int flag));

static void   initStates           __P((struct scr_softc * sc)); 




/*
**
** functions in bottom half of driver
**
*/

/* top level state machine */
static void   masterSM             __P((struct scr_softc * sc,int cmd));

/* mid level state machines, ie protocols  */
static void   t0SendSM             __P((struct scr_softc * sc,int cnd));
static void   t0RecvSM             __P((struct scr_softc * sc,int cnd));
static void   ATRSM                __P((struct scr_softc * sc,int cnd));

/* low level state machines, ie bash hardware bits */
static void   coldResetSM          __P((struct scr_softc * sc,int cnd));

static void   t0SendByteSM         __P((struct scr_softc * sc,int cnd));
static void   t0RecvByteSM         __P((struct scr_softc * sc,int cnd));

static void   cardOff              __P((struct scr_softc * sc));              

/* 
** functions used for our own timeout routines.
** we cannot use system ones as we are running at a spl level
** that can interrupt the system timeout routines
*/
static void scrClkInit     __P((void));
static void scrClkStart    __P((struct scr_softc* sc,int countPerTick));
static void scrClkAdj      __P((int count));
static void scrClkStop     __P((void));
static void hatClkIrq      __P((int count));               

static void scrTimeout     __P((void (*func)(struct scr_softc*,int), struct scr_softc*, int arg, int count));  
static void scrUntimeout   __P((void (*func)(struct scr_softc*,int), struct scr_softc*, int arg));


/* debug functions */
#ifdef SCR_DEBUG
    static void invalidStateCmd __P((struct scr_softc* sc,int state,int cmd, int line));
    static char * getText       __P((int x));
#endif



/* Declare the cdevsw and bdevsw entrypoint routines 
*/
cdev_decl(scr);
bdev_decl(scr);


struct cfattach scr_ca =
{
        sizeof(struct scr_softc), (cfmatch_t)scrprobe, scrattach
};

extern struct cfdriver scr_cd;


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      scrProbe
**     
**     This is the probe routine for the Smart Card.  Because the 
**     Smart Card is hard wired, there is no probing to peform.  The
**     function ensures that a succesfull problem occurs only once.
**
**  FORMAL PARAMETERS:
**
**     parent  - input  : pointer to the parent device 
**     match   - not used
**     aux     - output : pointer to an isa_attach_args structure.
**     
**  IMPLICIT INPUTS:
**
**     none.
**
**  IMPLICIT OUTPUTS:
**
**     none.
**
**  FUNCTION VALUE:
**
**     0 - Probe failed to find the requested device.
**     1 - Probe successfully talked to the device. 
**
**  SIDE EFFECTS:
**
**     none.
**--
*/
int scrprobe(parent, match, aux)
    struct  device  *parent;
    void            *match;
    void            *aux;
{
    struct isa_attach_args  *ia = aux;
    int                     rv = 0;           

    KERN_DEBUG (scrdebug, SCRPROBE_DEBUG_INFO,("scrprobe: called, name = %s\n",
                                               parent->dv_cfdata->cf_driver->cd_name));

    if (strcmp(parent->dv_cfdata->cf_driver->cd_name, "ofisascr") == 0 &&
        devices == 0)
    {
        /* set "devices" to ensure that we respond only once */
        devices++;      

        /* tell the caller that we are not using any resource */
        ia->ia_iosize = -1;
        ia->ia_irq   = -1;
        ia->ia_msize = 0;
        rv = 1;


        KERN_DEBUG (scrdebug, SCRPROBE_DEBUG_INFO,("scrprobe: successful \n"));

    } 


    return (rv);

} /* End scrprobe() */


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      scrattach
**
**      Initialize the clock and state machines 
**
**  FORMAL PARAMETERS:
**
**      parent - input  : pointer to my parents device structure.
**      self   - output : pointer to my softc with device structure at front.
**      aux    - input  : pointer to the isa_attach_args structure.
**
**  IMPLICIT INPUTS:
**
**      nill
**
**  IMPLICIT OUTPUTS:
**
**      scrconsinit - clock callout functions set
**                    state machines all at idle 
**
**  FUNCTION VALUE:
**
**      none.
**
**  SIDE EFFECTS:
**
**      none.
**--
*/
void scrattach(parent, self, aux)
    struct device *parent;
    struct device *self;
    void         *aux;
{
    struct scr_softc       *sc = (void *)self;

    printf("\n");
    if (!strcmp(parent->dv_cfdata->cf_driver->cd_name, "ofisascr"))
    {
        KERN_DEBUG (scrdebug, SCRATTACH_DEBUG_INFO,("scrattach: called \n"));

        /* set initial state machine values */
        scrClkInit();
        initStates(sc);
        sc->open = FALSE;
    } 

    else
    {
        panic("scrattach: not on an ISA bus, attach impossible");
    } /* End else we aren't on ISA and we can't handle it */


    return;
} /* End scrattach() */


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      initStates
**
**      sets the state of all machines to idle
**
**  FORMAL PARAMETERS:
**
**      sc    -  Pointer to the softc structure.
**
**  IMPLICIT INPUTS:
**
**      nill
**
**  IMPLICIT OUTPUTS:
**
**      nill
**
**  FUNCTION VALUE:
**
**      nill
**
**  SIDE EFFECTS:
**
**      nill
**--
*/
static void initStates(struct scr_softc * sc)
{
    sc->masterS         = msIdleOff;    
    sc->t0RecvS         = t0rsIdle;         
    sc->t0SendS         = t0ssIdle;         
    sc->coldResetS      = crsIdle;          
    sc->ATRS            = atrsIdle;         
    sc->t0RecvByteS     = t0rbsIdle;            
    sc->t0SendByteS     = t0sbsIdle;            
}



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     scrOpen
**
**     Opens the driver.  We only let the device be opened
**     once for security reasons
**
**  FORMAL PARAMETERS:
**
**     dev  - input : Device identifier consisting of major and minor numbers.
**     flag - input : Indicates if this is a blocking I/O call.
**     mode - not used.
**     p    - input : Pointer to the proc structure of the process 
**            performing the open. 
**
**  IMPLICIT INPUTS:
**
**     none.
**
**  IMPLICIT OUTPUTS:
**
**     none.
**
**  FUNCTION VALUE:
**
**     ENXIO   - invalid device specified for open.
**     EBUSY   - The unit is already open
**
**  SIDE EFFECTS:
**
**     none.
**--
*/
int scropen(dev, flag, mode, p)
    dev_t       dev;
    int         flag;
    int         mode;
struct proc *p;
{
    int                  unit = SCRUNIT(dev);
    struct scr_softc     *sc;

    KERN_DEBUG (scrdebug, SCROPEN_DEBUG_INFO,
                ("scropen: called with minor device %d and flag 0x%x\n",
                 unit, flag));

    /* Sanity check the minor device number we have been instructed
    ** to open and set up our softc structure pointer. 
    */
    if (unit >= scr_cd.cd_ndevs)
    {
        KERN_DEBUG (scrdebug, SCROPEN_DEBUG_INFO,("\t scropen, return ENXIO\n"));
        return (ENXIO);
    }
    sc = scr_cd.cd_devs[unit];
    if (!sc)
    {
        KERN_DEBUG (scrdebug, SCROPEN_DEBUG_INFO,("\t scropen, return ENXIO\n"));
        return (ENXIO);
    }


    // david,jim - remove ifdef this when NCI can cope with only 1 open
#if 0   
    if (sc->open)
    {

        KERN_DEBUG (scrdebug, SCROPEN_DEBUG_INFO,("\t scropen, return EBUSY\n"));
        return (EBUSY);
    }


    /* set all initial conditions */
    sc->open = TRUE;
#endif

    KERN_DEBUG (scrdebug, SCROPEN_DEBUG_INFO,("scropen: success \n"));
    /* Now invoke the line discipline open routine 
    */

    return 0;

} /* End scropen() */


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     This function closed the driver
**
**  FORMAL PARAMETERS:
**
**     dev  - input : Device identifier consisting of major and minor numbers.
**     flag - Not used.
**     mode - Not used.
**     p    - Not used. 
**
**  IMPLICIT INPUTS:
**
**     scr_cd  - used to locate the softc structure for the device unit
**               identified by dev.
**
**  IMPLICIT OUTPUTS:
**
**     The device is put into an idle state.
**
**  FUNCTION VALUE:
**
**     0 - Always returns success.
**
**  SIDE EFFECTS:
**
**     none.
**--
*/
int scrclose(dev, flag, mode, p)
    dev_t       dev;
    int         flag;
    int         mode;
    struct proc *p;
{
#if 0
    int                unit = SCRUNIT(dev);
    struct scr_softc   *sc  = scr_cd.cd_devs[unit];
#endif

    KERN_DEBUG (scrdebug, SCRCLOSE_DEBUG_INFO,
                ("scrclose: called for minor device %d flag 0x%x\n",
                 SCRUNIT(dev), flag));

    // david,jim - remove ifdef this when NCI can cope with only 1 open
#if 0   
    /* Check we are open in the first place
    */
    if (sc->open)
    {
        /* put everything in the idle state */
        scrClkInit();
        initStates(sc);
        sc->open = FALSE;

    } 
    
    
    else
    {
        KERN_DEBUG (scrdebug, SCRCLOSE_DEBUG_INFO,("\t scrclose, device not open\n"));
    }
#endif

    KERN_DEBUG (scrdebug, SCRCLOSE_DEBUG_INFO,("scrclose exiting\n"));
    return(0);
}



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      scrwrite
**
**      not supported
**
**  FORMAL PARAMETERS:
**      
**      dev  - input : Device identifier consisting of major and minor numbers.
**      uio  - input : Pointer to the user I/O information (ie. write data).
**      flag - input : Information on how the I/O should be done (eg. blocking
**                     or non-blocking).
**
**  IMPLICIT INPUTS:
**
**
**  IMPLICIT OUTPUTS:
**
**      none
**
**  FUNCTION VALUE:
**
**      Returns ENODEV
**
**  SIDE EFFECTS:
**
**      none
**--
*/
int
scrwrite(dev, uio, flag)
dev_t      dev;
struct uio *uio;
int        flag;
{
    return ENODEV;
} 



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      scrread
**
**      not supported
**
**  FORMAL PARAMETERS:
**      
**      dev  - input : Device identifier consisting of major and minor numbers.
**      uio  - input : Pointer to the user I/O information (ie. read buffer).
**      flag - input : Information on how the I/O should be done (eg. blocking
**                     or non-blocking).
**
**  IMPLICIT INPUTS:
**
**
**  IMPLICIT OUTPUTS:
**
**      none
**
**  FUNCTION VALUE:
**
**      Returns ENODEV
**
**  SIDE EFFECTS:
**
**      none
**--
*/
int
scrread(dev, uio, flag)
dev_t       dev;
struct uio  *uio;
int         flag;
{
    return ENODEV;
} 




/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      scrpoll
**
**      not supported
**
**  FORMAL PARAMETERS:
**      
**      dev  - input : Device identifier consisting of major and minor numbers.
**      events -input: Events to poll for
**      p    - input : Process requesting the poll.
**
**  IMPLICIT INPUTS:
**
**
**  IMPLICIT OUTPUTS:
**
**      none
**
**  FUNCTION VALUE:
**
**      Returns ENODEV
**
**  SIDE EFFECTS:
**
**      none
**--
*/
int
scrpoll(dev, events, p)
dev_t       dev;
int         events;
struct proc *p;
{
    return ENODEV;
} 




/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     scrstop
**
**     should not be called
**  
**  FORMAL PARAMETERS:
**
**     tp   - Pointer to our tty structure.
**     flag - Ignored.
**
**  IMPLICIT INPUTS:
**
**     none.
**
**  IMPLICIT OUTPUTS:
**
**     none.
**
**  FUNCTION VALUE:
**
**     none.
**
**  SIDE EFFECTS:
**
**     none.
**--
*/
void scrstop(tp, flag)
    struct tty *tp;
    int flag;
{
    panic("scrstop: not implemented");
} 




/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      tty
**
**      should not be called
**
**  FORMAL PARAMETERS:
**
**      dev - input : Device identifier consisting of major and minor numbers.
**
**  IMPLICIT INPUTS:
**
**
**  IMPLICIT OUTPUTS:
**
**      none
**
**  FUNCTION VALUE:
**
**      null
**
**  SIDE EFFECTS:
**
**      none.
**--
*/
struct tty * scrtty(dev)
    dev_t   dev;
{
    panic("scrtty: not implemented");
    return NULL;
} 





/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     This routine is responsible for performing I/O controls.
**
**      There are 4 commands.  Status, On, T0 and Off.  
**
**      Status checks to see if the card is inserted.  This command
**      does not use the state machines
**
**      On turns the card on and gets the ATR sequence from the card. 
**      This command does use the state machines
**
**      T0 is used to read and write the card.  This command does use
**      the state machines
**  
**      Off turns the card off.  This command does not use the state
**      machines. 
**
**     
**  FORMAL PARAMETERS:
**
**     dev - input :  Device identifier consisting of major and minor numbers.
**     cmd - input : The requested IOCTL command to be performed. 
**                   See scrio.h for details
**
**
**                   Bit Position      { 3322222222221111111111
**                                     { 10987654321098765432109876543210 
**                   Meaning           | DDDLLLLLLLLLLLLLGGGGGGGGCCCCCCCC
**
**                   D - Command direction, in/out/both.
**                   L - Command argument length.
**                   G - Command group, 't' used for tty.
**                   C - Actual command enumeration.
** 
**     data - input/output : Direction depends on the command.
**     flag - input : Not used by us but passed to line discipline and ttioctl
**     p    - input : pointer to proc structure of user.
**     
**  IMPLICIT INPUTS:
**
**     none.
**
**  IMPLICIT OUTPUTS:
**
**     sc->masterS      state of master state machine 
**
**
**  FUNCTION VALUE:
**
**      ENOTTY   if not correct ioctl
**
**
**  SIDE EFFECTS:
**
**--
*/
int
scrioctl(dev, cmd, data, flag, p)
    dev_t        dev;
    u_long       cmd;
    caddr_t      data;
    int          flag;
struct proc  *p;
{
    int                 unit = SCRUNIT(dev);
    struct scr_softc*   sc  = scr_cd.cd_devs[unit];
    
    int                 error = 0;          /* error value returned */
    int                 masterDoneRetries= 0;         /* nuber of times we looked at masterDone */
    int                 done;               /* local copy of masterDone */
    
    ScrStatus *         pIoctlStatus;       /* pointer to status ioctl */
    ScrOff *            pIoctlOff;          /* pointer to off ioctl */
    
    u_int               savedInts;          /* saved interrupts */
    int                 s;                  /* saved spl value */



    KERN_DEBUG (scrdebug, SCRIOCTL_DEBUG_INFO,
                ("scrioctl: called for device 0x%x, command 0x%lx, "
                 "flag 0x%x\n",
                 unit, cmd, flag));



    switch (cmd)
    {
        /* 
        ** get the status of the card, ie is it in, in but off, in and on 
        */
        case SCRIOSTATUS:
            pIoctlStatus = (ScrStatus*)data;
            if (scrGetDetect())
            {
                savedInts = disable_interrupts(I32_bit | F32_bit);
                if (sc->masterS == msIdleOn)
                {
                    pIoctlStatus->status = CARD_ON;   
                }
                else
                {
                    ASSERT(sc->masterS == msIdleOff);
                    pIoctlStatus->status = CARD_INSERTED;
                }
                restore_interrupts(savedInts);
            }

            else
            {
                pIoctlStatus->status = CARD_REMOVED;
            }
            break;



        /* 
        ** turn the card on and get the ATR sequence 
        */
        case SCRIOON:
            sc->pIoctlOn = (ScrOn*)data;
            // acquire the hat lock. 
            while (1)
            {
                s = splhigh();
                if(!hatLock)
                {
                    hatLock = TRUE;
                    splx(s);
                    break;
                }
                splx(s);
        
                tsleep(&tsleepIdent ,PZERO,"hat", 1); 
            }
            
            
            // check to see if the card is in
            if(!scrGetDetect())
            {
                initStates(sc);
                cardOff(sc);
                // do not  call scrClkInit() as it is idle already
                sc->pIoctlOn->status = ERROR_CARD_REMOVED;
            }

            
            // check to see if we are already on 
            else if(sc->masterS == msIdleOn)
            {
                sc->pIoctlOn->status = ERROR_CARD_ON;
            }
            
            // card was in, card is off, so lets start it 
            else             
            {   
                // set up the top half 
                sc->masterDone = FALSE;
                sc->bigTrouble = FALSE;    /* david/jim, remove this when the dust settles  */

                        
                
                // start bottom half
                scrClkStart (sc,400);
                savedInts = disable_interrupts(I32_bit | F32_bit);     
                masterSM(sc,mcOn);
                restore_interrupts(savedInts);
                
    

                // see if bottom half done 
                while (1)
                {
                    // check that we have not looped too many times 
                    if(masterDoneRetries >= MAX_FIQ_TIME * HZ)  
                    {
//printf("MAX_FIQ_TIME reached \n");
                        // big problems, so reset bottom 
                        savedInts = disable_interrupts(I32_bit | F32_bit);
                        scrClkInit();
                        initStates(sc);
                        cardOff(sc);
                        sc->status = ERROR_CARD_REMOVED;
                        sc->masterDone = TRUE;
                        restore_interrupts(savedInts);
                        // dont stop clock, done at bottom of case 
                    }
                    masterDoneRetries ++;

                    // get done bit 
                    savedInts = disable_interrupts(I32_bit | F32_bit);
                    done =  sc->masterDone;
                    restore_interrupts(savedInts);

                    // see if all done 
                    if(done)
                    {
                        break;
                    }

                    
                    // wait for a while 
                    tsleep(&tsleepIdent ,PZERO,"hat", 1);  
                }


                // stop bottom half
                scrClkStop();
    
    
                /* need to fix up count bits in non hat interrupt time, so */
                if (sc->status == ERROR_OK)
                {
                    sc->clkCountStartRecv = CLK_COUNT_START;  
                    sc->clkCountDataRecv  = sc->clkCountStartRecv * START_2_DATA;
                    sc->clkCountDataSend  = CLK_COUNT_DATA;
                }
    


                /* takes while to turn off all lines, so keep out of hat */
                if (sc->masterS != msIdleOn)
                {
                    cardOff(sc);
                }
                // get the status back from the state machine 
                sc->pIoctlOn->status = sc->status;
           
            
            }


            // release  the hat lock. 
            s = splhigh();
            ASSERT(hatlock);
            hatLock = FALSE;
            splx(s);
            
            // david,jim hack to stop ioctl memcpy problem, to be removed when problem fixed ejg
            if (sc->pIoctlOn->status != ERROR_OK)
            {
                sc->pIoctlOn->atrLen = 0; 
            }
            break;
        
    
        /*
        ** turn the card off
        */
        case SCRIOOFF:
            pIoctlOff = (ScrOff*)data;
            // card off does not requires any  state processing, so do work here 
            initStates(sc);
            cardOff(sc);
            // do not  call scrClkInit() as it is idle already
            pIoctlOff->status = ERROR_OK;
            break;


        /*
        ** do a T0 read or write 
        */
        case SCRIOT0:
            sc->pIoctlT0 = (ScrT0*)data;
            
            // acquire the hat lock. 
            while (1)
            {
                s = splhigh();
                if(!hatLock)
                {
                    hatLock = TRUE;
                    splx(s);
                    break;
                }
                splx(s);
                
                tsleep(&tsleepIdent ,PZERO,"hat", 1); 
            }
            
            // check to see if the card is in
            if(!scrGetDetect())
            {
                initStates(sc);
                cardOff(sc);
                // do not  call scrClkInit() as it is idle already
                sc->pIoctlT0->status = ERROR_CARD_REMOVED;
            }

            
            // check to see if card is off 
            else if(sc->masterS == msIdleOff)
            {
                sc->pIoctlT0->status = ERROR_CARD_OFF;
            }
            
            // card was in, card is on, lets do command
            else             

            {   
                // set up the top half 
                sc->masterDone = FALSE;
                sc->bigTrouble = FALSE;    /* david/jim, remove this when the dust settles  */
                
                // start bottom half
                scrClkStart (sc,sc->clkCountDataSend);
                savedInts = disable_interrupts(I32_bit | F32_bit); 
                if (sc->pIoctlT0->writeBuffer)
                {
                    masterSM(sc,mcT0DataSend);
                }
                else
                {
                    masterSM(sc,mcT0DataRecv);
                }
                restore_interrupts(savedInts);
    

               // see if bottom half done 
               while (1)
               {
                     // check that we have not looped too many times 
                     if(masterDoneRetries >= MAX_FIQ_TIME * HZ)  
                     {
//printf("MAX_FIQ_TIME reached \n");
                        // big problems, so reset bottom 
                        savedInts = disable_interrupts(I32_bit | F32_bit);
                        scrClkInit();
                        initStates(sc);
                        cardOff(sc);
                        sc->status = ERROR_CARD_REMOVED;
                        sc->masterDone = TRUE;
                        restore_interrupts(savedInts);
                     }
                     masterDoneRetries ++;
                    
                    
                    // get done bit 
                    savedInts = disable_interrupts(I32_bit | F32_bit);
                    done =  sc->masterDone;
                    restore_interrupts(savedInts);

                    
                    // see if all done 
                    if(done)
                    {
                        break;
                    }

                    
                    // wait for a while 
                    tsleep(&tsleepIdent ,PZERO,"hat", 1); 
                }

                // stop bottom half
                scrClkStop();
     
    

                // get the status back from the state machine 
                sc->pIoctlT0->status = sc->status;
            }
            
            
            // release  the hat lock. 
            s = splhigh();
            hatLock = FALSE;
            splx(s);
            
            
            
            // david, jim hack to stop ioctl memcpy problem, to be removed when problem fixed ejg
            if (sc->pIoctlT0->status != ERROR_OK)
            {
                sc->pIoctlT0->dataLen = 0;   
            }
            break;

        default:
            KERN_DEBUG (scrdebug, SCRIOCTL_DEBUG_INFO,("\t scrioctl: unknown command, ENOTTY \n"));
            error = ENOTTY;
            break;
    }


 
    KERN_DEBUG (scrdebug, SCRIOCTL_DEBUG_INFO,
                ("scrioctl: exiting with sc->status %d\n", error));
    return (error);
} /* End scrioctl */






/*
**
** All functions below this point are the bottom half of the driver
** 
** All are called during a FIQ, except for some functions in masterSM which 
** provides the interface between the bottom half and top half of
** the driver (nb masterDone() helps masterSM() out with this interface
** between top and bottom parts of the driver.
**
*/


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      masterSM
**
**      This state machine implements the top level state control  It 
**      receives commands to turn the card on, and do T0 reads and T0 writes
**      from the scrioctl.  It then calls mid level state machine to action
**      these commands.  
**
**      This machine is the only machine to keep state between scrioctl calls.  
**      Between calls, the state will be either msIdleOff, or msIdleOn.  msIdleOff
**      indicates that no signals are applied to the card.  msidleOn indicates that
**      power and clock are supplied to the card, and that the card has performed
**      a successful ATR sequence.
**
**      This routine gets called during FIQ interrupts and from scrioctl.  It is a
**      requirement that the scrioctl disables interrupts before calling this function.
**
**      NB:- there is no way for the machine to get from msIdleOn to msIdleOff.  Since
**      this is just a mater of turning all signals off and resetting state machines,
**      scrioctl takes a shortcut and resets everything itself.   Ie it hits everything
**      with a big hammer!!
**
**  FORMAL PARAMETERS:
**
**      sc      -  Pointer to the softc structure.
**      cmd     -  command to the state machine, can be from ioctl, or mid level SM
**      
**  IMPLICIT INPUTS:
**
**      sc->masterS     state of this machine
**      sc->pIoctlT0    pointer to T0 ioctl
**
**  IMPLICIT OUTPUTS:
**
**
**  FUNCTION VALUE:
**
**      nill
**
**  SIDE EFFECTS:
**
**      power and clock applied to card if successful ATR 
**--
*/
static void masterSM(struct scr_softc * sc,int cmd)
{

    if (sc->bigTrouble) return;     // david,jim , remove this when dust settles 

    switch (sc->masterS)
    {
        case msIdleOff: 
            switch (cmd)
            {
                case mcOn:  
                    if (scrGetDetect())
                    {
                        /* 
                        ** the card is off, and we want it on 
                        */
                        
                        /* set initial values */
                        sc->status          = 0;        
                        sc->convention      = CONVENTION_UNKOWN;            
                        sc->protocolType    = 0;        
                        sc->N               = N_DEFAULT;    
                        sc->Fi              = Fi_DEFAULT;   
                        sc->Di              = Di_DEFAULT;   
                        sc->Wi              = Wi_DEFAULT;   
                        sc->cardFreq        = CARD_FREQ_DEF;
                        sc->clkCountStartRecv = CLK_COUNT_START;  
                        sc->clkCountDataRecv  = sc->clkCountStartRecv * START_2_DATA;
                        sc->clkCountDataSend  = CLK_COUNT_DATA;
                        
                        /* get coldResetSM  to turn on power, clock, reset */
                        sc->masterS = msColdReset;
                        coldResetSM(sc,crcStart);
                    }
                    else
                    {
                        /* card not inserted, so just set status and give up */
                        sc->status = ERROR_CARD_REMOVED;
                        sc->masterDone = TRUE;
                    }
                    break;

                default:
                    INVALID_STATE_CMD(sc,sc->masterS,cmd);
                    break;
            }
            break;

        case msColdReset:
            switch (cmd)
            {
                case mcColdReset:   
                    /* 
                    ** coldResetSM has turned on power, clock , reset
                    ** tell ATRSM to get the ATR sequence from the card
                    */
                    sc->masterS = msATR;
                    ATRSM(sc,atrcStart);
                    break;

                default:
                    INVALID_STATE_CMD(sc,sc->masterS,cmd);
                    break;
            }
            break;

        case msATR:
            switch (cmd)
            {
                case mcATR:
                    /*
                    ** ATRSM has tried to get ATR sequence, so give 
                    ** back results to scrioctl.  ATR sequence data
                    ** was copied directly into ioctl data area, so 
                    ** no need to copy data 
                    */
                    if(sc->status == ERROR_OK)
                    {
                        sc->masterS = msIdleOn;
                    }
                    else
                    {
                        sc->masterS = msIdleOff;
                    }
                    sc->masterDone = TRUE;
                    break;


                default:
                    INVALID_STATE_CMD(sc,sc->masterS,cmd);
                    break;
            }
            break;

        case msIdleOn:
            switch (cmd)
            {
                // nb there is no command to go to the IdleOff state.  This
                // is a reset of the state machine, so is done in ioctl

                case mcT0DataSend:
                    /*
                    ** card is on, and we want to T0 Send, so 
                    ** as t0SendSM to do work 
                    */
                    sc->status  = ERROR_OK;        
                    sc->masterS = msT0Send;
                    t0SendSM(sc,t0scStart);
                    break;

                case mcT0DataRecv:
                    /*
                    ** card is on, and we want to T0 Recv, so 
                    ** as t0RecvSM to do work 
                    */
                    sc->status  = ERROR_OK;        
                    sc->masterS = msT0Recv;
                    t0RecvSM(sc,t0rcStart);
                    break;

                default:
                    INVALID_STATE_CMD(sc,sc->masterS,cmd);
                    break;
            }

            break;

        case msT0Send:
            switch (cmd)
            {
                case mcT0Send:
                    /*
                    ** t0SendSM has tried to send , so lets give back results
                    */
                    sc->masterS = msIdleOn;
                    sc->masterDone = TRUE;
                    break;

                default:
                    INVALID_STATE_CMD(sc,sc->masterS,cmd);
                    break;
            }
            break;

        case msT0Recv:
            switch (cmd)
            {
                case mcT0Recv:
                    /*
                    ** t0RecvSM has tried to recv , so lets give back results
                    ** data was written directly into ioctl data area, so we 
                    ** do not  need to copy any data 
                    */
                    sc->pIoctlT0->dataLen = sc->dataCount;
                    sc->masterS = msIdleOn;
                    sc->masterDone = TRUE;
                    break;

                default:
                    INVALID_STATE_CMD(sc,sc->masterS,cmd);
                    break;
            }
            break;

        default:
            INVALID_STATE_CMD(sc,sc->masterS,cmd);
            break;

    }
}




/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      t0SendSM
**
**      This is the T=0 Send State Machine.  It is responsible
**      for performing the send part of the ISO 7816-3 T=0 
**      protocol.  It is mid level protocol state machine. 
**
**      Once started, this machine is driven entirely via the 
**      FIQ/timeout structure .  
**
**      
**
**  FORMAL PARAMETERS:
**
**      sc      -  Pointer to the softc structure.
**      cmd     -  command to this machine 
**
**  IMPLICIT INPUTS:
**
**      sc->t0SendS             state of this machine 
**      sc->pIoctlT0->command   command to send to card
**      sc->pIoctlT0->data      data to send to card 
**
**  IMPLICIT OUTPUTS:
**
**      sc->status              error status from this machine 
**      sc->pIoctlT0->sw1       command status from card 
**      sc->pIoctlT0->sw2       command status from card 
**      sc->status              error status from this machine
**
**  FUNCTION VALUE:
**
**      nill
**
**  SIDE EFFECTS:
**
**      nill
**--
*/
static void   t0SendSM         (struct scr_softc * sc, int cmd)
{
    if (sc->bigTrouble) return;     // david,jim , remove this when dust settles 
    /*
    ** check for major failures that are common to most states
    */
    if (cmd == t0scTWorkWaiting ||
        cmd == gcT0RecvByteErr  || 
        cmd == gcT0SendByteErr 
        )
    {
        switch(cmd)
        {
            case t0scTWorkWaiting:
                ASSERT(sc->t0SendS != t0ssIdle); 
                
                /* kill all lower machines */
                t0SendByteSM(sc,t0sbcAbort);
                t0RecvByteSM(sc,t0rbcAbort);
        
                /* set status */
                sc->status = ERROR_WORK_WAITING;
                break;
    
            case gcT0RecvByteErr:   // fall through 
            case gcT0SendByteErr:
                scrUntimeout(t0SendSM, sc, t0scTWorkWaiting);
                // done set status, already set in lower machine 
                break;

            default: 
                INVALID_STATE_CMD(sc, sc->t0SendS,cmd);
                break;
        }

        /* change states */
        sc->t0SendS = t0ssIdle;   
        masterSM(sc,mcT0Send);
        return;
    }

    switch (sc->t0SendS)
    {
        case t0ssIdle:
            switch (cmd)
            {
                case t0scStart:
                    /* set initial values */
                    sc->t0SendS = t0ssSendHeader;
                    sc->t0ByteParent = t0SendSM;
                    sc->commandCount = 0;   
                    sc->dataCount = 0;
                    sc->dataMax = sc->pIoctlT0->command[CMD_BUF_DATA_LEN_OFF];
                    sc->dataByte = sc->pIoctlT0->command[sc->commandCount];

                    // get a byte 
                    t0SendByteSM(sc,t0sbcStart);
                    break;

                default:
                    INVALID_STATE_CMD(sc, sc->t0SendS,cmd);
                    break;
            }
            break;

        case t0ssSendHeader:
            switch (cmd)
            {
                case gcT0SendByte:
                    sc->commandCount++;
                    if (sc->commandCount < CMD_BUF_LEN)
                    {
                        sc->dataByte = sc->pIoctlT0->command[sc->commandCount];
                        t0SendByteSM(sc,t0sbcStart);
                    }
                    else
                    {
                        ASSERT(sc->commandCount == CMD_BUF_LEN);

                        sc->t0SendS = t0ssRecvProcedure;
                        scrTimeout(t0SendSM,sc,t0scTWorkWaiting,T_WORK_WAITING);
                        t0RecvByteSM(sc,t0rbcStart);
                    }
                    break;

                default:
                    INVALID_STATE_CMD(sc, sc->t0SendS,cmd);
                    break;
            }
            break;

        case t0ssRecvProcedure:
            switch (cmd)
            {
                case gcT0RecvByte:
                    scrUntimeout(t0SendSM, sc,t0scTWorkWaiting);
                    /* see if we should send all remaining bytes */
                    if ( (sc->dataByte == sc->pIoctlT0->command[CMD_BUF_INS_OFF])          ||
                         (sc->dataByte == (sc->pIoctlT0->command[CMD_BUF_INS_OFF] ^ 0x01))   )
                    {
                        ASSERT(sc->dataCount < sc->dataMax);
                        sc->t0SendS = t0ssSendData;
                        sc->dataByte = sc->pIoctlT0->data[sc->dataCount];
                        t0SendByteSM(sc,t0sbcStart);
                        sc->dataCount++;    
                    }

                    /* see if we should send one data byte */
                    else if ((sc->dataByte == (sc->pIoctlT0->command[CMD_BUF_INS_OFF]  ^ 0xFF)) ||
                             (sc->dataByte == (sc->pIoctlT0->command[CMD_BUF_INS_OFF]  ^ 0xFE)) )
                    {
                        ASSERT(sc->dataCount < sc->dataMax);
                        sc->t0SendS = t0ssSendByte;
                        sc->dataByte = sc->pIoctlT0->data[ sc->dataCount];
                        t0SendByteSM(sc,t0sbcStart);
                        sc->dataCount++;    
                    }

                    /* see if we should extend the work waiting period */
                    else if (sc->dataByte == 0x60)
                    {
                        t0RecvByteSM(sc,t0rbcStart);
                        scrTimeout(t0SendSM,sc,t0scTWorkWaiting,T_WORK_WAITING);
                    }

#ifdef ORIGINAL_SW1_CODE /* XXX XXX XXX cgd */
                    /* see if we have a SW1 byte */
                    else if ( ((sc->dataByte & 0xf0)  == 0x60) || ((sc->dataByte & 0xf0)  == 0x90)  
                              &&
                              sc->dataByte != 0x60)
#else /* XXX XXX XXX cgd */
                    /* see if we have a SW1 byte */
                    else if ( ( ((sc->dataByte & 0xf0)  == 0x60) || ((sc->dataByte & 0xf0)  == 0x90) )
                              &&
                              sc->dataByte != 0x60)
#endif /* XXX XXX XXX cgd */
                    {
                        sc->pIoctlT0->sw1 = sc->dataByte;
                        sc->t0SendS = t0ssRecvSW2;
                        t0RecvByteSM(sc,t0rbcStart);
                        scrTimeout(t0SendSM,sc,t0scTWorkWaiting,T_WORK_WAITING);
                    }

                    /* got bad data byte, log error and get out */
                    else
                    {
                        sc->status = ERROR_BAD_PROCEDURE_BYTE;

                        /* change state */
                        sc->t0SendS = t0ssIdle;
                        masterSM(sc,mcT0Send);
                    }
                    break;

                default:
                    INVALID_STATE_CMD(sc, sc->t0SendS,cmd);
                    break;
            }
            break;

        case t0ssSendByte:
            switch (cmd)
            {
                case gcT0SendByte:
                    if (sc->dataCount < sc->dataMax)
                    {
                        sc->t0SendS = t0ssRecvProcedure;
                    }

                    /* wait for sw1 byte */
                    else
                    {
                        ASSERT(sc->dataCount == sc->dataMax);
                        sc->t0SendS = t0ssRecvSW1;
                    }

                    // ask for another byte 
                    t0RecvByteSM(sc,t0rbcStart);
                    scrTimeout(t0SendSM,sc,t0scTWorkWaiting,T_WORK_WAITING);
                    break;

                default:
                    INVALID_STATE_CMD(sc, sc->t0SendS,cmd);
                    break;
            }
            break;

        case t0ssSendData:
            switch (cmd)
            {
                case gcT0SendByte:
                    /* send data */
                    if (sc->dataCount < sc->dataMax)
                    {
                        sc->t0SendS = t0ssSendData;
                        sc->dataByte = sc->pIoctlT0->data[ sc->dataCount];
                        t0SendByteSM(sc,t0sbcStart);
                        sc->dataCount++;    
                    }

                    /* wait for sw1 byte */
                    else
                    {
                        ASSERT(sc->dataCount == sc->dataMax);
                        sc->t0SendS = t0ssRecvSW1;
                        t0RecvByteSM(sc,t0rbcStart);
                        scrTimeout(t0SendSM,sc,t0scTWorkWaiting,T_WORK_WAITING);
                    }
                    break;

                default:
                    INVALID_STATE_CMD(sc, sc->t0SendS,cmd);
                    break;
            }
            break;

        case t0ssRecvSW1:
            switch (cmd)
            {
                case gcT0RecvByte:
                    scrUntimeout(t0SendSM, sc,t0scTWorkWaiting);
                    sc->pIoctlT0->sw1 = sc->dataByte;
                    sc->t0SendS = t0ssRecvSW2;
                    t0RecvByteSM(sc,t0rbcStart);
                    scrTimeout(t0SendSM,sc,t0scTWorkWaiting,T_WORK_WAITING);
                    break;
                default:
                    INVALID_STATE_CMD(sc, sc->t0SendS,cmd);
                    break;
            }
            break;

        case t0ssRecvSW2:
            switch (cmd)
            {
                case gcT0RecvByte:
                    scrUntimeout(t0SendSM, sc,t0scTWorkWaiting);
                    sc->pIoctlT0->sw2 = sc->dataByte;
                    sc->t0SendS = t0ssIdle;
                    masterSM(sc,mcT0Send);
                    break;

                default:
                    INVALID_STATE_CMD(sc, sc->t0SendS,cmd);
                    break;
            }
            break;

        default:
            INVALID_STATE_CMD(sc, sc->t0SendS,cmd);
            break;
    }
}



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      t0RecvSM
**
**      This is the T=0 Recv State Machine.  It is responsible
**      for performing the recv part of the ISO 7816-3 T=0 
**      protocol.   It is mid level protocol state machine.
**
**      Once started, this machine is driven entirely via the 
**      FIQ/timeout structure .  
**
**  FORMAL PARAMETERS:
**
**      sc      -  Pointer to the softc structure.
**      cmd     -  command to this machine 
**
**  IMPLICIT INPUTS:
**
**      sc->t0RecvS             state of this machine 
**      sc->pIoctlT0->command   command to send to card
**
**  IMPLICIT OUTPUTS:
**
**      sc->pIoctlT0->data      data from card 
**      sc->pIoctlT0->dataLen   size of data from card 
**      sc->pIoctlT0->sw1       command status from card 
**      sc->pIoctlT0->sw2       command status from card 
**      sc->status              error status from this machine
**
**  FUNCTION VALUE:
**
**      nill
**
**  SIDE EFFECTS:
**
**      nill
**--
*/
static void   t0RecvSM (struct scr_softc * sc,int cmd)
{
    if (sc->bigTrouble) return;     // david,jim , remove this when dust settles 

    /*
    ** check for major failures that are common to most states
    */
    if (cmd == t0rcTWorkWaiting ||
        cmd == gcT0RecvByteErr  || 
        cmd == gcT0SendByteErr  )
        
    {
        switch(cmd)
        {
    
            case t0rcTWorkWaiting:
                ASSERT(sc->t0RecvS != t0rsIdle); 
                
                /* kill all lower level machines */
                t0SendByteSM(sc,t0sbcAbort);
                t0RecvByteSM(sc,t0rbcAbort);
                
                /* set status */
                sc->status = ERROR_WORK_WAITING;
                break;
            
            case gcT0RecvByteErr:       // fall through
            case gcT0SendByteErr:
                /* kill all the timers */
                scrUntimeout(t0RecvSM, sc,t0rcTWorkWaiting);
                break;
            
            default:
                INVALID_STATE_CMD(sc, sc->t0RecvS,cmd);
                break;
            
        }



        /* change state */
        sc->t0RecvS = t0rsIdle;   
        masterSM(sc,mcT0Recv);

        /* all done */
        return;
    }

    switch (sc->t0RecvS)
    {
        case t0rsIdle:
            switch (cmd)
            {
                case t0rcStart:
                    /* set initial values */
                    sc->t0RecvS = t0rsSendHeader;
                    sc->t0ByteParent = t0RecvSM;
                    sc->commandCount = 0;   
                    sc->dataCount = 0;
                    sc->dataMax = sc->pIoctlT0->command[CMD_BUF_DATA_LEN_OFF];
                    if (sc->dataMax == 0)
                    {
                        sc->dataMax = 256;
                    }
                    sc->dataByte = sc->pIoctlT0->command[sc->commandCount];
                    t0SendByteSM(sc,t0sbcStart);
                    break;

                default:
                    INVALID_STATE_CMD(sc, sc->t0RecvS,cmd);
                    break;
            }
            break;

        case t0rsSendHeader:
            switch (cmd)
            {
                case gcT0SendByte:
                    sc->commandCount++;
                    if (sc->commandCount < CMD_BUF_LEN)
                    {
                        sc->dataByte = sc->pIoctlT0->command[sc->commandCount];
                        t0SendByteSM(sc,t0sbcStart);
                    }
                    else
                    {
                        ASSERT(sc->commandCount == CMD_BUF_LEN);

                        sc->t0RecvS = t0rsRecvProcedure;
                        scrTimeout(t0RecvSM,sc,t0rcTWorkWaiting,T_WORK_WAITING);
                        t0RecvByteSM(sc,t0rbcStart);
                    }
                    break;

                default:
                    INVALID_STATE_CMD(sc, sc->t0RecvS,cmd);
                    break;
            }
            break;

        case t0rsRecvProcedure:
            switch (cmd)
            {
                case gcT0RecvByte:
                    scrUntimeout(t0RecvSM, sc,t0rcTWorkWaiting);

                    /* see if we should recv all remaining bytes */
                    if ( (sc->dataByte == sc->pIoctlT0->command[CMD_BUF_INS_OFF])          ||
                         (sc->dataByte == (sc->pIoctlT0->command[CMD_BUF_INS_OFF] ^ 0x01))   )
                    {
                        ASSERT(sc->dataCount < sc->dataMax);

                        sc->t0RecvS = t0rsRecvData;
                        scrTimeout(t0RecvSM,sc,t0rcTWorkWaiting,T_WORK_WAITING);
                        t0RecvByteSM(sc,t0rbcStart);
                    }

                    /* see if we should send one data byte */
                    else if ((sc->dataByte == (sc->pIoctlT0->command[CMD_BUF_INS_OFF]  ^ 0xFF)) ||
                             (sc->dataByte == (sc->pIoctlT0->command[CMD_BUF_INS_OFF]  ^ 0xFE)) )
                    {
                        ASSERT(sc->dataCount < sc->dataMax);
                        sc->t0RecvS = t0rsRecvByte;
                        scrTimeout(t0RecvSM,sc,t0rcTWorkWaiting,T_WORK_WAITING);
                        t0RecvByteSM(sc,t0rbcStart);
                    }

                    /* see if we should extend the work waiting period */
                    else if (sc->dataByte == 0x60)
                    {
                        t0RecvByteSM(sc,t0rbcStart);
                        scrTimeout(t0RecvSM,sc,t0rcTWorkWaiting,T_WORK_WAITING);
                    }

#ifdef ORIGINAL_SW1_CODE /* XXX XXX XXX cgd */
                    /* see if we have a SW1 byte */
                    else if ( ((sc->dataByte & 0xf0)  == 0x60) || ((sc->dataByte & 0xf0)  == 0x90)  
                              &&
                              sc->dataByte != 0x60)
#else /* XXX XXX XXX cgd */
                    /* see if we have a SW1 byte */
                    else if ( ( ((sc->dataByte & 0xf0)  == 0x60) || ((sc->dataByte & 0xf0)  == 0x90) ) 
                              &&
                              sc->dataByte != 0x60)
#endif /* XXX XXX XXX cgd */
                    {
                        sc->pIoctlT0->sw1 = sc->dataByte;
                        sc->t0RecvS = t0rsRecvSW2;
                        t0RecvByteSM(sc,t0rbcStart);
                        scrTimeout(t0RecvSM,sc,t0rcTWorkWaiting,T_WORK_WAITING);
                    }

                    /* got bad data byte, log error and get out */
                    else
                    {
                        sc->status = ERROR_BAD_PROCEDURE_BYTE;

                        /* change state */
                        sc->t0RecvS = t0rsIdle;
                        masterSM(sc,mcT0Recv);
                    }
                    break;

                default:
                    INVALID_STATE_CMD(sc, sc->t0RecvS,cmd);
                    break;
            }
            break;

        case t0rsRecvByte:
            switch (cmd)
            {
                case gcT0RecvByte:
                    /* clock in byte */
                    scrUntimeout(t0RecvSM, sc,t0rcTWorkWaiting);
                    sc->pIoctlT0->data[sc->dataCount] = sc->dataByte;
                    sc->dataCount++;    


                    if (sc->dataCount < sc->dataMax)
                    {
                        /* get procedure byte */
                        sc->t0RecvS = t0rsRecvProcedure;
                    }

                    else
                    {
                        ASSERT(sc->dataCount == sc->dataMax);
                        sc->t0RecvS = t0rsRecvSW1;
                    }

                    // ask for another byte 
                    scrTimeout(t0RecvSM,sc,t0rcTWorkWaiting,T_WORK_WAITING);
                    t0RecvByteSM(sc,t0rbcStart);
                    break;

                default:
                    INVALID_STATE_CMD(sc, sc->t0RecvS,cmd);
                    break;
            }
            break;

        case t0rsRecvData:
            switch (cmd)
            {
                case gcT0RecvByte:
                    /* clock in data */
                    scrUntimeout(t0RecvSM, sc,t0rcTWorkWaiting);
                    sc->pIoctlT0->data[sc->dataCount] = sc->dataByte;
                    sc->dataCount++;    

                    /* decide if we have all data */
                    if (sc->dataCount >= sc->dataMax)
                    {
                        KERN_DEBUG (scrdebug, T0_RECV_SM_DEBUG_INFO,("\t\tt0RecvSM: changing state to t0rsRecvSW1\n",sc->dataByte));
                        ASSERT(sc->dataCount == sc->dataMax);
                        sc->t0RecvS = t0rsRecvSW1;
                    }

                    /* ask for another byte */
                    scrTimeout(t0RecvSM,sc,t0rcTWorkWaiting,T_WORK_WAITING);
                    t0RecvByteSM(sc,t0rbcStart);
                    break;

                default:
                    INVALID_STATE_CMD(sc, sc->t0RecvS,cmd);
                    break;
            }
            break;

        case t0rsRecvSW1:
            switch (cmd)
            {
                case gcT0RecvByte:
                    scrUntimeout(t0RecvSM, sc,t0rcTWorkWaiting);
                    sc->pIoctlT0->sw1 = sc->dataByte;

                    sc->t0RecvS = t0rsRecvSW2;
                    t0RecvByteSM(sc,t0rbcStart);
                    scrTimeout(t0RecvSM,sc,t0rcTWorkWaiting,T_WORK_WAITING);
                    break;
                default:
                    INVALID_STATE_CMD(sc, sc->t0RecvS,cmd);
                    break;
            }
            break;

        case t0rsRecvSW2:
            switch (cmd)
            {
                case gcT0RecvByte:
                    scrUntimeout(t0RecvSM, sc,t0rcTWorkWaiting);
                    sc->pIoctlT0->sw2 = sc->dataByte;

                    sc->t0RecvS = t0rsIdle; 
                    masterSM(sc,mcT0Recv);
                    break;

                default:
                    INVALID_STATE_CMD(sc, sc->t0RecvS,cmd);
                    break;
            }
            break;

        default:
            INVALID_STATE_CMD(sc, sc->t0RecvS,cmd);
            break;
    }
}


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      coldResetSM
**
**      This state machine switches on the power, clock and reset pins 
**      in the correct order/timing.
**      It is a low level bit-bashing state machine.
**
**
**  FORMAL PARAMETERS:
**
**      sc      -  Pointer to the softc structure.
**      cmd     -  command to this machine 
**
**  IMPLICIT INPUTS:
**
**      sc->coldResetS     state of this machine 
**
**  IMPLICIT OUTPUTS:
**
**      nill
**
**  FUNCTION VALUE:
**
**      nill
**
**  SIDE EFFECTS:
**
**      signals to card are on 
**--
*/
static void   coldResetSM(struct scr_softc * sc,int cmd)
{
    if (sc->bigTrouble) return;     // david,jim , remove this when dust settles 

    switch (sc->coldResetS)
    {
        case    crsIdle:
            switch (cmd)
            {
                case crcStart:
                    scrSetReset(TRUE);
                    scrSetClock(TRUE);
                    scrSetDataHighZ();
                    scrSetPower(TRUE);

                    /* start a t2 timer */
                    scrTimeout(coldResetSM,sc,crcT2,T_t2);
                    sc->coldResetS = crsT2Wait;
                    break;

                default:
                    INVALID_STATE_CMD(sc,sc->masterS,cmd);
                    break;
            }
            break;

        case    crsT2Wait:      
            switch (cmd)
            {
                case crcT2:
                    /* turn off rst */
                    scrSetReset(FALSE);

                    /* tell master state machine that we are all done */
                    sc->coldResetS = crsIdle;
                    masterSM(sc,mcColdReset);       
                    break;

                default:
                    INVALID_STATE_CMD(sc,sc->masterS,cmd);
                    break;
            }
            break;


        default:
            INVALID_STATE_CMD(sc,sc->coldResetS,cmd);
            break;
    }
}

/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      ATRSM
**
**      This is the Answer To Reset State Machine.  It is responsible 
**      for performing the Answer To Reset as specified in ISO 7816-3.
**      It is mid level protocol state machine.
**
**      Once started, this machine is driven entirely via the 
**      FIQ/timeout structure.
**
**
**      During the first byte, we have to check if the card is operating
**      at full speed or half speed.  The first couple of bits are 
**      checked to see if it is 1/2 speed, and if so, the clock is changed
**      and the state adjustes
**
**      At the end of the first byte we have to determin the logic being
**      used by the card, ie is it active high/low and msb/lsb.  
**
**
**  FORMAL PARAMETERS:
**
**      sc      -  Pointer to the softc structure.
**      cmd     -  command to this machine 
**
**  IMPLICIT INPUTS:
**
**      sc->pIoctlAtr->atr      data from card 
**      sc->pIoctlT0->sw1       command status from card 
**      sc->pIoctlT0->sw2       command status from card 
**      sc->status              error status from this machine
**
**  IMPLICIT OUTPUTS:
**
**      sc->pIoctlOn->atrBuf    data from ATR sequence
**      sc->pIoctlOn->atrLen    size of data from ATR sequence
**      sc->status              error status from this machine
**      
**
**  FUNCTION VALUE:
**
**      nill
**
**  SIDE EFFECTS:
**
**      nill
**--
*/
static void ATRSM (struct scr_softc * sc,int cmd)
{
    int lc;
    int tck;

    if (sc->bigTrouble) return;     // david,jim , remove this when dust settles 

    /* 
    ** check for major failures that are common to most states
    */
    if (cmd == atrcT3            ||
        cmd == atrcTWorkWaiting  ||
        cmd == gcT0RecvByteErr  
        )
    {
        switch(cmd)
        {
            case atrcT3:
                scrUntimeout (ATRSM,sc,atrcTWorkWaiting);
                sc->status = ERROR_ATR_T3;
                t0RecvByteSM(sc,t0rbcAbort);
                break;
            
            case atrcTWorkWaiting:
                scrUntimeout (ATRSM,sc,atrcT3);
                sc->status = ERROR_WORK_WAITING;
                t0RecvByteSM(sc,t0rbcAbort);
                break;
            
            case gcT0RecvByteErr:
                scrUntimeout (ATRSM,sc,atrcT3);
                scrUntimeout (ATRSM,sc,atrcTWorkWaiting);
                /* done set status, its already set */
                break;
            
            default:
                INVALID_STATE_CMD(sc,sc->ATRS,cmd);
                break;
        }
      
        /* change state */
        sc->ATRS = atrsIdle;
        masterSM(sc,mcATR);
        return;
    }

    switch (sc->ATRS)
    {
        case    atrsIdle:
            switch (cmd)
            {
                case atrcStart:
                    /* lets start looking */
                    sc->ATRS = atrsTS;
                    sc->pIoctlOn->atrLen = 0;
                    sc->t0ByteParent = ATRSM;
                    scrTimeout(ATRSM,sc,atrcT3,T_t3 *2);  /* by 2 to accomodate 1/2 freq cards */
                    t0RecvByteSM(sc,t0rbcStart);
                    break;

                default:
                    INVALID_STATE_CMD(sc,sc->ATRS,cmd);
                    break;
            }
            break;

        case atrsTS:
            switch (cmd)
            {
                case gcT0RecvByte:
                    scrUntimeout(ATRSM,sc,atrcT3);
                    sc->pIoctlOn->atrBuf[sc->pIoctlOn->atrLen] = sc->dataByte;  
                    sc->pIoctlOn->atrLen++;
                    if(sc->pIoctlOn->atrLen >= ATR_BUF_MAX)
                    {
                        #ifdef SCR_DEBUG
                            DEBUGGER;
                        #endif
                        sc->status = ERROR_ATR_TCK;
                        sc->ATRS = atrsIdle;
                        masterSM(sc,mcATR);
                    }
                    else
                    {
                        /* move onto recv T0 */
                        sc->ATRS = atrsT0;
                        scrTimeout(ATRSM,sc,atrcTWorkWaiting,T_WORK_WAITING);
                        t0RecvByteSM(sc,t0rbcStart);
                    }
                    break;

                default:
                    INVALID_STATE_CMD(sc,sc->ATRS,cmd);
                    break;
            }
            break;

        case atrsT0:
            switch (cmd)
            {
                case gcT0RecvByte:
                    scrUntimeout(ATRSM,sc,atrcTWorkWaiting);
                    sc->pIoctlOn->atrBuf[sc->pIoctlOn->atrLen] = sc->dataByte;
                    sc->pIoctlOn->atrLen++;
                    if(sc->pIoctlOn->atrLen >= ATR_BUF_MAX)
                    {
                        #ifdef SCR_DEBUG
                            printf("atrLen >= ATR_BUF_MAX\n");
                            DEBUGGER;
                        #endif
                        sc->status = ERROR_ATR_TCK;
                        sc->ATRS = atrsIdle;
                        masterSM(sc,mcATR);
                    }
                    else
                    {
                        /* store Y & K */
                        sc->atrY = sc->dataByte & 0xf0;
                        sc->atrK = sc->dataByte & 0x0f;

                        sc->atrTABCDx = 1;
                        sc->atrKCount = 1;
    
                        /* if there are no TDx following set T0 protocol */
                        if (ISCLR(sc->atrY,ATR_Y_TD))
                        {
                            sc->protocolType    = PROTOCOL_T0;      
                        }
    
    
                        if (sc->atrY)
                        {
    
                            sc->ATRS = atrsTABCD;
                            scrTimeout(ATRSM,sc,atrcTWorkWaiting,T_WORK_WAITING);
                            t0RecvByteSM(sc,t0rbcStart);
                        }
    
                        else if (sc->atrK)
                        {
                            sc->ATRS = atrsTK;
                            scrTimeout(ATRSM,sc,atrcTWorkWaiting,T_WORK_WAITING);
                            t0RecvByteSM(sc,t0rbcStart);
                        }
    
                        else if (sc->protocolType != PROTOCOL_T0)
                        {
                            sc->ATRS = atrsTCK;
                            scrTimeout(ATRSM,sc,atrcTWorkWaiting,T_WORK_WAITING);
                            t0RecvByteSM(sc,t0rbcStart);
                        }
    
                        else /* got all of ATR */
                        {
                            sc->ATRS = atrsIdle;
                            masterSM(sc,mcATR);
                        }
                    }
                    break;


                default:
                    INVALID_STATE_CMD(sc,sc->ATRS,cmd);
                    break;
            }
            break;


        case atrsTABCD:
            switch (cmd)
            {
                case gcT0RecvByte:
                    scrUntimeout(ATRSM,sc,atrcTWorkWaiting);
                    sc->pIoctlOn->atrBuf[sc->pIoctlOn->atrLen] = sc->dataByte;
                    sc->pIoctlOn->atrLen++;
                    if(sc->pIoctlOn->atrLen >= ATR_BUF_MAX)
                    {
                        #ifdef SCR_DEBUG
                            printf("atrLen >= ATR_BUF_MAX\n");
                            DEBUGGER;
                        #endif
                        sc->status = ERROR_ATR_TCK;
                        sc->ATRS = atrsIdle;
                        masterSM(sc,mcATR);
                    }
                    else
                    {
                        if (sc->atrY & ATR_Y_TA)
                        {
                            sc->atrY &= ~ATR_Y_TA;
                            if (sc->atrTABCDx == 1)
                            {
                                sc->Fi = FI2Fi[((sc->dataByte >> 4) & 0x0f)];
                                if (sc->Fi == 0)
                                {
                                    sc->status = ERROR_ATR_FI_INVALID;
                                    sc->Fi = Fi_DEFAULT;
                                }
    
                                sc->Di = DI2Di[(sc->dataByte & 0x0f)];
                                if (sc->Di == 0)
                                {
                                    sc->status = ERROR_ATR_DI_INVALID;
                                    sc->Di = Di_DEFAULT;
                                }
    
                            }
                        }
    
                        else if (sc->atrY & ATR_Y_TB)
                        {
                            sc->atrY &= ~ATR_Y_TB;
                        }
    
                        else if (sc->atrY & ATR_Y_TC)
                        {
                            sc->atrY &= ~ATR_Y_TC;
                            if (sc->atrTABCDx == 1)
                            {
                                sc->N = sc->dataByte;
                            }
    
                            if (sc->atrTABCDx == 2)
                            {
                                sc->Wi = sc->dataByte;
                            }
                        }
    
                        else
                        {
                            ASSERT(sc->atrY & ATR_Y_TD);
                            sc->atrY &= ~ATR_Y_TD;

                            /* copy across the y section of TD */
                            sc->atrY    =    sc->dataByte;
                            sc->atrY &= 0xf0;

                            /* step to the next group of TABCD */
                            sc->atrTABCDx++;

                            /* store protocols */
                            sc->protocolType = (1 << (sc->dataByte &0x0f));
                        }
    
    
                        /* see what we should do next */
                        if (sc->atrY)
                        {
                            /* just stay in the same state */
                            scrTimeout(ATRSM,sc,atrcTWorkWaiting,T_WORK_WAITING);
                            t0RecvByteSM(sc,t0rbcStart);
                        }
    
                        else if (sc->atrK)
                        {
                            sc->ATRS = atrsTK;
                            scrTimeout(ATRSM,sc,atrcTWorkWaiting,T_WORK_WAITING);
                            t0RecvByteSM(sc,t0rbcStart);
                        }
    
                        else if (sc->protocolType != PROTOCOL_T0)
                        {
                            sc->ATRS = atrsTCK;
                            scrTimeout(ATRSM,sc,atrcTWorkWaiting,T_WORK_WAITING);
                            t0RecvByteSM(sc,t0rbcStart);
                        }
    
                        else /* got all of ATR */
                        {
                            sc->ATRS = atrsIdle;
                            masterSM(sc,mcATR);
                        }
                    }

                    break;


                default:
                    INVALID_STATE_CMD(sc,sc->ATRS,cmd);
                    break;
            }
            break;

        case atrsTK:
            switch (cmd)
            {
                case gcT0RecvByte:
                    scrUntimeout(ATRSM,sc,atrcTWorkWaiting);
                    sc->pIoctlOn->atrBuf[sc->pIoctlOn->atrLen] = sc->dataByte;
                    sc->pIoctlOn->atrLen++;
                    if(sc->pIoctlOn->atrLen >= ATR_BUF_MAX)
                    {
                        #ifdef SCR_DEBUG
                            printf("atrLen >= ATR_BUF_MAX\n");
                            DEBUGGER;
                        #endif
                        sc->status = ERROR_ATR_TCK;
                        sc->ATRS = atrsIdle;
                        masterSM(sc,mcATR);
                    }
                    else
                    {
    
                        if (sc->atrKCount < sc->atrK)
                        {
                            sc->atrKCount++;
                            scrTimeout(ATRSM,sc,atrcTWorkWaiting,T_WORK_WAITING);
                            t0RecvByteSM(sc,t0rbcStart);
                        }
    
    
                        else if (sc->protocolType != PROTOCOL_T0)
                        {
                            sc->ATRS = atrsTCK;
                            scrTimeout(ATRSM,sc,atrcTWorkWaiting,T_WORK_WAITING);
                            t0RecvByteSM(sc,t0rbcStart);
                        }
    
                        else /* got all of ATR */
                        {
                            sc->ATRS = atrsIdle;
                            masterSM(sc,mcATR);
                        }
                    }
                    break;

                default:
                    INVALID_STATE_CMD(sc,sc->ATRS,cmd);
                    break;
            }
            break;

        case atrsTCK:
            switch (cmd)
            {
                case gcT0RecvByte:
                    scrUntimeout(ATRSM,sc,atrcTWorkWaiting);
                    sc->pIoctlOn->atrBuf[sc->pIoctlOn->atrLen] = sc->dataByte;
                    sc->pIoctlOn->atrLen++;
                    if(sc->pIoctlOn->atrLen >= ATR_BUF_MAX)
                    {
                        #ifdef SCR_DEBUG
                            printf("atrLen >= ATR_BUF_MAX\n");
                            DEBUGGER;
                        #endif
                        sc->status = ERROR_ATR_TCK;
                        sc->ATRS = atrsIdle;
                        masterSM(sc,mcATR);
                    }
                    else
                    {
                        tck = 0;
                        for (lc = 1; lc < sc->pIoctlOn->atrLen-1; lc++)
                        {
                            tck ^= sc->pIoctlOn->atrBuf[lc];
                        }
    
                        if (tck == sc->pIoctlOn->atrBuf[sc->pIoctlOn->atrLen-1])
                        {
                            sc->ATRS = atrsIdle;
                            masterSM(sc,mcATR);
                        }
                        else
                        {
                            sc->status = ERROR_ATR_TCK;
                            sc->ATRS = atrsIdle;
                            masterSM(sc,mcATR);
                        }
                    }
                    break;

                default:
                    INVALID_STATE_CMD(sc,sc->ATRS,cmd);
                    break;
            }
            break;



        default:
            INVALID_STATE_CMD(sc,sc->ATRS,cmd);
            break;
    }
}



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      t0RecvByteSM
**
**      This state machine attempts to read 1 byte from a card.
**      It is a low level bit-bashing state machine.
**
**      Data from the card is async, so the machine scans at 
**      5 times the data rate looking for a state bit.  Once 
**      a start bit has been found, it waits for the middle of 
**      the bit and starts sampling at the bit rate.
**
**      Several mid level machines can use this machine, so the value
**      sc->t0ByteParent is used to point to back to the mid level machine
**      
**
**  FORMAL PARAMETERS:
**
**      sc      -  Pointer to the softc structure.
**      cmd     -  command to this machine 
**
**  IMPLICIT INPUTS:
**
**      sc->t0RecvByteS     state of this machine 
**      sc->t0ByteParent    mid level machine that started this machine
**
**  IMPLICIT OUTPUTS:
**
**      sc->shiftByte       byte read from the card 
**      sc->status          error value if could not read byte
**
**  FUNCTION VALUE:
**
**      nill
**
**  SIDE EFFECTS:
**
**      nill
**--
*/
static void   t0RecvByteSM(struct scr_softc* sc,int cmd)
{
    if (sc->bigTrouble) return;     // david,jim , remove this when dust settles 

    if (cmd == t0rbcAbort)
    {
        /* kill all the timers */
        scrUntimeout(t0RecvByteSM, sc,t0rbcTFindStartEdge);
        scrUntimeout(t0RecvByteSM, sc,t0rbcTFindStartMid);
        scrUntimeout(t0RecvByteSM, sc,t0rbcTClockData);
        scrUntimeout(t0RecvByteSM, sc,t0rbcTErrorStart);
        scrUntimeout(t0RecvByteSM, sc,t0rbcTErrorStop);

        scrSetDataHighZ();
        sc->t0RecvByteS = t0rbsIdle; 
        return;
    }


    switch (sc->t0RecvByteS)
    {
        case t0rbsIdle:
            switch (cmd)
            {
                case t0rbcStart:
                    /* set initial conditions */
                    sc->shiftBits   = 0;
                    sc->shiftByte   = 0;
                    sc->shiftParity = 0;
                    sc->shiftParityCount = 0; 
                    scrClkAdj(sc->clkCountStartRecv); /* recv data clock running at 5 times */

                    /* check if start bit is already here */
                    //if (scrGetData())
                    if (1)
                    {
                        /* didn't find it, keep looking */
                        scrTimeout(t0RecvByteSM,sc,t0rbcTFindStartEdge,sc->clkCountStartRecv);
                        sc->t0RecvByteS = t0rbsFindStartEdge;
                    }
                    else
                    {
                        /* found start bit, look for mid bit */
                        scrTimeout(t0RecvByteSM,sc,t0rbcTFindStartMid,sc->clkCountStartRecv);
                        sc->t0RecvByteS = t0rbsFindStartMid;
                    }
                    break;



                default:
                    INVALID_STATE_CMD(sc, sc->t0RecvByteS,cmd);
                    break;
            }
            break;


        case    t0rbsFindStartEdge:
            switch (cmd)
            {
                case t0rbcTFindStartEdge:
                    if (scrGetData())
                    {
                        /* didn't find it, keep looking */
                        scrTimeout(t0RecvByteSM,sc,t0rbcTFindStartEdge,sc->clkCountStartRecv);
                    }
                    else
                    {
                        /* found start bit, look for mid bit */
                        scrTimeout(t0RecvByteSM,sc,t0rbcTFindStartMid,sc->clkCountStartRecv * 2); 
                        sc->t0RecvByteS = t0rbsFindStartMid;
                    }
                    break;


                default:
                    INVALID_STATE_CMD(sc, sc->t0RecvByteS,cmd);
                    break;
            }
            break;

        case    t0rbsFindStartMid:
            switch (cmd)
            {
                case t0rbcTFindStartMid:
                    if (scrGetData())
                    {
                        /* found glitch, so just go back to hunting */
                        scrTimeout(t0RecvByteSM,sc,t0rbcTFindStartEdge,sc->clkCountStartRecv);
                        sc->t0RecvByteS = t0rbsFindStartEdge;
                    }
                    else
                    {
                        /* found start bit, start clocking in data */
                        TOGGLE_TEST_PIN();
                        scrTimeout(t0RecvByteSM,sc,t0rbcTClockData,sc->clkCountDataRecv);
                        sc->t0RecvByteS = t0rbsClockData;
                    }
                    break;


                default:
                    INVALID_STATE_CMD(sc, sc->t0RecvByteS,cmd);
                    break;
            }
            break;


        case    t0rbsClockData:     
            TOGGLE_TEST_PIN();
            switch (cmd)
            {
                case t0rbcTClockData:
                    if (sc->shiftBits < 8)
                    {
                        if (sc->convention == CONVENTION_INVERSE || 
                            sc->convention == CONVENTION_UNKOWN)
                        {
                            /* logic 1 is low, msb is first */
                            sc->shiftByte <<= 1;
                            sc->shiftByte &=  0xfe;
                            if (!scrGetData())
                            {
                                sc->shiftByte |= 0x01;
                                sc->shiftParity++;
                            }
                        }
                        else
                        {
                            ASSERT(sc->convention == CONVENTION_DIRECT);
                            /* logic 1 is high, lsb is first */
                            sc->shiftByte = sc->shiftByte >> 1;
                            sc->shiftByte &=  0x7f;
                            if (scrGetData())
                            {
                                sc->shiftParity++;
                                sc->shiftByte |= 0x80;
                            }
                        }
                        sc->shiftBits++;


                        /* in TS byte, check if we have a card that works at 1/2 freq */
                        if (sc->convention == CONVENTION_UNKOWN  &&   /* in TS byte */
                            sc->shiftBits == 3 &&                     /* test at bit 3 in word */
                            sc->shiftByte == 4 &&                     /* check for 1/2 freq pattern */
                            sc->cardFreq  == CARD_FREQ_DEF)           /* only do this if at full freq */
                        {
                            /* adjust counts down to 1/2 freq */
                            sc->cardFreq        = CARD_FREQ_DEF / 2;
                            sc->clkCountStartRecv   = sc->clkCountStartRecv *2; 
                            sc->clkCountDataRecv    = sc->clkCountDataRecv  *2;  
                            sc->clkCountDataSend    = sc->clkCountDataSend  *2;  


                            /* adjust this so that we have clocked in only fist bit of TS */
                            sc->shiftParity = 0;
                            sc->shiftByte   = 0;
                            sc->shiftBits   = 1;

                            scrTimeout(t0RecvByteSM,sc,t0rbcTClockData,(sc->clkCountDataRecv * 3) /4);
                        }
                        else
                        {
                            scrTimeout(t0RecvByteSM,sc,t0rbcTClockData,sc->clkCountDataRecv);
                        }
                    }

                    /* clock in parity bit  */
                    else if (sc->shiftBits == 8)
                    {
                        if (sc->convention == CONVENTION_INVERSE)
                        {
                            if (!scrGetData())
                            {
                                sc->shiftParity++;
                            }
                        }
                        else if (sc->convention == CONVENTION_DIRECT)
                        {
                            if (scrGetData())
                            {
                                sc->shiftParity++;
                            }
                        }


                        else
                        {
                            /* sc->convention not set so sort it out */
                            ASSERT(sc->convention == CONVENTION_UNKOWN);
                            if (sc->shiftByte == CONVENIONT_INVERSE_ID && scrGetData())
                            {
                                sc->convention = CONVENTION_INVERSE;
                                sc->shiftParity = 0;    /* force good parity */
                            }

                            else if (sc->shiftByte == CONVENTION_DIRECT_ID && scrGetData())
                            {
                                sc->shiftByte = CONVENTION_DIRECT_FIX;
                                sc->convention = CONVENTION_DIRECT;
                                sc->shiftParity = 0;    /* force good parity */
                            }

                            else
                            {
                                sc->shiftParity = 1; /* force bad parity */
                            }
                        }


                        if ((sc->shiftParity & 01) == 0)
                        {
                            sc->shiftBits++;
                            scrTimeout(t0RecvByteSM,sc,t0rbcTClockData,sc->clkCountDataRecv);
                        }
                        else
                        {
                            /* got parity error */
                            if (sc->shiftParityCount < PARITY_ERROR_MAX)
                            {
                                sc->shiftParityCount++;
                                scrTimeout(t0RecvByteSM,sc,t0rbcTErrorStart,sc->clkCountDataRecv);
                                sc->t0RecvByteS = t0rbsSendError;
                            }
                            else

                            {
                                /* too many parity errors, just give up on this sc->dataByte */
                                sc->status = ERROR_PARITY;
                                sc->t0RecvByteS = t0rbsIdle;
                                sc->t0ByteParent(sc,gcT0RecvByteErr);
                            }
                        }
                    }

                    else
                    {
                        sc->dataByte = sc->shiftByte;
                        sc->t0RecvByteS = t0rbsIdle;
                        sc->t0ByteParent(sc,gcT0RecvByte);
                    }
                    break;

                default:
                    INVALID_STATE_CMD(sc, sc->t0RecvByteS,cmd);
                    break;
            }
            break;


        case    t0rbsSendError:     
            TOGGLE_TEST_PIN();
            switch (cmd)
            {
                case t0rbcTErrorStart:
                    /* start sending error bit */
                    scrSetData(FALSE);
                    scrTimeout(t0RecvByteSM,sc,t0rbcTErrorStop,sc->clkCountDataRecv * 2);
                    break;

                case t0rbcTErrorStop:
                    /* stop sending parity error & reset information*/
                    scrSetData(TRUE);
                    sc->shiftBits   = 0;
                    sc->shiftByte   = 0;
                    sc->shiftParity = 0;

                    /* start looking for start bit */
                    scrTimeout(t0RecvByteSM,sc,t0rbcTFindStartEdge,1);
                    sc->t0RecvByteS = t0rbsFindStartEdge;
                    break;

                default:
                    INVALID_STATE_CMD(sc, sc->t0RecvByteS,cmd);
                    break;
            }
            break;


        default:
            INVALID_STATE_CMD(sc, sc->t0RecvByteS,cmd);
            break;
    }
}

/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      t0SendByteSM
**
**      This state machine writes 1 byte to a card.
**      It is a low level bit-bashing state machine.
**
**
**      Several mid level machines can use this machine, so the value
**      sc->t0ByteParent is used to point to back to the mid level machine
**
**  FORMAL PARAMETERS:
**
**      sc      -  Pointer to the softc structure.
**      cmd     -  command to this machine 
**
**  IMPLICIT INPUTS:
**
**      sc->t0SendByteS     state of this machine 
**      sc->shiftByte       byte to write to the card 
**
**  IMPLICIT OUTPUTS:
**
**      sc->status          error value if could not read byte
**
**  FUNCTION VALUE:
**
**      nill
**
**  SIDE EFFECTS:
**
**      nill
**--
*/
//int bigTroubleTest = 0;
static void t0SendByteSM (struct scr_softc * sc,int cmd)
{
    //if(bigTroubleTest == 2000)
    //{
    //    INVALID_STATE_CMD(sc, sc->t0SendByteS,cmd);
    //    bigTroubleTest = 0;
    //}
    //
    //bigTroubleTest++;
    
    if (sc->bigTrouble) return;     // david,jim , remove this when dust settles 

    if (cmd == t0sbcAbort)
    {
        /* kill all the timers */
        scrUntimeout(t0SendByteSM, sc, t0sbcTGuardTime);
        scrUntimeout(t0SendByteSM, sc, t0sbcTClockData);
        scrUntimeout(t0SendByteSM, sc, t0sbcTError);

        scrSetDataHighZ();
        return;
    }


    switch (sc->t0SendByteS)
    {
        case t0sbsIdle:
            switch (cmd)
            {
                case t0sbcStart:
                    /* set initial conditions */
                    sc->shiftBits   = 0;
                    sc->shiftParity = 0;
                    sc->shiftParityCount = 0;
                    sc->shiftByte = sc->dataByte;

                    scrClkAdj(sc->clkCountDataSend); /* send data clock running at 1 ETU  */

                    /* check if we have to wait for guard time */
                    if (0) /* possible optimization here */
                    {
                        /* can send start bit now */
                        scrTimeout(t0SendByteSM,sc,t0sbcTClockData,sc->clkCountDataSend);
                        scrSetData(FALSE);
                        sc->t0SendByteS = t0sbsClockData;
                    }
                    else
                    {
                        /* need to wait for guard time */
                        scrTimeout(t0SendByteSM,sc,t0sbcTGuardTime,sc->clkCountDataSend * (12 + sc->N));
                        sc->t0SendByteS = t0sbsWaitGuardTime;

                    }
                    break;

                default:
                    INVALID_STATE_CMD(sc, sc->t0SendByteS,cmd);
                    break;
            }
            break;


        case t0sbsWaitGuardTime:
            switch (cmd)
            {
                case t0sbcTGuardTime: 
                    TOGGLE_TEST_PIN();
                    /*  set start bit */
                    scrTimeout(t0SendByteSM,sc,t0sbcTClockData,sc->clkCountDataSend);
                    scrSetData(FALSE);
                    sc->t0SendByteS = t0sbsClockData;
                    break;

                default:
                    INVALID_STATE_CMD(sc, sc->t0SendByteS,cmd);
                    break;
            }
            break;


        case t0sbsClockData:
            switch (cmd)
            {
                case t0sbcTClockData: 
                    TOGGLE_TEST_PIN();
                    /* clock out data bit */
                    if (sc->shiftBits < 8)
                    {
                        if (sc->convention == CONVENTION_INVERSE)
                        {
                            if (sc->shiftByte & 0x80)
                            {
                                scrSetData(FALSE);
                                sc->shiftParity++;
                            }
                            else
                            {
                                scrSetData(TRUE);
                            }
                            sc->shiftByte = sc->shiftByte << 1;
                        }
                        else
                        {
                            ASSERT(sc->convention == CONVENTION_DIRECT);
                            if (sc->shiftByte & 0x01)
                            {
                                scrSetData(TRUE);
                                sc->shiftParity++;
                            }
                            else
                            {
                                scrSetData(FALSE);
                            }
                            sc->shiftByte = sc->shiftByte >> 1;
                        }
                        sc->shiftBits++;
                        scrTimeout(t0SendByteSM,sc,t0sbcTClockData,sc->clkCountDataSend);
                    }

                    /* clock out parity bit */
                    else if (sc->shiftBits == 8)
                    {
                        if ( ((sc->shiftParity & 0x01) &&  (sc->convention == CONVENTION_INVERSE))  ||
                             (!(sc->shiftParity & 0x01) && (sc->convention == CONVENTION_DIRECT))     )
                        {
                            scrSetData(FALSE);
                        }
                        else
                        {
                            scrSetData(TRUE);
                        }
                        sc->shiftBits++;
                        scrTimeout(t0SendByteSM,sc,t0sbcTClockData,sc->clkCountDataSend);
                    }

                    /* all data shifted out, move onto next state */
                    else
                    {
                        ASSERT(sc->shiftBits > 8);
                        scrSetData(TRUE);
                        scrTimeout(t0SendByteSM,sc,t0sbcTError,sc->clkCountDataSend);
                        sc->t0SendByteS = t0sbsWaitError;
                    }
                    break;

                default:
                    INVALID_STATE_CMD(sc, sc->t0SendByteS,cmd);
                    break;
            }
            break;

        case t0sbsWaitError:
            switch (cmd)
            {
                case t0sbcTError: 
                    /* no error indicated*/
                    if (scrGetData())
                    {
                        sc->t0SendByteS = t0sbsIdle;
                        sc->t0ByteParent(sc,gcT0SendByte);
                    }

                    /* got error */
                    else
                    {
                        /* got parity error */
                        if (sc->shiftParityCount < PARITY_ERROR_MAX)
                        {
                            sc->shiftParityCount++;
                            scrTimeout(t0SendByteSM,sc,t0sbcTResend,sc->clkCountDataSend * 2);
                            sc->t0SendByteS = t0sbsWaitResend;
                        }
                        else
                        {
                            /* too many parity errors, just give up on this sc->dataByte */
                            sc->status = ERROR_PARITY;
                            sc->t0SendByteS = t0sbsIdle;
                            sc->t0ByteParent(sc,gcT0SendByteErr);
                        }
                    }
                    break;

                default:
                    INVALID_STATE_CMD(sc, sc->t0SendByteS,cmd);
                    break;
            }
            break;

        case t0sbsWaitResend:
            switch (cmd)
            {
                case t0sbcTResend:
                    sc->shiftBits   = 0;
                    sc->shiftParity = 0;
                    sc->shiftByte = sc->dataByte;
                    /*  set start bit */

                    scrTimeout(t0SendByteSM,sc,t0sbcTClockData,sc->clkCountDataSend);
                    scrSetData(FALSE);
                    sc->t0SendByteS = t0sbsClockData;
                    break;

                default:
                    INVALID_STATE_CMD(sc, sc->t0SendByteS,cmd);
                    break;
            }
            break;


        default:
            INVALID_STATE_CMD(sc, sc->t0SendByteS,cmd);
            break;
    }
}











/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      cardOff
**
**      Turn all signals to the card off
**
**  FORMAL PARAMETERS:
**
**      sc      -  Pointer to the softc structure.
**
**  IMPLICIT INPUTS:
**
**      nill
**
**  IMPLICIT OUTPUTS:
**
**      nill
**
**  FUNCTION VALUE:
**
**      nill
**
**  SIDE EFFECTS:
**
**      nill
**--
*/
static void   cardOff  (struct scr_softc * sc)
{
    scrSetReset(TRUE);
    scrSetDataHighZ();
    scrSetClock(FALSE);
    scrSetPower(FALSE);
}




/*
**
**
**    **************** timer routines ***************
**
*/

/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      scrClkInit
**
**      Init the callout queues.  The callout queues are used 
**      by the timeout/untimeout queues
**
**  FORMAL PARAMETERS:
**
**      nill
**
**  IMPLICIT INPUTS:
**
**      nill
**
**  IMPLICIT OUTPUTS:
**
**      nill
**
**  FUNCTION VALUE:
**
**      nill
**
**  SIDE EFFECTS:
**
**      nill
**--
*/
static void scrClkInit(void)
{

    int lc;
    Callout *c;
    Callout *new;

    scrClkCallTodo.c_next = NULL;
    scrClkCallFree = &scrClkCalloutArray[0];
    c = scrClkCallFree;

    for (lc = 1; lc < SCR_CLK_CALLOUT_COUNT; lc++)
    {
        new = &scrClkCalloutArray[lc];
        c->c_next = new;
        c = new;
    }

    c->c_next = NULL;
}


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      scrClkStart
**
**      This function starts the clock running.  The clock is reall the 
**      HAT clock (High Available Timer) that is using a FIQ (fast interrupt
**      request).   
**
**  FORMAL PARAMETERS:
**
**      sc              -  Pointer to the softc structure.
**      countPerTick    -  value for T2 timer  that drives FIQ
**
**  IMPLICIT INPUTS:
**
**      nill
**
**  IMPLICIT OUTPUTS:
**
**      nill
**
**  FUNCTION VALUE:
**
**      nill
**
**  SIDE EFFECTS:
**
**      nill
**--
*/
static void scrClkStart(struct scr_softc * sc,int countPerTick)
{
    u_int savedInts; 

    savedInts = disable_interrupts(I32_bit | F32_bit);

    

    ASSERT(scrClkCallTodo.c_next == NULL);
    ASSERT(!scrClkEnable);
    scrClkEnable = 1;
    scrClkCount = countPerTick;

    hatClkOn(countPerTick,
             hatClkIrq,
             0xdeadbeef,
             hatStack + HATSTACKSIZE - sizeof(unsigned),
             myHatWedge);

    restore_interrupts(savedInts);
}

/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      scrClkAdj
**
**      Adjusts the frequence of the clock
**
**  FORMAL PARAMETERS:
**
**      count   -  new value for T2 timer that drives FIQ
**
**  IMPLICIT INPUTS:
**
**      nill
**
**  IMPLICIT OUTPUTS:
**
**      nill
**
**  FUNCTION VALUE:
**
**      nill
**
**  SIDE EFFECTS:
**
**      nill
**--
*/
static void scrClkAdj (int count)
{   
    u_int savedInts; 

    if (count != scrClkCount)
    {
        savedInts = disable_interrupts(I32_bit | F32_bit);

        ASSERT(scrClkEnable);

        scrClkCount = count;
        hatClkAdjust(count);

        restore_interrupts(savedInts);
    }
}

/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      scrClkStop
**
**      Stops the clock
**
**  FORMAL PARAMETERS:
**
**      nill
**      
**
**  IMPLICIT INPUTS:
**
**      nill
**
**  IMPLICIT OUTPUTS:
**
**      nill
**
**  FUNCTION VALUE:
**
**      nill
**
**  SIDE EFFECTS:
**
**      nill
**--
*/
static void scrClkStop(void)
{
    u_int savedInts; 
    savedInts = disable_interrupts(I32_bit | F32_bit);

    ASSERT(scrClkEnable);
    scrClkEnable = 0;
    ASSERT(scrClkCallTodo.c_next == NULL);
    hatClkOff();

    restore_interrupts(savedInts);
}



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      hatClkIrq
**
**      This is what the HAT clock calls.   This call drives
**      the timeout queues, which in turn drive the state machines 
**      
**      Be very carefully when calling a timeout as the function
**      that is called may in turn do timeout/untimeout calls 
**      before returning 
**
**  FORMAL PARAMETERS:
**
**      int x       - not used 
**
**  IMPLICIT INPUTS:
**
**      nill
**
**  IMPLICIT OUTPUTS:
**
**      nill
**
**  FUNCTION VALUE:
**
**      nill
**
**  SIDE EFFECTS:
**
**      a timeout may be called if it is due
**--
*/
static void hatClkIrq(int  x)
{
    register Callout *p1;
    register int needsoft =0;
    register Callout *c;
    register int arg;
    register void (*func) __P((struct scr_softc*,int));
    struct scr_softc * sc;

    ASSERT(scrClkEnable);
    for (p1 = scrClkCallTodo.c_next; p1 != NULL; p1 = p1->c_next)
    {
        p1->c_time -= scrClkCount;

        if (p1->c_time > 0)
        {
            break;
        }
        needsoft = 1;
        if (p1->c_time == 0)
        {
            break;
        }
    }


    if (needsoft)
    {
        while ((c = scrClkCallTodo.c_next) != NULL && c->c_time <= 0)
        {
            func = c->c_func;
            sc = c->c_sc;
            arg = c->c_arg;
            scrClkCallTodo.c_next = c->c_next;
            c->c_next = scrClkCallFree;
            scrClkCallFree = c;
            (*func)(sc,arg);
        }
    }
}

/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      myHatWedge
**
**      Called if the HAT timer becomes clogged/wedged.  Not
**      used by this driver, we let upper layers recover
**      from this condition
**
**  FORMAL PARAMETERS:
**
**      int nFIQs - not used
**
**  IMPLICIT INPUTS:
**
**      nill
**
**  IMPLICIT OUTPUTS:
**
**      nill
**
**  FUNCTION VALUE:
**
**      nill
**
**  SIDE EFFECTS:
**
**      nill
**--
*/
static void myHatWedge(int nFIQs)
{
    #ifdef DEBUG
        printf("myHatWedge: nFIQ = %d\n",nFIQs);
    #endif    
}



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      scrTimeout
**
**	    Execute a function after a specified length of time.
**
**
**  FORMAL PARAMETERS:
**
**      ftn     -   function to execute
**      sc      -   pointer to soft c
**      arg     -   argument passed to function
**      count   -   number of T2 counts for timeout
**
**  IMPLICIT INPUTS:
**
**      nill
**
**  IMPLICIT OUTPUTS:
**
**      nill
**
**  FUNCTION VALUE:
**
**      nill
**
**  SIDE EFFECTS:
**
**      nill
**--
*/

static void scrTimeout(ftn, sc, arg, count)
    void (*ftn) __P((struct scr_softc*,int));
    struct scr_softc* sc;
    int arg;
    register int count;
{

    register Callout *new, *p, *t;
    ASSERT(scrClkEnable);


    if (count <= 0)
    {
        count = 1;
    }


    /* Fill in the next free fcallout structure. */
    if (scrClkCallFree == NULL)
    {
        panic("timeout table full");
    }

    new = scrClkCallFree;
    scrClkCallFree = new->c_next;
    new->c_sc  = sc;
    new->c_arg = arg;
    new->c_func = ftn;

    /*
     * The time for each event is stored as a difference from the time
     * of the previous event on the queue.  Walk the queue, correcting
     * the counts argument for queue entries passed.  Correct the counts
     * value for the queue entry immediately after the insertion point
     * as well.  Watch out for negative c_time values; these represent
     * overdue events.
     */
    for (p = &scrClkCallTodo; (t = p->c_next) != NULL && count > t->c_time; p = t)
    {
        if (t->c_time > 0)
        {
            count -= t->c_time;
        }
    }


    new->c_time = count;
    if (t != NULL)
    {
        t->c_time -= count;
    }

    /* Insert the new entry into the queue. */
    p->c_next = new;
    new->c_next = t;
}

/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      scrUntimeout
**
**	    Cancel previous timeout function call.
**
**  FORMAL PARAMETERS:
**
**      ftn     - function of timeout to cancel
**      sc      - sc  of timeout to cancel
**      arg     - arg of timeout to cancel
**
**  IMPLICIT INPUTS:
**
**      nill
**
**  IMPLICIT OUTPUTS:
**
**      nill
**
**  FUNCTION VALUE:
**
**      nill
**
**  SIDE EFFECTS:
**
**      nill
**--
*/
static void scrUntimeout(ftn, sc, arg)
void (*ftn) __P((struct scr_softc*,int));
struct scr_softc* sc;
int arg;
{
    register Callout *p, *t;
    ASSERT(scrClkEnable);

    for (p = &scrClkCallTodo; (t = p->c_next) != NULL; p = t)
    {
        if (t->c_func == ftn && t->c_sc == sc && t->c_arg == arg)
        {
            /* Increment next entry's count. */
            if (t->c_next && t->c_time > 0)
            {
                t->c_next->c_time += t->c_time;
            }

            /* Move entry from fcallout queue to scrClkCallFree queue. */
            p->c_next = t->c_next;
            t->c_next = scrClkCallFree;
            scrClkCallFree = t;
            break;
        }
    }
}















/******************* routines used only during debugging */
#ifdef SCR_DEBUG

/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      invalidStateCmd
**
**      Debugging function.  Printout information about problem
**      and then kick in the debugger or panic
**
**  FORMAL PARAMETERS:
**
**      sc      - pointer to soft c
**      state   - state of machine 
**      cmd     - command of machine
**      line    - line that problem was detected
**
**
**  IMPLICIT INPUTS:
**
**      nill
**
**  IMPLICIT OUTPUTS:
**
**      nill
**
**  FUNCTION VALUE:
**
**      nill
**
**  SIDE EFFECTS:
**
**      nill
**--
*/
void invalidStateCmd (struct scr_softc* sc,int state,int cmd,int line)
{
    printf("INVALID_STATE_CMD: sc = %X, state = %X, cmd = %X, line = %d\n",sc,state,cmd,line);
    DEBUGGER;
}

/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      getText
**
**      Get text representation of state or command
**
**  FORMAL PARAMETERS:
**
**      x   - state or command
**
**  IMPLICIT INPUTS:
**
**      nill
**
**  IMPLICIT OUTPUTS:
**
**      nill
**
**  FUNCTION VALUE:
**
**      nill
**
**  SIDE EFFECTS:
**
**      nill
**--
*/
char * getText(int x)
{
    switch (x)
    {
            /* commands to Master State Machine (mc = Master Command )*/
        case    mcOn:               return "mcOn";
        case    mcT0DataSend:       return "mcT0DataSend";
        case    mcT0DataRecv:       return "mcT0DataRecv";
        case    mcColdReset:        return "mcColdReset";
        case    mcATR:              return "mcATR";
        case    mcT0Send:           return "mcT0Send";
        case    mcT0Recv:           return "mcT0Recv";

            /* states in Master state machine (ms = Master State) */
        case    msIdleOff:          return "msIdleOff";
        case    msColdReset:        return "msColdReset";
        case    msATR:              return "msATR";
        case    msIdleOn:           return "msIdleOn";
        case    msT0Send:           return "msT0Send";
        case    msT0Recv:           return "msT0Recv";



            /* commands to T0 send state machine */
        case    t0scStart:          return "t0scStart";
        case    t0scTWorkWaiting:   return "t0scTWorkWaiting";


            /* states in T0 send state machine */
        case    t0ssIdle:           return "t0ssIdle";
        case    t0ssSendHeader:     return "t0ssSendHeader";
        case    t0ssRecvProcedure:  return "t0ssRecvProcedu";
        case    t0ssSendByte:       return "t0ssSendByte";
        case    t0ssSendData:       return "t0ssSendData";
        case    t0ssRecvSW1:        return "t0ssRecvSW1";
        case    t0ssRecvSW2:        return "t0ssRecvSW2";


            /* commands to T0 recv state machine */
        case t0rcStart:             return "t0rcStart";
        case t0rcTWorkWaiting:      return "t0rcTWorkWaiting";

            /* states in T0 recv state machine */
        case t0rsIdle:              return "t0rsIdle";
        case t0rsSendHeader:        return "t0rsSendHeader";
        case t0rsRecvProcedure:     return "t0rsRecvProcedure";
        case t0rsRecvByte:          return "t0rsRecvByte";
        case t0rsRecvData:          return "t0rsRecvData";
        case t0rsRecvSW1:           return "t0rsRecvSW1";
        case t0rsRecvSW2:           return "t0rsRecvSW2";





            /* commands to Answer To Reset (ATR) state machine */
        case    atrcStart:      return "atrcStart";
        case    atrcT3:         return "0x0b04";
        case    atrcTWorkWaiting: return "atrcTWorkWaiting";


            /* states in in Anser To Reset (ATR) state machine */
        case    atrsIdle:        return "atrsIdle";
        case    atrsTS:          return "atrsTS";
        case    atrsT0:          return "atrsT0";
        case    atrsTABCD:       return "atrsTABCD";
        case    atrsTK:          return "atrsTK";
        case    atrsTCK:         return "atrsTCK";



            /* commands to T0 Recv Byte state machine */
        case    t0rbcStart:         return "t0rbcStart";
        case    t0rbcAbort:         return "t0rbcAbort";

        case    t0rbcTFindStartEdge:return "t0rbcTFindStartEdge";
        case    t0rbcTFindStartMid: return "t0rbcTFindStartMid";
        case    t0rbcTClockData:    return "t0rbcTClockData";
        case    t0rbcTErrorStart:   return "t0rbcTErrorStart";
        case    t0rbcTErrorStop:    return "t0rbcTErrorStop";

            /* states in in TO Recv Byte state machine */
        case    t0rbsIdle:          return "t0rbsIdle";
        case    t0rbsFindStartEdge: return "t0rbcFindStartEdge";    
        case    t0rbsFindStartMid:  return "t0rbcFindStartMid"; 
        case    t0rbsClockData:     return "t0rbcClockData";    
        case    t0rbsSendError:     return "t0rbcSendError";    


            /* commands to T0 Send Byte  state machine */
        case    t0sbcStart:         return "t0sbcStart";
        case    t0sbcAbort:         return "t0sbcAbort";
        case    t0sbcTGuardTime:    return "t0sbcTGuardTime";
        case    t0sbcTClockData:    return "t0sbcTClockData";
        case    t0sbcTError:        return "t0sbcTError";
        case    t0sbcTResend:       return "t0sbcTResend";

            /* states in in T0 Send Byte state machine */
        case    t0sbsIdle:          return "t0sbsIdle";
        case    t0sbsClockData:     return "t0sbsClockData";
        case    t0sbsWaitError:     return "t0sbsWaitError";
        case    t0sbsWaitResend:    return "t0sbsWaitResend";
        case    t0sbsWaitGuardTime: return "t0sbsWaitGuardTime";


        case    gcT0RecvByte:       return     "gcT0RecvByte";
        case gcT0RecvByteErr:       return "gcT0RecvByteErr";
        case gcT0SendByte:          return "gcT0SendByte";
        case gcT0SendByteErr:       return "gcT0SendByteErr";


        case crcStart:              return "crcStart";
        case crcT2:                 return "crcT2";


        default:
            printf("unkown case, %x\n",x);
            break;
    }
    return "???";
}

#endif /*  SCR_DEBUG */



