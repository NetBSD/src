/* $Header: /cvsroot/src/sys/netiso/Attic/tp_states.h,v 1.1.1.1 1998/03/01 02:10:26 fvdl Exp $ */
/* $Source: /cvsroot/src/sys/netiso/Attic/tp_states.h,v $ */
#define ST_ERROR 0x0
#define TP_CLOSED 0x1
#define TP_CRSENT 0x2
#define TP_AKWAIT 0x3
#define TP_OPEN 0x4
#define TP_CLOSING 0x5
#define TP_REFWAIT 0x6
#define TP_LISTENING 0x7
#define TP_CONFIRMING 0x8

#define tp_NSTATES 0x9
