/* $Header: /cvsroot/src/games/warp/object.c,v 1.1 2020/11/09 23:37:05 kamil Exp $ */

/* $Log: object.c,v $
/* Revision 1.1  2020/11/09 23:37:05  kamil
/* Add Warp Kit, Version 7.0 by Larry Wall
/*
/* Warp is a real-time space war game that doesn't get boring very quickly.
/* Read warp.doc and the manual page for more information.
/*
/* games/warp originally distributed with 4.3BSD-Reno, is back to the BSD
/* world via NetBSD. Its remnants were still mentioned in games/Makefile.
/*
/* Larry Wall, the original author and the copyright holder, generously
/* donated the game and copyright to The NetBSD Foundation, Inc.
/*
/* Import the game sources as-is from 4.3BSD-Reno, with the cession
/* of the copyright and license to BSD-2-clause NetBSD-style.
/*
/* Signed-off-by: Larry Wall <larry@wall.org>
/* Signed-off-by: Kamil Rytarowski <kamil@netbsd.org>
/*
 * Revision 7.0  86/10/08  15:12:55  lwall
 * Split into separate files.  Added amoebas and pirates.
 * 
 */

#include "EXTERN.h"
#include "warp.h"
#include "INTERN.h"
#include "object.h"

void
object_init()
{
    ;
}

OBJECT *
make_object(typ, img, py, px, vy, vx, energ, mas, where)
char typ;
char img;
int px, py, vx, vy;
long energ, mas;
OBJECT *where;
{
    Reg1 OBJECT *obj;

    if (free_root.next == &free_root)
#ifndef lint
	obj = (OBJECT *) malloc(sizeof root);
#else
	obj = Null(OBJECT*);
#endif
    else {
	obj = free_root.next;
	free_root.next = obj->next;
	obj->next->prev = &free_root;
    }
    obj->type = typ;
    obj->image = img;
    obj->next = where;
    obj->prev = where->prev;
    where->prev = obj;
    obj->prev->next = obj;
    obj->velx = vx;
    obj->vely = vy;
    obj->contend = 0;
    obj->strategy = 0;
    obj->flags = 0;
    obj->posx = px;
    obj->posy = py;
    if (typ != Torp && typ != Web) {
	occupant[py][px] = obj;
    }
    obj->energy = energ;
    obj->mass = mas;
    return(obj);
}

void
unmake_object(curobj)
Reg1 OBJECT *curobj;
{
    curobj->prev->next = curobj->next;
    curobj->next->prev = curobj->prev;
    if (curobj == movers) {
	movers = curobj->next;
    }
    free_object(curobj);
}

void
free_object(curobj)
Reg1 OBJECT *curobj;
{
    curobj->next = free_root.next;
    curobj->prev = &free_root;
    free_root.next->prev = curobj;
    free_root.next = curobj;
}
