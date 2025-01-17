// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <string.h>
#include <sys/time.h>
#include <poll.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <urcu/arch.h>
#include "api.h"
#define _LGPL_SOURCE

#ifdef RCU_MEMBARRIER
#include <urcu.h>
#endif
#ifdef RCU_MB
#include <urcu.h>
#endif
#ifdef RCU_QSBR
#include <urcu-qsbr.h>
#endif
#ifdef RCU_BP
#include <urcu-bp.h>
#endif

#include <urcu/uatomic.h>
#include <urcu/rculist.h>
#include "rcutorture.h"
