/*
 * IRC - Internet Relay Chat, ircd/m_squit.c
 * Copyright (C) 1990 Jarkko Oikarinen and
 *                    University of Oulu, Computing Center
 *
 * See file AUTHORS in IRC package for additional names of
 * the programmers.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 */

#if 0
/*
 * No need to include handlers.h here the signatures must match
 * and we don't need to force a rebuild of all the handlers everytime
 * we add a new one to the list. --Bleep
 */
#include "handlers.h"
#endif /* 0 */
#include "client.h"
#include "hash.h"
#include "ircd.h"
#include "ircd_chattr.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "numeric.h"
#include "numnicks.h"
#include "match.h"
#include "s_debug.h"
#include "s_misc.h"
#include "s_user.h"
#include "send.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 *  ms_squit (server)
 *
 *    parv[0] = sender prefix
 *    parv[1] = server name
 *    parv[2] = timestamp
 *    parv[parc-1] = comment
 *
 * No longer supports wildcards from servers. 
 * No longer squits a server that gave us an malformed squit message.
 *    - Isomer 1999-12-18
 * 
 */
int ms_squit(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  const char* server = parv[1];
  struct Client *acptr;
  time_t timestamp;
  char *comment = 0;
  
  if (parc < 2) 
    return need_more_params(sptr, "SQUIT");

  comment = parv[parc-1];
  
  if (BadPtr(parv[parc - 1]))
  	comment=sptr->name;
  	
  acptr = FindNServer(server);

  if (!acptr)
    acptr = FindServer(server);

  if (!acptr) {
    Debug((DEBUG_NOTICE, "Ignoring SQUIT to an unknown server"));
    return 0;
  }
  
  /* If they are squitting me, we reverse it */
  if (IsMe(acptr)) {
  	cptr = acptr;
  	acptr = &me;
  }
  	
  timestamp = atoi(parv[2]);

  /* If atoi(parv[2]) == 0 we must indeed squit !
   * It will be our neighbour.
   */
  if ( timestamp != 0 && timestamp != acptr->serv->timestamp) {
    Debug((DEBUG_NOTICE, "Ignoring SQUIT with the wrong timestamp"));
    return 0;
  }
  
  return exit_client(cptr, acptr, sptr, comment);
}

/*
 *  mo_squit (oper)
 *
 *    parv[0] = sender prefix
 *    parv[1] = server name
 *    parv[2] = comment (optional)
 *
 */
int mo_squit(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  const char* server;
  struct Client *acptr;
  struct Client *acptr2;
  char *comment;
      
  if (parc < 2) 
    return need_more_params(sptr, "SQUIT");

  if (parc < 3 || BadPtr(parv[2]))
    comment = sptr->name;
  else
    comment = parv[2];

  server = parv[1];
  /*
   * The following allows wild cards in SQUIT. Only usefull
   * when the command is issued by an oper.
   */
  for (acptr = GlobalClientList; (acptr = next_client(acptr, server));
      acptr = acptr->next) {
    if (IsServer(acptr) || IsMe(acptr))
      break;
  }
  
  /* Not found? Bugger. */
  if (!acptr || IsMe(acptr))
    return send_reply(sptr, ERR_NOSUCHSERVER, server);

  /*
   * Look for a matching server that is closer,
   * that way we won't accidently squit two close
   * servers like davis.* and davis-r.* when typing
   * /SQUIT davis*
   */
  for (acptr2 = acptr->serv->up; acptr2 != &me;
      acptr2 = acptr2->serv->up)
    if (!match(server, acptr2->name))
      acptr = acptr2;
  
  /* Disallow local opers to squit remote servers */
  if (IsLocOp(sptr) && !MyConnect(acptr))
    return send_reply(sptr, ERR_NOPRIVILEGES);

  return exit_client(cptr, acptr, sptr, comment);
}

