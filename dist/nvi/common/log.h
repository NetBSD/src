/*-
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1992, 1993, 1994, 1995, 1996
 *	Keith Bostic.  All rights reserved.
 *
 * See the LICENSE file for redistribution information.
 *
 *	$Id: log.h,v 1.1.1.1.2.2 2008/05/18 12:29:23 yamt Exp $ (Berkeley) $Date: 2008/05/18 12:29:23 $
 */

#define	LOG_NOTYPE		0
#define	LOG_CURSOR_INIT		2
#define	LOG_CURSOR_END		3
#define	LOG_LINE_APPEND_B	4
#define	LOG_LINE_APPEND_F	5
#define	LOG_LINE_DELETE_B	6
#define	LOG_LINE_DELETE_F	7
#define	LOG_LINE_RESET_B	8
#define	LOG_LINE_RESET_F	9
#define	LOG_MARK		10	

typedef enum { UNDO_FORWARD, UNDO_BACKWARD, UNDO_SETLINE } undo_t;

struct _log_state {
	int	didop;
	MARK	pos;
	undo_t	undo;
};
