/*
 * IRC-Hispano IRC Daemon, include/capab.h
 *
 * Copyright (C) 1997-2019 IRC-Hispano Development Team <toni@tonigarcia.es>
 * Copyright (C) 2004 Kevin L. Mitchell <klmitch@mit.edu>
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
 * @brief Interface and public definitions for capabilities extension
 */
#ifndef INCLUDED_capab_h
#define INCLUDED_capab_h

#ifndef INCLUDED_client_h
#include "client.h"
#endif

#define CAPFL_HIDDEN	0x0001	/**< Do not advertize this capability */
#define CAPFL_PROHIBIT	0x0002	/**< Client may not set this capability */
#define CAPFL_PROTO	0x0004	/**< Cap must be acknowledged by client */
#define CAPFL_STICKY    0x0008  /**< Cap may not be cleared once set */

#define CAPLIST	\
	_CAP(USERPFX, 0, "undernet.org/userpfx")

/** Client capabilities */
enum Capab {
#define _CAP(cap, flags, name, feat)	CAP_ ## cap
  _CAP(NONE, CAPFL_HIDDEN|CAPFL_PROHIBIT, "none", 0),
  _CAP(NAMESX, 0, "multi-prefix", 0),
  _CAP(UHNAMES, 0, "userhost-in-names", 0),
  _CAP(EXTJOIN, 0, "extended-join", 0),
  _CAP(AWAYNOTIFY, 0, "away-notify", 0),
  _CAP(ACCNOTIFY, 0, "account-notify", 0),
  _CAP(INVITENOTIFY, 0 , "invite-notify", 0),
 // _CAP(SASL, 0, "sasl", 0),
#ifdef USE_SSL
  _CAP(TLS, 0, "tls", 0),
#endif
 /* CAPLIST, */
#undef _CAP
  _CAP_LAST_CAP
};

DECLARE_FLAGSET(CapSet, _CAP_LAST_CAP);

#define CapHas(cs, cap)	FlagHas(cs, cap)
#define CapSet(cs, cap)	FlagSet(cs, cap)
#define CapClr(cs, cap)	FlagClr(cs, cap)

#endif /* INCLUDED_capab_h */
