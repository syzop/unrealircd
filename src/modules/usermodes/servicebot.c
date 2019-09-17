/*
 * Prevents you from being kicked (User mode +q)
 * (C) Copyright 2000-.. Bram Matthys (Syzop) and the UnrealIRCd team
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
 */

#include "unrealircd.h"

#define IsServiceBot(cptr)    (cptr->umodes & UMODE_SERVICEBOT)

#define WHOIS_SERVICE_STRING ":%s 313 %s %s :is a Network Service"

/* Module header */
ModuleHeader MOD_HEADER
  = {
	"usermodes/servicebot",
	"4.2",
	"User Mode +S",
	"UnrealIRCd Team",
	"unrealircd-5",
    };

/* Global variables */
long UMODE_SERVICEBOT = 0L;

/* Forward declarations */
int servicebot_can_kick(Client *sptr, Client *target, Channel *chptr,
                    char *comment, long sptr_flags, long target_flags, char **reject_reason);
int servicebot_mode_deop(Client *sptr, Client *target, Channel *chptr,
                    u_int what, int modechar, long my_access, char **reject_reason);
int servicebot_pre_kill(Client *sptr, Client *target, char *reason);
int servicebot_whois(Client *sptr, Client *acptr);
int servicebot_see_channel_in_whois(Client *sptr, Client *target, Channel *chptr);
                    
MOD_TEST()
{
	return MOD_SUCCESS;
}

MOD_INIT()
{
	UmodeAdd(modinfo->handle, 'S', UMODE_GLOBAL, 1, umode_allow_none, &UMODE_SERVICEBOT);
	
	HookAdd(modinfo->handle, HOOKTYPE_CAN_KICK, 0, servicebot_can_kick);
	HookAdd(modinfo->handle, HOOKTYPE_MODE_DEOP, 0, servicebot_mode_deop);
	HookAdd(modinfo->handle, HOOKTYPE_PRE_KILL, 0, servicebot_pre_kill);
	HookAdd(modinfo->handle, HOOKTYPE_WHOIS, 0, servicebot_whois);
	HookAdd(modinfo->handle, HOOKTYPE_SEE_CHANNEL_IN_WHOIS, 0, servicebot_see_channel_in_whois);
	
	MARK_AS_OFFICIAL_MODULE(modinfo);
	return MOD_SUCCESS;
}

MOD_LOAD()
{
	return MOD_SUCCESS;
}

MOD_UNLOAD()
{
	return MOD_SUCCESS;
}

int servicebot_can_kick(Client *sptr, Client *target, Channel *chptr, char *comment,
                    long sptr_flags, long target_flags, char **reject_reason)
{
	static char errmsg[NICKLEN+256];

	if (MyUser(sptr) && !IsULine(sptr) && IsServiceBot(target))
	{
		char errmsg2[NICKLEN+32];
		snprintf(errmsg2, sizeof(errmsg2), "%s is a Service Bot", target->name);
		
		snprintf(errmsg, sizeof(errmsg), ":%s %d %s %s :%s",
		         me.name, ERR_CANNOTDOCOMMAND, sptr->name, "KICK", errmsg2);

		*reject_reason = errmsg;

		return EX_DENY;
	}

	return EX_ALLOW;
}

int servicebot_mode_deop(Client *sptr, Client *target, Channel *chptr,
                    u_int what, int modechar, long my_access, char **reject_reason)
{
	static char errmsg[NICKLEN+256];
	
	if (IsServiceBot(target) && MyUser(sptr) && !ValidatePermissionsForPath("services:servicebot:deop",sptr,target,chptr,NULL) && (what == MODE_DEL))
	{
		snprintf(errmsg, sizeof(errmsg), ":%s %d %s %c :%s is a Service Bot",
			me.name, ERR_CANNOTCHANGECHANMODE, sptr->name, (char)modechar, target->name);
		
		*reject_reason = errmsg;
		
		return EX_DENY;
	}
	
	return EX_ALLOW;
}

int servicebot_pre_kill(Client *sptr, Client *target, char *reason)
{
	if (IsServiceBot(target) && !(ValidatePermissionsForPath("services:servicebot:kill",sptr,target,NULL,NULL) || IsULine(sptr)))
	{
		sendnumeric(sptr, ERR_KILLDENY, target->name);
		return EX_ALWAYS_DENY;
	}
	return EX_ALLOW;
}

int servicebot_whois(Client *sptr, Client *acptr)
{
	int hideoper = (IsHideOper(acptr) && (sptr != acptr) && !IsOper(sptr)) ? 1 : 0;

	if (IsServiceBot(acptr) && !hideoper)
		sendto_one(sptr, NULL, WHOIS_SERVICE_STRING, me.name, sptr->name, acptr->name);

	return 0;
}

/* This hides the servicebot, even if you are in the same channel, unless oper overriding */
int servicebot_see_channel_in_whois(Client *sptr, Client *target, Channel *chptr)
{
	if (IsServiceBot(target))
		return EX_DENY;
	
	return EX_ALLOW;
}
