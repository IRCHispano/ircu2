/*
 * IRC-Hispano IRC Daemon, include/s_serv.h
 *
 * Copyright (C) 1997-2017 IRC-Hispano Development Team <devel@irc-hispano.es>
 * Copyright (C) 1990 Jarkko Oikarinen
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
/** @file s_serv.h
 * @brief Miscellaneous server support functions.
 */
#ifndef INCLUDED_s_serv_h
#define INCLUDED_s_serv_h

#ifndef INCLUDED_sys_types_h
#include <sys/types.h>
#define INCLUDED_sys_types_h
#endif

struct ConfItem;
struct Client;

extern unsigned int max_connection_count;
extern unsigned int max_client_count;

/*
 * Prototypes
 */
extern int exit_new_server(struct Client* cptr, struct Client* sptr,
                           const char* host, time_t timestamp, const char* fmt, ...);
extern int a_kills_b_too(struct Client *a, struct Client *b);
extern int server_estab(struct Client *cptr, struct ConfItem *aconf);


#endif /* INCLUDED_s_serv_h */
