/* Remote notification in GDB protocol

   Copyright (C) 1988-2016 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef REMOTE_NOTIF_H
#define REMOTE_NOTIF_H

#include "queue.h"

/* An event of a type of async remote notification.  */

struct notif_event
{
  /* Destructor.  Release everything from SELF, but not SELF
     itself.  */
  void (*dtr) (struct notif_event *self);
};

/* ID of the notif_client.  */

enum REMOTE_NOTIF_ID
{
  REMOTE_NOTIF_STOP = 0,
  REMOTE_NOTIF_LAST,
};

/* A client to a sort of async remote notification.  */

typedef struct notif_client
{
  /* The name of notification packet.  */
  const char *name;

  /* The packet to acknowledge a previous reply.  */
  const char *ack_command;

  /* Parse BUF to get the expected event and update EVENT.  This
     function may throw exception if contents in BUF is not the
     expected event.  */
  void (*parse) (struct notif_client *self, char *buf,
		 struct notif_event *event);

  /* Send field <ack_command> to remote, and do some checking.  If
     something wrong, throw an exception.  */
  void (*ack) (struct notif_client *self, char *buf,
	       struct notif_event *event);

  /* Check this notification client can get pending events in
     'remote_notif_process'.  */
  int (*can_get_pending_events) (struct notif_client *self);

  /* Allocate an event.  */
  struct notif_event *(*alloc_event) (void);

  /* Id of this notif_client.  */
  const enum REMOTE_NOTIF_ID id;
} *notif_client_p;

DECLARE_QUEUE_P (notif_client_p);

/* State on remote async notification.  */

struct remote_notif_state
{
  /* Notification queue.  */

  QUEUE(notif_client_p) *notif_queue;

  /* Asynchronous signal handle registered as event loop source for when
     the remote sent us a notification.  The registered callback
     will do a ACK sequence to pull the rest of the events out of
     the remote side into our event queue.  */

  struct async_event_handler *get_pending_events_token;

/* One pending event for each notification client.  This is where we
   keep it until it is acknowledged.  When there is a notification
   packet, parse it, and create an object of 'struct notif_event' to
   assign to it.  This field is unchanged until GDB starts to ack
   this notification (which is done by
   remote.c:remote_notif_pending_replies).  */

  struct notif_event *pending_event[REMOTE_NOTIF_LAST];
};

void remote_notif_ack (struct notif_client *nc, char *buf);
struct notif_event *remote_notif_parse (struct notif_client *nc,
					char *buf);

void notif_event_xfree (struct notif_event *event);

void handle_notification (struct remote_notif_state *notif_state,
			  char *buf);

void remote_notif_process (struct remote_notif_state *state,
			   struct notif_client *except);
struct remote_notif_state *remote_notif_state_allocate (void);
void remote_notif_state_xfree (struct remote_notif_state *state);

extern struct notif_client notif_client_stop;

extern int notif_debug;

#endif /* REMOTE_NOTIF_H */
