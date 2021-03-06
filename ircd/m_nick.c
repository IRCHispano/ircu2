/*
 * IRC-Hispano IRC Daemon, ircd/m_nick.c
 *
 * Copyright (C) 1997-2019 IRC-Hispano Development Team <toni@tonigarcia.es>
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
/** @file
 * @brief Handlers for NICK command.
 */
#include "config.h"

#include "IPcheck.h"
#include "client.h"
#include "ddb.h"
#include "hash.h"
#include "ircd.h"
#include "ircd_chattr.h"
#include "ircd_features.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "match.h"
#include "msg.h"
#include "numeric.h"
#include "numnicks.h"
#include "s_conf.h"
#include "s_debug.h"
#include "s_misc.h"
#include "s_user.h"
#include "send.h"
#include "sys.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */
#include <stdlib.h>
#include <string.h>

 /*
* 'do_nick_name' ensures that the given parameter (nick) is really a proper
* string for a nickname (note, the 'nick' may be modified in the process...)
*
* RETURNS the length of the final NICKNAME (0, if nickname is invalid)
*
* Nickname characters are in range 'A'..'}', '_', '-', '0'..'9'
*  anything outside the above set will terminate nickname.
* In addition, the first character cannot be '-' or a Digit.
*
* Note:
*  The '~'-character should be allowed, but a change should be global,
*  some confusion would result if only few servers allowed it...
*/
int do_nick_name(char* nick)
{
  char* ch  = nick;
  char* end = ch + NICKLEN;
  assert(0 != ch);

  /* first character in [0..9-] */
  if (*ch == '-' || IsDigit(*ch))
    return 0;
  for ( ; (ch < end) && *ch; ++ch)
    if (!IsNickChar(*ch))
      break;

  *ch = '\0';

  return (ch - nick);
}

/*
 * m_nick - message handler for local clients
 * parv[0] = sender prefix
 * parv[1] = nickname
 */
int m_nick(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct Client* acptr;
  char           nick[NICKLEN + 2];
  char*          arg;
  char*          s;
  int            flags = 0;
  int            random_nick = 0;
#if defined(DDB)
  struct DdbNick *ddbnick;
  char *p;
#endif

  assert(0 != cptr);
  assert(cptr == sptr);

  if (IsServerPort(cptr))
    return exit_client(cptr, cptr, &me, "Use a different port");
  if (IsWebircPort(cptr) && !cli_wline(cptr))
    return exit_client(cptr, cptr, &me, "WebIRC authorization required");
  if (IsProxyPort(cptr) && !find_proxy(cptr))
    return exit_client(cptr, cptr, &me, "Proxy authorization required");

  if (parc < 2) {
    send_reply(sptr, ERR_NONICKNAMEGIVEN);
    return 0;
  }

#if 0
  /*
   * parv[0] will be empty for clients connecting for the first time
   */
  client_name = (*(cli_name(sptr))) ? cli_name(sptr) : "*";

  /*
   * Not change nick several times BEFORE to have completed your entrance
   * in the network.
   */
  if (cli_auth(sptr) && !IsRegistered(sptr))
    return 0;
#endif

#if defined(DDB)
  p = strchr(parv[1], ':');
  if (p) {
    /* nick:password  Support */
    *p++ = '\0';
    parc = 3;
    parv[2] = p;

  } else if ((p = strchr(parv[1], '!'))) {
    /* GHOST nick!password  Support */
    SetGhost(flags);
    if (strlen(p) > 1) {
      *p++ = '\0';
      parc = 3;
      parv[2] = p;
    }

  } else if (parc > 2 && (*parv[2] == '!')) {
    /* GHOST nick !password  Support */
    SetGhost(flags);
    *parv[2]++ = '\0';
  }
#endif /* defined(DDB) */

  /*
   * Special CASE
   * NICK *
   * Random nickname
   */
  if (feature_bool(FEAT_ALLOW_RANDOM_NICKS) && (strlen(parv[1]) == 1) && (*parv[1] == '*'))
  {
    strcpy(parv[1], get_random_nick(sptr));
    random_nick = 1;
  }

  /*
   * Don't let them send make us send back a really long string of
   * garbage
   */
  arg = parv[1];
  if (strlen(arg) > IRCD_MIN(NICKLEN, feature_int(FEAT_NICKLEN)))
    arg[IRCD_MIN(NICKLEN, feature_int(FEAT_NICKLEN))] = '\0';

#if !defined(DDB)
  /* ~ nicks support */
  if ((s = strchr(arg, '~')))
    *s = '\0';
#endif

  strcpy(nick, arg);

  /*
   * If do_nick_name() returns a null name then reject it.
   */
  if (0 == do_nick_name(nick)) {
    send_reply(sptr, ERR_ERRONEUSNICKNAME, arg);
    return 0;
  }

  /*
   * Check if this is a LOCAL user trying to use a reserved (Juped)
   * nick, if so tell him that it's a nick in use...
   */
  if (!random_nick) {
#if defined(DDB)
    /* Jupe by DDB */
    struct Ddb *ddbj;
    for (ddbj = ddb_iterator_first(DDB_JUPEDB); ddbj; ddbj = ddb_iterator_next())
    {
      if (!match(ddb_key(ddbj), nick))
      {
        send_reply(sptr, SND_EXPLICIT | ERR_NICKNAMEINUSE, "%s :Nickname is juped: %s",
           nick, ddb_content(ddbj));
        return 0;                        /* NICK message ignored */
      }

      /* TODO: PCRE-Match Jupe */
    }
#endif /* defined(DDB) */

    /* Jupe by conf */
    if (isNickJuped(nick)) {
      send_reply(sptr, ERR_NICKNAMEINUSE, nick);
      return 0;                        /* NICK message ignored */
    }
  }

  if (!(acptr = FindClient(nick))) {
    /*
     * No collisions, all clear...
     */
    return set_nick_name(cptr, sptr, nick, parc, parv, flags);
  }
  if (IsServer(acptr)) {
    send_reply(sptr, ERR_NICKNAMEINUSE, nick);
    return 0;                        /* NICK message ignored */
  }
  /*
   * If acptr == sptr, then we have a client doing a nick
   * change between *equivalent* nicknames as far as server
   * is concerned (user is changing the case of his/her
   * nickname or somesuch)
   */
  if (acptr == sptr) {
    /*
     * If acptr == sptr, then we have a client doing a nick
     * change between *equivalent* nicknames as far as server
     * is concerned (user is changing the case of his/her
     * nickname or somesuch)
     */
#if defined(DDB)
    if (IsAccount(sptr) || IsNickSuspended(sptr))
#endif
      SetNickEquivalent(flags);

    if (0 != strcmp(cli_name(acptr), nick)) {
      /*
       * Allows change of case in his/her nick
       */
      return set_nick_name(cptr, sptr, nick, parc, parv, flags);
    }
    /*
     * This is just ':old NICK old' type thing.
     * Just forget the whole thing here. There is
     * no point forwarding it to anywhere,
     * especially since servers prior to this
     * version would treat it as nick collision.
     */
    return 0;
  }
  /*
   * Note: From this point forward it can be assumed that
   * acptr != sptr (point to different client structures).
   */
  assert(acptr != sptr);
  /*
   * If the older one is "non-person", the new entry is just
   * allowed to overwrite it. Just silently drop non-person,
   * and proceed with the nick. This should take care of the
   * "dormant nick" way of generating collisions...
   *
   * XXX - hmmm can this happen after one is registered?
   *
   * Yes, client 1 connects to IRC and registers, client 2 connects and
   * sends "NICK foo" but doesn't send anything more.  client 1 now does
   * /nick foo, they should succeed and client 2 gets disconnected with
   * the message below.
   */
  if (IsUnknown(acptr) && MyConnect(acptr)) {
    ServerStats->is_ref++;
    IPcheck_connect_fail(acptr, 0);
    exit_client(cptr, acptr, &me, "Overridden by other sign on");
    return set_nick_name(cptr, sptr, nick, parc, parv, flags);
  }

#if defined(DDB)
  /*
   * GHOST through NICK nick!password Support
   * Verify if nick is registered, if a local user, do not flooding
   * and put a !
   *
   * If puts the correct password, kill to the user with ghost session and
   * propagates the KILL by the network, and put the nick; in opposite case,
   * follows and leave the nick not available message.
   * --zoltan
   */
  if (IsGhost(flags)) {
    ddbnick = ddb_nick_find(nick);

    if (ddbnick && (CurrentTime >= cli_nextnick(sptr))) {

      if (ddbnick->password) {
        char *botname = ddb_get_botname(DDB_NICKSERV);

        if (verify_pass_nick(ddbnick->name, ddbnick->password,
                      (parc >= 3 ? parv[2] : cli_passwd(sptr)))) {
          char nickwho[NICKLEN + 2];

          SetIdentify(flags);

          if (cli_name(sptr)[0] == '\0') {
            /* Ghost during the establishment of the connection
             * do not have old nick information, it is added ! to the nick
             */
            strncpy(nickwho, nick, sizeof(nickwho));
            nickwho[strlen(nickwho)] = '!';

          } else {
            /* Set your older nick */
            strncpy(nickwho, cli_name(sptr), sizeof(nickwho));
          }

          /* Kill user */
          sendto_opmask_butone(0, SNO_SERVKILL,
              "Received KILL message for %C. From %s, Reason: GHOST kill", acptr, nickwho);

          sendcmdto_serv_butone(&me, CMD_KILL, acptr, "%C :GHOST session released by %s",
              acptr, nickwho);

          if (MyConnect(acptr)) {
            sendcmdto_one(acptr, CMD_QUIT, sptr, ":Killed (GHOST session "
                   "released by %s)", nickwho);
            sendcmdto_one(&me, CMD_KILL, acptr, "%C :GHOST session released by %s",
                   acptr, nickwho);
          }
          sendcmdbotto_one(botname, CMD_NOTICE, sptr, "%C :*** %C GHOST session has been "
                   "released.", acptr, sptr);
          exit_client_msg(sptr, acptr, &me, "Killed (GHOST session released by %s)",
                    nickwho);

          return set_nick_name(cptr, sptr, nick, parc, parv, flags);
        } else {
          sendcmdbotto_one(botname, CMD_NOTICE, sptr,
                 "%C :*** Password incorrect.", sptr);
          /* TODO: Contraseña incorrecta */
        }
      }
    }
  }
#endif /* defined(DDB) */

  /*
   * NICK is coming from local client connection. Just
   * send error reply and ignore the command.
   */
  send_reply(sptr, ERR_NICKNAMEINUSE, nick);
  return 0;                        /* NICK message ignored */
}


/*
 * ms_nick - server message handler for nicks
 * parv[0] = sender prefix
 * parv[1] = nickname
 *
 * If from server, source is client:
 *   parv[2] = timestamp
 *   parv[3] = umode (optional)
 *
 * Source is server:
 *   parv[2] = hopcount
 *   parv[3] = timestamp
 *   parv[4] = username
 *   parv[5] = hostname
 *   parv[6] = umode (optional)
 *   parv[parc-3] = IP#                 <- Only Protocol >= 10
 *   parv[parc-2] = YXX, numeric nick   <- Only Protocol >= 10
 *   parv[parc-1] = info
 *   parv[0] = server
 */
int ms_nick(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct Client *acptr;
  char nick[NICKLEN + 2];
  time_t lastnick = 0;
  int differ = 1;
  const char *type;

  assert(0 != cptr);
  assert(0 != sptr);
  assert(IsServer(cptr));

  if ((IsServer(sptr) && parc < 8) || parc < 3)
  {
    sendto_opmask_butone(0, SNO_OLDSNO, "bad NICK param count for %s from %C",
			 parv[1], cptr);
    return need_more_params(sptr, "NICK");
  }

  ircd_strncpy(nick, parv[1], NICKLEN);
  nick[NICKLEN] = '\0';

  if (IsServer(sptr))
  {
    lastnick = atoi(parv[3]);
    if (lastnick > OLDEST_TS && !IsBurstOrBurstAck(sptr))
      cli_serv(sptr)->lag = TStime() - lastnick;
  }
  else
  {
    lastnick = atoi(parv[2]);
    if (lastnick > OLDEST_TS && !IsBurstOrBurstAck(sptr))
      cli_serv(cli_user(sptr)->server)->lag = TStime() - lastnick;
  }
  /*
   * If do_nick_name() returns a null name OR if the server sent a nick
   * name and do_nick_name() changed it in some way (due to rules of nick
   * creation) then reject it. If from a server and we reject it,
   * and KILL it. -avalon 4/4/92
   */
  if (!do_nick_name(nick) || strcmp(nick, parv[1]))
  {
    send_reply(sptr, ERR_ERRONEUSNICKNAME, parv[1]);

    ++ServerStats->is_kill;
    sendto_opmask_butone(0, SNO_OLDSNO, "Bad Nick: %s From: %s %C", parv[1],
			 parv[0], cptr);
    sendcmdto_one(&me, CMD_KILL, cptr, "%s :%s (%s <- %s[%s])",
		  IsServer(sptr) ? parv[parc - 2] : parv[0], cli_name(&me), parv[1],
		  nick, cli_name(cptr));
    if (!IsServer(sptr))
    {
      /*
       * bad nick _change_
       */
      sendcmdto_serv_butone(&me, CMD_KILL, 0, "%s :%s (%s <- %s!%s@%s)",
			    parv[0], cli_name(&me), cli_name(cptr), parv[0],
			    cli_user(sptr) ? cli_username(sptr) : "",
			    cli_user(sptr) ? cli_name(cli_user(sptr)->server) :
			    cli_name(cptr));
    }
    return 0;
  }
  /* Check against nick name collisions. */
  if ((acptr = FindClient(nick)) == NULL)
    /* No collisions, all clear... */
    return set_nick_name(cptr, sptr, nick, parc, parv, 0);

  /*
   * If acptr == sptr, then we have a client doing a nick
   * change between *equivalent* nicknames as far as server
   * is concerned (user is changing the case of his/her
   * nickname or somesuch)
   */
  if (acptr == sptr)
  {
    if (strcmp(cli_name(acptr), nick) != 0)
      /* Allows change of case in his/her nick */
      return set_nick_name(cptr, sptr, nick, parc, parv, 0);
    else
      /* Setting their nick to what it already is? Ignore it. */
      return 0;
  }
  /* now we know we have a real collision. */
  /*
   * Note: From this point forward it can be assumed that
   * acptr != sptr (point to different client structures).
   */
  assert(acptr != sptr);
  /*
   * If the older one is "non-person", the new entry is just
   * allowed to overwrite it. Just silently drop non-person,
   * and proceed with the nick. This should take care of the
   * "dormant nick" way of generating collisions...
   */
  if (IsUnknown(acptr) && MyConnect(acptr))
  {
    ServerStats->is_ref++;
    IPcheck_connect_fail(acptr, 0);
    exit_client(cptr, acptr, &me, "Overridden by other sign on");
    return set_nick_name(cptr, sptr, nick, parc, parv, 0);
  }
  /*
   * Decide, we really have a nick collision and deal with it
   */
  /*
   * NICK was coming from a server connection.
   * This means we have a race condition (two users signing on
   * at the same time), or two net fragments reconnecting with the same nick.
   * The latter can happen because two different users connected
   * or because one and the same user switched server during a net break.
   * If the TimeStamps are equal, we kill both (or only 'new'
   * if it was a ":server NICK new ...").
   * Otherwise we kill the youngest when user@host differ,
   * or the oldest when they are the same.
   * We treat user and ~user as different, because if it wasn't
   * a faked ~user the AUTH wouldn't have added the '~'.
   * --Run
   *
   */
  if (IsServer(sptr))
  {
    struct irc_in_addr ip;
    /*
     * A new NICK being introduced by a neighbouring
     * server (e.g. message type ":server NICK new ..." received)
     *
     * compare IP address and username
     */
    base64toip(parv[parc - 3], &ip);
    differ =  (0 != memcmp(&cli_ip(acptr), &ip, sizeof(cli_ip(acptr)))) ||
              (0 != ircd_strcmp(cli_user(acptr)->username, parv[4]));
    sendto_opmask_butone(0, SNO_OLDSNO, "Nick collision on %C (%C %Tu <- "
			 "%C %Tu (%s user@host))", acptr, cli_from(acptr),
			 cli_lastnick(acptr), cptr, lastnick,
			 differ ? "Different" : "Same");
  }
  else
  {
    /*
     * A NICK change has collided (e.g. message type ":old NICK new").
     *
     * compare IP address and username
     */
    differ =  (0 != memcmp(&cli_ip(acptr), &cli_ip(sptr), sizeof(cli_ip(acptr)))) ||
              (0 != ircd_strcmp(cli_user(acptr)->username, cli_user(sptr)->username));
    sendto_opmask_butone(0, SNO_OLDSNO, "Nick change collision from %C to "
			 "%C (%C %Tu <- %C %Tu)", sptr, acptr, cli_from(acptr),
			 cli_lastnick(acptr), cptr, lastnick);
  }
  type = differ ? "overruled by older nick" : "nick collision from same user@host";
  /*
   * Now remove (kill) the nick on our side if it is the youngest.
   * If no timestamp was received, we ignore the incoming nick
   * (and expect a KILL for our legit nick soon ):
   * When the timestamps are equal we kill both nicks. --Run
   * acptr->from != cptr should *always* be true (?).
   *
   * This exits the client sending the NICK message
   */
  if ((differ && lastnick >= cli_lastnick(acptr)) ||
      (!differ && lastnick <= cli_lastnick(acptr)))
  {
    ServerStats->is_kill++;
    if (!IsServer(sptr))
    {
      /* If this was a nick change and not a nick introduction, we
       * need to ensure that we remove our record of the client, and
       * send a KILL to the whole network.
       */
      assert(!MyConnect(sptr));
      /* Inform the rest of the net... */
      sendcmdto_serv_butone(&me, CMD_KILL, 0, "%C :%s (%s)",
                            sptr, cli_name(&me), type);
      /* Don't go sending off a QUIT message... */
      SetFlag(sptr, FLAG_KILLED);
      /* Remove them locally. */
      exit_client_msg(cptr, sptr, &me,
                      "Killed (%s (%s))",
                      feature_str(FEAT_HIS_SERVERNAME), type);
    }
    else
    {
      /* If the origin is a server, this was a new client, so we only
       * send the KILL in the direction it came from.  We have no
       * client record that we would have to clean up.
       */
      sendcmdto_one(&me, CMD_KILL, cptr, "%s :%s (%s)",
                    parv[parc - 2], cli_name(&me), type);
    }
    /* If the timestamps differ and we just killed sptr, we don't need to kill
     * acptr as well.
     */
    if (lastnick != cli_lastnick(acptr))
      return 0;
  }
  /* Tell acptr why we are killing it. */
  send_reply(acptr, ERR_NICKCOLLISION, nick);

  ServerStats->is_kill++;
  SetFlag(acptr, FLAG_KILLED);
  /*
   * This exits the client we had before getting the NICK message
   */
  sendcmdto_serv_butone(&me, CMD_KILL, NULL, "%C :%s (%s)",
                        acptr, feature_str(FEAT_HIS_SERVERNAME),
                        type);
  exit_client_msg(cptr, acptr, &me, "Killed (%s (%s))",
                  feature_str(FEAT_HIS_SERVERNAME), type);
  if (lastnick == cli_lastnick(acptr))
    return 0;
  if (sptr == NULL)
    return 0;
  return set_nick_name(cptr, sptr, nick, parc, parv, 0);
}
