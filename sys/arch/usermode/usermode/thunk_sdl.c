/* $NetBSD: thunk_sdl.c,v 1.1 2011/08/25 11:45:26 jmcneill Exp $ */

/*-
 * Copyright (c) 2011 Jared D. McNeill <jmcneill@invisible.ca>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: thunk_sdl.c,v 1.1 2011/08/25 11:45:26 jmcneill Exp $");

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/reboot.h>
#include <sys/shm.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <SDL.h>

#include "../include/thunk.h"

extern const char ostype[];
extern const char osrelease[];
extern const char kernel_ident[];

#define THUNK_SDL_REDRAW_INTERVAL	100	/* 10 Hz */

#define THUNK_SDL_EV_REDRAW		1

static int thunk_sdl_running = 0;	/* SDL event loop run status */
static void *thunk_sdl_fbaddr = NULL;	/* framebuffer va */
static int thunk_sdl_shmid;

/* Framebuffer parameters */
static unsigned int thunk_sdl_width;
static unsigned int thunk_sdl_height;
static unsigned short thunk_sdl_depth;

/*
 * Create an SDL surface from the framebuffer memory, then blit
 * it to the screen.
 */
static void
thunk_sdl_redraw(SDL_Surface *screen)
{
	SDL_Surface *wsfb;
	Uint32 rmask, gmask, bmask, amask;

	if (thunk_sdl_fbaddr == NULL)
		return;

	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0x00000000;

	wsfb = SDL_CreateRGBSurfaceFrom(thunk_sdl_fbaddr,
	    thunk_sdl_width, thunk_sdl_height, thunk_sdl_depth,
	    thunk_sdl_width * (thunk_sdl_depth / 8),
	    rmask, gmask, bmask, amask);
	if (wsfb == NULL)
		abort();
	if (SDL_BlitSurface(wsfb, NULL, screen, NULL) == -1)
		abort();
	SDL_Flip(screen);
	SDL_FreeSurface(wsfb);
}

/*
 * Timer callback used to inject a redraw event into the SDL event loop
 */
static Uint32
thunk_sdl_tick(Uint32 interval, void *param)
{
	SDL_Event ev;

	if (!thunk_sdl_running)
		return 0;

	ev.type = SDL_USEREVENT;
	ev.user.code = THUNK_SDL_EV_REDRAW;
	ev.user.data1 = ev.user.data2 = NULL;
	SDL_PushEvent(&ev);

	return interval;
}

/*
 * SDL event loop
 */
static int
thunk_sdl_eventloop(void *priv)
{
	SDL_Surface *screen;
	char caption[81];
	//int stdin_fd;
	SDL_Event ev;
	key_t k;

	thunk_sdl_running = 1;

	k = ftok("/dev/null", 1);
	if (k == (key_t)-1)
		return errno;
	thunk_sdl_shmid = shmget(k,
	    thunk_sdl_width * thunk_sdl_height * (thunk_sdl_depth / 8), 0600);
	thunk_sdl_fbaddr = shmat(thunk_sdl_shmid, NULL, 0);
	if (thunk_sdl_fbaddr == NULL)
		abort();

	/* SDL_Init closes stdin, so we need to restore it after it returns */
	//stdin_fd = dup(0);
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) < 0)
		abort();
	atexit(SDL_Quit);
	//close(0);
	//dup2(stdin_fd, 0);

	/* Clear SDL signal handler so ^C will work */
	//signal(SIGINT, SIG_DFL);

	/* Configure key repeat */
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_DELAY);

	/* Set the window title */
	memset(caption, 0, sizeof(caption));
	snprintf(caption, sizeof(caption), "%s %s (%s)",
	    ostype, osrelease, kernel_ident); 
	SDL_WM_SetCaption(caption, NULL);

	/* Try to configure the requested video mode */
	screen = SDL_SetVideoMode(thunk_sdl_width, thunk_sdl_height,
	    thunk_sdl_depth, 0);
	if (screen == NULL)
		abort();

	/* Hide the mouse cursor */
	SDL_ShowCursor(SDL_DISABLE);

	SDL_AddTimer(THUNK_SDL_REDRAW_INTERVAL, thunk_sdl_tick, NULL);

	while (thunk_sdl_running) {
		if (!SDL_WaitEvent(&ev))
			abort();
		printf("ev.type = %d\n", ev.type);
		switch (ev.type) {
		case SDL_QUIT:
			thunk_sdl_running = 0;
			break;
		case SDL_KEYDOWN:
			printf("key %X scancode %X\n", ev.key.keysym.sym,
			    ev.key.keysym.scancode);
			break;
		case SDL_USEREVENT:
			switch (ev.user.code) {
			case THUNK_SDL_EV_REDRAW:
				thunk_sdl_redraw(screen);
				break;
			}
		}

		thunk_sdl_redraw(screen);
	}

	printf("shutting down...\n");
	abort();
	return 0;
}

int
thunk_sdl_init(unsigned int width, unsigned int height, unsigned short depth)
{
	static int thunk_sdl_inited = 0;
	static int thunk_sdl_init_status = -1;

	if (thunk_sdl_inited)
		goto done;
	thunk_sdl_inited = 1;

	/* Save a copy of the video mode parameters for later */
	thunk_sdl_width = width;
	thunk_sdl_height = height;
	thunk_sdl_depth = depth;

	/* run event loop */
	switch (fork()) {
	case 0:
		thunk_sdl_eventloop(NULL);
		break;
	case -1:
		abort();
	default:
		break;
	}

	thunk_sdl_init_status = 0;

done:
	return thunk_sdl_init_status;
}

/*
 * Get the framebuffer virtual address
 */
void *
thunk_sdl_getfb(size_t fbsize)
{
	key_t k;

	k = ftok("/dev/null", 1);
	if (k == (key_t)-1)
		return NULL;

	thunk_sdl_shmid = shmget(k, fbsize, IPC_CREAT | 0600);
	return shmat(thunk_sdl_shmid, NULL, 0);
}
