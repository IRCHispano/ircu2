/*
 * IRC-Hispano IRC Daemon, ircd/m_fingerprint.c
 *
 * Copyright (C) 1997-2019 IRC-Hispano Development Team <toni@tonigarcia.es>
 * Copyright (C) 2013 Matthew Beeching (Jobe)
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
#include "config.h"

#include "client.h"
#include "hash.h"
#include "ircd.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "numeric.h"
#include "numnicks.h"
#include "send.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */

int m_fingerprint(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct Client* acptr;

  if (parc > 1) {
    if (!(acptr = FindUser(parv[1])))
      return send_reply(sptr, ERR_NOSUCHNICK, parv[1]);

    if (cli_sslclifp(acptr) && !EmptyString(cli_sslclifp(acptr)))
      send_reply(sptr, RPL_SSLFP, cli_name(acptr), cli_sslclifp(acptr));
    else
      send_reply(sptr, ERR_NOSSLFP, cli_name(acptr));
  } else {
    if (cli_sslclifp(sptr) && !EmptyString(cli_sslclifp(sptr)))
      send_reply(sptr, RPL_SSLFP, cli_name(sptr), cli_sslclifp(sptr));
    else
      send_reply(sptr, ERR_NOSSLFP, cli_name(sptr));
  }

  return 0;
}

