/*
 *   IRC - Internet Relay Chat, src/modules/m_topic.c
 *   (C) 2004 The UnrealIRCd Team
 *
 *   See file AUTHORS in IRC package for additional names of
 *   the programmers.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 1, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "unrealircd.h"

CMD_FUNC(m_topic);

#define MSG_TOPIC 	"TOPIC"	

ModuleHeader MOD_HEADER(m_topic)
  = {
	"m_topic",
	"4.2",
	"command /topic", 
	"3.2-b8-1",
	NULL 
    };

MOD_INIT(m_topic)
{
	CommandAdd(modinfo->handle, MSG_TOPIC, m_topic, 4, M_USER|M_SERVER);
	MARK_AS_OFFICIAL_MODULE(modinfo);
	return MOD_SUCCESS;
}

MOD_LOAD(m_topic)
{
	return MOD_SUCCESS;
}

MOD_UNLOAD(m_topic)
{
	return MOD_SUCCESS;
}

void topicoverride(aClient *sptr, aChannel *chptr, char *topic)
{
	sendto_snomask(SNO_EYES,
	    "*** OperOverride -- %s (%s@%s) TOPIC %s \'%s\'",
	    sptr->name, sptr->user->username, sptr->user->realhost,
	    chptr->chname, topic);
						
	/* Logging implementation added by XeRXeS */
	ircd_log(LOG_OVERRIDE, "OVERRIDE: %s (%s@%s) TOPIC %s \'%s\'",
		sptr->name, sptr->user->username, sptr->user->realhost,
		chptr->chname, topic);
}

/*
** m_topic
**	parv[1] = topic text
**
**	For servers using TS: (Lefler)
**	parv[1] = channel name
**	parv[2] = topic nickname
**	parv[3] = topic time
**	parv[4] = topic text
*/
CMD_FUNC(m_topic)
{
aChannel *chptr = NullChn;
char *topic = NULL, *name, *tnick = sptr->name;
TS   ttime = 0;
int i = 0;
Hook *h;
int ismember; /* cache: IsMember() */
long flags = 0; /* cache: membership flags */

	if (parc < 2)
	{
		sendto_one(sptr, err_str(ERR_NEEDMOREPARAMS),
		    me.name, sptr->name, "TOPIC");
		return 0;
	}
	name = parv[1];

	if (name && IsChannelName(name))
	{
		chptr = find_channel(parv[1], NullChn);
		if (!chptr)
		{
			if (!MyClient(sptr) && !IsULine(sptr))
			{
				sendto_snomask
				    (SNO_JUNK,"Remote TOPIC for unknown channel %s (%s)",
				    parv[1], backupbuf);
			}
			sendto_one(sptr, rpl_str(ERR_NOSUCHCHANNEL),
			    me.name, sptr->name, name);
			return 0;
		}
		
		ismember = IsMember(sptr, chptr); /* CACHE */
		if (ismember)
			flags = get_access(sptr, chptr); /* CACHE */
		
		if (parc > 2 || SecretChannel(chptr))
		{
			if (!ismember && !IsServer(sptr)
			    && !ValidatePermissionsForPath("channel:see:list:secret",sptr,NULL,chptr,NULL) && !IsULine(sptr))
			{
				sendto_one(sptr, err_str(ERR_NOTONCHANNEL),
				    me.name, sptr->name, name);
				return 0;
			}
			if (parc > 2)
				topic = parv[2];
		}
		if (parc > 4)
		{
			tnick = parv[2];
			ttime = atol(parv[3]);
			topic = parv[4];

		}

		if (!topic)	/* only asking  for topic  */
		{
			if (IsServer(sptr))
				return 0; /* Servers must maintain state, not ask */

			for (h = Hooks[HOOKTYPE_VIEW_TOPIC_OUTSIDE_CHANNEL]; h; h = h->next)
			{
				i = (*(h->func.intfunc))(sptr,chptr);
				if (i != HOOK_CONTINUE)
					break;
			}

			/* If you're not a member, and you can't view outside channel, deny */
			if ((!ismember && i == HOOK_DENY) || (is_banned(sptr,chptr,BANCHK_JOIN) && !ValidatePermissionsForPath("channel:see:topic",sptr,NULL,chptr,NULL)))
			{
				sendto_one(sptr, err_str(ERR_NOTONCHANNEL), me.name, sptr->name, name);
				return 0;
			}

			if (!chptr->topic)
				sendto_one(sptr, rpl_str(RPL_NOTOPIC),
				    me.name, sptr->name, chptr->chname);
			else
			{
				sendto_one(sptr, rpl_str(RPL_TOPIC),
				    me.name, sptr->name,
				    chptr->chname, chptr->topic);
				sendto_one(sptr,
				    rpl_str(RPL_TOPICWHOTIME), me.name,
				    sptr->name, chptr->chname,
				    chptr->topic_nick, chptr->topic_time);
			}
		}
		else if (ttime && topic && (IsServer(sptr)
		    || IsULine(sptr)))
		{
			if (!chptr->topic_time || ttime > chptr->topic_time || IsULine(sptr))
			/* The IsUline is to allow services to use an old TS. Apparently
			 * some services do this in their topic enforcement -- codemastr 
			 */
			{
				/* Set the topic */
				safestrldup(chptr->topic, topic, iConf.topic_length+1);
				safestrldup(chptr->topic_nick, tnick, NICKLEN+USERLEN+HOSTLEN+5);
				chptr->topic_time = ttime;

				RunHook4(HOOKTYPE_TOPIC, cptr, sptr, chptr, topic);
				sendto_server(cptr, PROTO_SID, 0, ":%s TOPIC %s %s %lu :%s",
				    ID(sptr), chptr->chname, chptr->topic_nick,
				    chptr->topic_time, chptr->topic);
				sendto_server(cptr, 0, PROTO_SID, ":%s TOPIC %s %s %lu :%s",
				    sptr->name, chptr->chname, chptr->topic_nick,
				    chptr->topic_time, chptr->topic);
				sendto_channel_butserv(chptr, sptr,
				    ":%s TOPIC %s :%s", sptr->name,
				    chptr->chname, chptr->topic);
			}
		}
		else if (((chptr->mode.mode & MODE_TOPICLIMIT) == 0 ||
		    (is_chan_op(sptr, chptr)) || ValidatePermissionsForPath("channel:override:topic",sptr,NULL,chptr,NULL) 
		    || is_halfop(sptr, chptr)) && topic)
		{
			/* setting a topic */
			if (chptr->mode.mode & MODE_TOPICLIMIT)
			{
				if (!is_halfop(sptr, chptr) && !IsULine(sptr) && !
					is_chan_op(sptr, chptr))
				{
#ifndef NO_OPEROVERRIDE
					if (!ValidatePermissionsForPath("channel:override:topic",sptr,NULL,chptr,NULL))
					{
#endif
					sendto_one(sptr, err_str(ERR_CHANOPRIVSNEEDED),
					    me.name, sptr->name, chptr->chname);
					return 0;
#ifndef NO_OPEROVERRIDE
					}
					else
						topicoverride(sptr, chptr, topic);
#endif
				}
			} else
			if (MyClient(sptr) && !is_chan_op(sptr, chptr) && !is_halfop(sptr, chptr) && is_banned(sptr, chptr, BANCHK_MSG))
			{
				char buf[512];
				
				if (ValidatePermissionsForPath("channel:override:topic",sptr,NULL,chptr,NULL))
				{
					topicoverride(sptr, chptr, topic);
				} else {
					ircsnprintf(buf, sizeof(buf), "You cannot change the topic on %s while being banned", chptr->chname);
					sendto_one(sptr, err_str(ERR_CANNOTDOCOMMAND), me.name, sptr->name, "TOPIC",  buf);
					return -1;
				}
			} else
			if (MyClient(sptr) && ((flags&CHFL_OVERLAP) == 0) && (chptr->mode.mode & MODE_MODERATED))
			{
				char buf[512];
				
				if (ValidatePermissionsForPath("channel:override:topic",sptr,NULL,chptr,NULL))
				{
					topicoverride(sptr, chptr, topic);
				} else {
					/* With +m and -t, only voice and higher may change the topic */
					ircsnprintf(buf, sizeof(buf), "Voice (+v) or higher is required in order to change the topic on %s (channel is +m)", chptr->chname);
					sendto_one(sptr, err_str(ERR_CANNOTDOCOMMAND), me.name, sptr->name, "TOPIC",  buf);
					return -1;
				}
			}
			
			/* ready to set... */
			if (MyClient(sptr))
			{
				Hook *tmphook;
				int n;
				
				if ((n = dospamfilter(sptr, topic, SPAMF_TOPIC, chptr->chname, 0, NULL)) < 0)
					return n;

				for (tmphook = Hooks[HOOKTYPE_PRE_LOCAL_TOPIC]; tmphook; tmphook = tmphook->next) {
					topic = (*(tmphook->func.pcharfunc))(sptr, chptr, topic);
					if (!topic)
						return 0;
				}
				RunHook4(HOOKTYPE_LOCAL_TOPIC, cptr, sptr, chptr, topic);
			}

			/* At this point 'tnick' is set to sptr->name.
			 * If set::topic-setter nick-user-host; is set
			 * then we update it here to nick!user@host.
			 */
			if (IsPerson(sptr) && (iConf.topic_setter = SETTER_NICK_USER_HOST))
				tnick = make_nick_user_host(sptr->name, sptr->user->username, GetHost(sptr));

			/* Set the topic */
			safestrldup(chptr->topic, topic, iConf.topic_length+1);
			safestrldup(chptr->topic_nick, tnick, NICKLEN+USERLEN+HOSTLEN+5);

			RunHook4(HOOKTYPE_TOPIC, cptr, sptr, chptr, topic);
			if (ttime && IsServer(cptr))
				chptr->topic_time = ttime;
			else
				chptr->topic_time = TStime();
			sendto_server(cptr, 0, 0, ":%s TOPIC %s %s %lu :%s",
			    sptr->name, chptr->chname, chptr->topic_nick,
			    chptr->topic_time, chptr->topic);
			sendto_channel_butserv(chptr, sptr,
			    ":%s TOPIC %s :%s", sptr->name, chptr->chname,
			    chptr->topic);
		}
		else
			sendto_one(sptr, err_str(ERR_CHANOPRIVSNEEDED),
			    me.name, sptr->name, chptr->chname);
	}
	return 0;
}
