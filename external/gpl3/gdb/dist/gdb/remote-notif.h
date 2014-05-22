/* Remote notification in GDB protocol

   Copyright (C) 1988-2013 Free Software Foundation, Inc.

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

  /* One pending event.  This is where we keep it until it is
     acknowledged.  When there is a notification packet, parse it,
     and create an object of 'struct notif_event' to assign to
     it.  This field is unchanged until GDB starts to ack this
     notification (which is done by
     remote.c:remote_notif_pending_replies).  */
  struct notif_event *pending_event;
} *notif_client_p;

void remote_notif_ack (struct notif_client *nc, char *buf);
struct notif_event *remote_notif_parse (struct notif_client *nc,
					char *buf);

void handle_notification (char *buf);

void remote_notif_register_async_event_handler (void);
void remote_notif_unregister_async_event_handler (void);

void remote_notif_process (struct notif_client *except);
extern struct notif_client notif_client_stop;

extern int notif_debug;

#endif /* REMOTE_NOTIF_H */
