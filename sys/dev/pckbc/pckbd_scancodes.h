/* $NetBSD: pckbd_scancodes.h,v 1.1 1998/03/22 15:41:28 drochner Exp $ */

#define	PCKBD_CODE_SIZE 4	/* Use a max of 4 for now... */
#define PCKBD_NUM_KEYS	128	/* Number of scan codes */

typedef struct {
	u_short	type;
	char unshift[PCKBD_CODE_SIZE];
	char shift[PCKBD_CODE_SIZE];
	char ctl[PCKBD_CODE_SIZE];
	char altgr[PCKBD_CODE_SIZE];
} pckbd_scan_def;

/*
 * DANGER WIL ROBINSON -- the values of SCROLL, NUM, CAPS, and ALT are
 * important.
 */
#define	SCROLL		0x0001	/* stop output */
#define	NUM		0x0002	/* numeric shift  cursors vs. numeric */
#define	CAPS		0x0004	/* caps shift -- swaps case of letter */
#define	SHIFT		0x0008	/* keyboard shift */
#define	CTL		0x0010	/* control shift  -- allows ctl function */
#define	ASCII		0x0020	/* ascii code for this key */
#define ALTGR           0x0040  /* Alt graphic */
#define	ALT		0x0080	/* alternate shift -- alternate chars */
#define	FUNC		0x0100	/* function key */
#define	KP		0x0200	/* Keypad keys */
#define	NONE		0x0400	/* no function */

extern pckbd_scan_def
	pckbd_scan_codes_de[],
	pckbd_scan_codes_fi[],
	pckbd_scan_codes_fr[],
	pckbd_scan_codes_no[],
	pckbd_scan_codes_us[];
