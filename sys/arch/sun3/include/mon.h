/*
 * This file derived from kernel/mach/sun3.md/machMon.h from the
 * sprite distribution.
 *
 * In particular, this file came out of the Walnut Creek cdrom collection
 * which contained no warnings about any possible copyright infringement
 * It was also indentical to a file in the sprite kernel tar file found on
 * allspice.berkeley.edu.
 * It also written in the annoying sprite coding style.  I've made
 * efforts not to heavily edit their file, just ifdef parts out. -- glass
 */

#ifndef MON_H
#define MON_H
/*
 * machMon.h --
 *
 *     Structures, constants and defines for access to the sun monitor.
 *     These are translated from the sun monitor header file "sunromvec.h".
 *
 * NOTE: The file keyboard.h in the monitor directory has all sorts of useful
 *       keyboard stuff defined.  I haven't attempted to translate that file
 *       because I don't need it.  If anyone wants to use it, be my guest.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * Header: /cdrom/src/kernel/Cvsroot/kernel/mach/sun3.md/machMon.h,v 9.1 90/10/03 13:52:34 mgbaker Exp SPRITE (Berkeley)
 */

#ifndef _MACHMON
#define _MACHMON

/*
 * The memory addresses for the PROM, and the EEPROM.
 * On the sun2 these addresses are actually 0x00EF??00
 * but only the bottom 24 bits are looked at so these still
 * work ok.
 */

#define PROM_BASE       0x0fef0000

/*
 * The table entry that describes a device.  It exists in the PROM; a
 * pointer to it is passed in MachMonBootParam.  It can be used to locate
 * PROM subroutines for opening, reading, and writing the device.
 *
 * When using this interface, only one device can be open at once.
 *
 * NOTE: I am not sure what arguments boot, open, close, and strategy take.  
 * What is here is just translated verbatim from the sun monitor code.  We 
 * should figure this out eventually if we need it.
 */

typedef struct {
	char	devName[2];		/* The name of the device */
	int	(*probe)();		/* probe() --> -1 or found controller 
					   number */
	int	(*boot)();		/* boot(bp) --> -1 or start address */
	int	(*open)();		/* open(iobp) --> -1 or 0 */
	int	(*close)();		/* close(iobp) --> -1 or 0 */
	int	(*strategy)();		/* strategy(iobp,rw) --> -1 or 0 */
	char	*desc;			/* Printable string describing dev */
} MachMonBootTable;

/*
 * Structure set up by the boot command to pass arguments to the program that
 * is booted.
 */

typedef struct {
	char		*argPtr[8];	/* String arguments */
	char		strings[100];	/* String table for string arguments */
	char		devName[2];	/* Device name */
	int		ctlrNum;	/* Controller number */
	int		unitNum;	/* Unit number */
	int		partNum;	/* Partition/file number */
	char		*fileName;	/* File name, points into strings */
	MachMonBootTable   *bootTable;	/* Points to table entry for device */
} MachMonBootParam;

/*
 * Here is the structure of the vector table which is at the front of the boot
 * rom.  The functions defined in here are explained below.
 *
 * NOTE: This struct has references to the structures keybuf and globram which
 *       I have not translated.  If anyone needs to use these they should
 *       translate these structs into Sprite format.
 */

typedef struct {
	char		*initSp;		/* Initial system stack ptr  
						 * for hardware */
	int		(*startMon)();		/* Initial PC for hardware */

	int		*diagberr;		/* Bus err handler for diags */

	/* 
	 * Monitor and hardware revision and identification
	 */

	MachMonBootParam **bootParam;		/* Info for bootstrapped pgm */
 	unsigned	*memorySize;		/* Usable memory in bytes */

	/* 
	 * Single-character input and output 
	 */

	unsigned char	(*getChar)();		/* Get char from input source */
	int		(*putChar)();		/* Put char to output sink */
	int		(*mayGet)();		/* Maybe get char, or -1 */
	int		(*mayPut)();		/* Maybe put char, or -1 */
	unsigned char	*echo;			/* Should getchar echo? */
	unsigned char	*inSource;		/* Input source selector */
	unsigned char	*outSink;		/* Output sink selector */

	/* 
	 * Keyboard input (scanned by monitor nmi routine) 
	 */

	int		(*getKey)();		/* Get next key if one exists */
	int		(*initGetKey)();	/* Initialize get key */
	unsigned int	*translation;		/* Kbd translation selector 
						   (see keyboard.h in sun 
						    monitor code) */
	unsigned char	*keyBid;		/* Keyboard ID byte */
	int		*screen_x;		/* V2: Screen x pos (R/O) */
	int		*screen_y;		/* V2: Screen y pos (R/O) */
	struct keybuf	*keyBuf;		/* Up/down keycode buffer */

	/*
	 * Monitor revision level.
	 */

	char		*monId;

	/* 
	 * Frame buffer output and terminal emulation 
	 */

	int		(*fbWriteChar)();	/* Write a character to FB */
	int		*fbAddr;		/* Address of frame buffer */
	char		**font;			/* Font table for FB */
	int		(*fbWriteStr)();	/* Quickly write string to FB */

	/* 
	 * Reboot interface routine -- resets and reboots system.  No return. 
	 */

	int		(*reBoot)();		/* e.g. reBoot("xy()vmunix") */

	/* 
	 * Line input and parsing 
	 */

	unsigned char	*lineBuf;		/* The line input buffer */
	unsigned char	**linePtr;		/* Cur pointer into linebuf */
	int		*lineSize;		/* length of line in linebuf */
	int		(*getLine)();		/* Get line from user */
	unsigned char	(*getNextChar)();	/* Get next char from linebuf */
	unsigned char	(*peekNextChar)();	/* Peek at next char */
	int		*fbThere;		/* =1 if frame buffer there */
	int		(*getNum)();		/* Grab hex num from line */

	/* 
	 * Print formatted output to current output sink 
	 */

	int		(*printf)();		/* Similar to "Kernel printf" */
	int		(*printHex)();		/* Format N digits in hex */

	/*
	 * Led stuff 
	 */

	unsigned char	*leds;			/* RAM copy of LED register */
	int		(*setLeds)();		/* Sets LED's and RAM copy */

	/* 
	 * Non-maskable interrupt  (nmi) information
	 */ 

	int		(*nmiAddr)();		/* Addr for level 7 vector */
	int		(*abortEntry)();	/* Entry for keyboard abort */
	int		*nmiClock;		/* Counts up in msec */

	/*
	 * Frame buffer type: see <sun/fbio.h>
	 */

	int		*fbType;

	/* 
	 * Assorted other things 
	 */

	unsigned	romvecVersion;		/* Version # of Romvec */ 
	struct globram  *globRam;		/* monitor global variables */
	Address		kbdZscc;		/* Addr of keyboard in use */

	int		*keyrInit;		/* ms before kbd repeat */
	unsigned char	*keyrTick; 		/* ms between repetitions */
	unsigned	*memoryAvail;		/* V1: Main mem usable size */
	long		*resetAddr;		/* where to jump on a reset */
	long		*resetMap;		/* pgmap entry for resetaddr */
						/* Really struct pgmapent *  */
	int		(*exitToMon)();		/* Exit from user program */
	unsigned char	**memorybitmap;		/* V1: &{0 or &bits} */
	void		(*setcxsegmap)();	/* Set seg in any context */
	void		(**vector_cmd)();	/* V2: Handler for 'v' cmd */
	int		dummy1z;
	int		dummy2z;
	int		dummy3z;
	int		dummy4z;
} MachMonRomVector;

/*
 * Functions defined in the vector:
 *
 *
 * getChar -- Return the next character from the input source
 *
 *     unsigned char getChar()
 *
 * putChar -- Write the given character to the output source.
 *
 *     void putChar(ch)
 *	   char ch;	
 *
 * mayGet -- Maybe get a character from the current input source.  Return -1 
 *           if don't return a character.
 *
 * 	int mayGet()
 *	
 * mayPut -- Maybe put a character to the current output source.   Return -1
 *           if no character output.
 *
 *	int  mayPut(ch)
 *	    char ch;
 *
 * getKey -- Returns a key code (if up/down codes being returned),
 * 	     a byte of ASCII (if that's requested),
 * 	     NOKEY (if no key has been hit).
 *
 *	int getKey()
 *	
 * initGetKey --  Initialize things for get key.
 *
 *	void initGetKey()
 *
 * fbWriteChar -- Write a character to the frame buffer
 *
 *	void fwritechar(ch)
 *	    unsigned char ch;
 *
 * fbWriteStr -- Write a string to the frame buffer.
 *
 *   	void fwritestr(addr,len)
 *  	    register unsigned char *addr;	/ * String to be written * /
 *  	    register short len;			/ * Length of string * /
 *
 * getLine -- read the next input line into a global buffer
 *
 *	getline(echop)
 *          int echop;	/ * 1 if should echo input, 0 if not * /
 *
 * getNextChar -- return the next character from the global line buffer.
 *
 *	unsigned char getNextChar()
 *
 * peekNextChar -- look at the next character in the global line buffer.
 *
 *	unsigned char peekNextChar()
 *
 * getNum -- Grab hex num from the global line buffer.
 *
 *	int getNum()
 *
 * printf -- Scaled down version of C library printf.  Only %d, %x, %s, and %c
 * 	     are recognized.
 *
 * printhex -- prints rightmost <digs> hex digits of <val>
 *
 *      printhex(val,digs)
 *          register int val;
 *     	    register int digs;
 *
 * abortEntry -- Entry for keyboard abort.
 *
 *     abortEntry()
 */

/*
 * Where the rom vector is defined.
 */

#define	romVectorPtr	((MachMonRomVector *) PROM_BASE)

#if 0
/*
 * Functions and defines to access the monitor.
 */

#define Mach_MonPrintf (romVectorPtr->printf)

extern void Mach_MonPutChar _ARGS_((int ch));
extern int Mach_MonMayPut _ARGS_((int ch));
extern void Mach_MonAbort _ARGS_((void));
extern void Mach_MonReboot _ARGS_((char *rebootString));
extern void Mach_MonStartNmi _ARGS_((void));
extern void Mach_MonStopNmi _ARGS_((void));

extern  void    Mach_MonTrap _ARGS_((Address address_to_trap_to));

/*
 * These routines no longer work correctly with new virtual memory.
 */

#define Mach_MonGetChar (romVectorPtr->getChar)
#define Mach_MonGetLine (romVectorPtr->getLine)
#define Mach_MonGetNextChar (romVectorPtr->getNextChar)
#define Mach_MonPeekNextChar (romVectorPtr->peekNextChar)
#endif

#define mon_printf (romVectorPtr->printf)
#define mon_putchar (romVectorPtr->printf)
#define mon_exit_to_mon (romVectorPtr->exitToMon)
#endif /* _MACHMON */
#endif /* MON_H */     
