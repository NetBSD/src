/*	$NetBSD: xkbcomp-stubs.c,v 1.1.1.1 2003/09/11 18:37:09 lukem Exp $	*/

#include <stdio.h>

#include "Xlibint.h"
#include "Xlcint.h"
#include "XKBlibint.h"
#include <X11/extensions/XKBfile.h>

Display *
XOpenDisplay(const char *display)
{
	return NULL;
}

int
XCloseDisplay(Display *dpy)
{
	return 0;
}

int (*
XSynchronize(Display *dpy, int onoff))()
{
	return NULL;
}

XrmMethods
_XrmInitParseInfo(XPointer *state)
{
	return NULL;
}

int
XGetErrorText(Display *dpy, int code, char *buffer, int nbytes)
{
	return 0;
}


char *
XGetAtomName(Display *dpy, Atom atom)
{
	return NULL;
}

Atom
XInternAtom(Display *dpy, const char *name, Bool onlyIfExists)
{
	return None;
}

XkbDescPtr
XkbGetMap(Display *dpy,unsigned which,unsigned deviceSpec)
{
	return NULL;
}

Status
XkbGetIndicatorMap(Display *dpy,unsigned long which,XkbDescPtr xkb)
{
	return BadValue;
}

Status
XkbGetControls(Display *dpy, unsigned long which, XkbDescPtr xkb)
{
	return BadValue;
}

Status
XkbGetCompatMap(Display *dpy,unsigned which,XkbDescPtr xkb)
{
	return BadValue;
}

Status
XkbGetNames(Display *dpy,unsigned which,XkbDescPtr xkb)
{
	return BadValue;
}

Status
XkbChangeKbdDisplay(Display *newDpy,XkbFileInfo *result)
{
	return BadValue;
}

Bool
XkbWriteToServer(XkbFileInfo *result)
{
	return False;
}

void
_XFlush(Display *dpy)
{
}

Bool
XkbUseExtension(Display *dpy,int *major_rtrn,int *minor_rtrn)
{
	return False;
}

Status
_XReply(Display *dpy, xReply *rep, int extra, Bool discard)
{
	return False;
}

int
_XRead(Display *dpy, char *data, long size)
{
	return 0;
}



/*		=========		*/
#if 0

Status
XkbGetGeometry(Display *dpy,XkbDescPtr xkb)
{
	return BadValue;
}

Display *
XkbOpenDisplay(char * name, int * ev_rtrn, int * err_rtrn, int * major_rtrn,
    int * minor_rtrn, int * reason)
{
	return NULL;
}

Bool
XkbLibraryVersion(int *libMajorRtrn,int *libMinorRtrn)
{
	return True;
}

int
XFlush(Display *dpy)
{
	return 1;
}

#endif
