#ifndef CDKSCREEN_H
#define CDKSCREEN_H	1

#include <cdk/cdk.h>

/*
 * Copyright 1999, Mike Glover
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
 *    must display the following acknowledgment:
 *      This product includes software developed by Mike Glover
 *      and contributors.
 * 4. Neither the name of Mike Glover, nor the names of contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MIKE GLOVER AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL MIKE GLOVER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Declare and definitions needed for the widget.
 */
#define MAX_OBJECTS	1000
#define MAX_WIDGETS	MAX_OBJECTS

struct CDKOBJS;

/*
 * Define the CDK screen structure.
 */
struct SScreen {
   WINDOW *		window;
   struct CDKOBJS *	object[MAX_OBJECTS];
   EObjectType		cdktype[MAX_OBJECTS];
   int			objectCount;
};
typedef struct SScreen CDKSCREEN;

/*
 * This function creates a CDKSCREEN pointer.
 */
CDKSCREEN *initCDKScreen (
		WINDOW *	/* window */);

/*
 * This registers a CDKSCREEN. You would do this if you had more
 * than one screen created and you wanted them managed.
 */
int registerCDKScreen (
		CDKSCREEN **	/* screens */);

/*
 * This removes the given CDKSCREEN pointer from the array
 * of managed screens.
 */
int unregisterCDKScreen (
		CDKSCREEN **	/* screens */);

/*
 * This raises a screen in a list of managed screens.
 */
void raiseCDKScreen (
		CDKSCREEN **	/* screens */);

/*
 * This lowers a screen in a list of managed screens.
 */
void lowerCDKScreen (
		CDKSCREEN **	/* screens */);

/*
 * This sets which CDKSCREEN pointer will be the default screen
 * in the list of managed screen.
 */
CDKSCREEN *setDefaultCDKScreen (
		int		/* screenNumber */);

/*
 * This function registers a CDK widget with a given screen.
 */
void registerCDKObject (
		CDKSCREEN *	/* screen */,
		EObjectType	/* cdktype */,
		void *		/* object */);

/*
 * This unregisters a CDK widget from a screen.
 */
void unregisterCDKObject (
		EObjectType	/* cdktype */,
		void *		/* object */);

/*
 * This function raises a widget on a screen to the top of the widget
 * stack of the screen the widget is associated with. The side effect
 * of doing this is the widget will be on 'top' of all the other
 * widgets on their screens.
 */
void raiseCDKObject (
		EObjectType	/* cdktype */,
		void *		/* object */);

/*
 * This function lowers a widget on a screen to the bottom of the widget
 * stack of the screen the widget is associated with. The side effect
 * of doing this is that all the other widgets will be on 'top' of the
 * widget on their screens.
 */
void lowerCDKObject (
		EObjectType	/* cdktype */,
		void *		/* object */);

/*
 * This redraws all the widgets associated with the given screen.
 */
void refreshCDKScreen (
		CDKSCREEN *	/* screen */);

/*
 * This calls refreshCDKScreen. (it is here to try to be consistent
 * with the drawCDKXXX functions associated with the widgets)
 */
void drawCDKScreen (
		CDKSCREEN *	/* screen */);

/*
 * This removes all the widgets from the screen.
 */
void eraseCDKScreen (
		CDKSCREEN *	/* screen */);

/*
 * This frees up any memory the CDKSCREEN pointer used.
 */
void destroyCDKScreen (
		CDKSCREEN *	/* screen */);

/*
 * This shuts down curses and everything else needed to
 * exit cleanly.
 */
void endCDK(void);

/*
 * This creates all the color pairs.
 */
void initCDKColor(void);

#endif /* CDKSCREEN_H */
