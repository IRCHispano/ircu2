/*
 * IRC-Hispano IRC Daemon, include/destruct_event.h
 *
 * Copyright (C) 1997-2019 IRC-Hispano Development Team <toni@tonigarcia.es>
 * Copyright (C) 2002 Carlo Wood <carlo@alinoe.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/** @file
 * @brief Functions for handling timed channel destruction events.
 */
#ifndef INCLUDED_destruct_event_h
#define INCLUDED_destruct_event_h

#ifndef INCLUDED_config_h
#include "config.h"
#endif
#ifndef INCLUDED_channel_h
#include "channel.h"
#endif

extern void schedule_destruct_event_1m(struct Channel* chptr);
extern void schedule_destruct_event_48h(struct Channel* chptr);
extern void remove_destruct_event(struct Channel* chptr);
extern void exec_expired_destruct_events(struct Event* ev);

#endif /* INCLUDED_destruct_event_h */
