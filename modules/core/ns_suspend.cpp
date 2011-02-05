/* NickServ core functions
 *
 * (C) 2003-2011 Anope Team
 * Contact us at team@anope.org
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of Epona by Lara.
 * Based on the original code of Services by Andy Church.
 */

/*************************************************************************/

#include "module.h"

class CommandNSSuspend : public Command
{
 public:
	CommandNSSuspend() : Command("SUSPEND", 2, 2, "nickserv/suspend")
	{
	}

	CommandReturn Execute(CommandSource &source, const std::vector<Anope::string> &params)
	{
		User *u = source.u;

		const Anope::string &nick = params[0];
		const Anope::string &reason = params[1];

		if (readonly)
		{
			source.Reply(LanguageString::READ_ONLY_MODE);
			return MOD_CONT;
		}

		NickAlias *na = findnick(nick);
		if (!na)
		{
			source.Reply(LanguageString::NICK_X_NOT_REGISTERED, nick.c_str());
			return MOD_CONT;
		}

		if (na->HasFlag(NS_FORBIDDEN))
		{
			source.Reply(LanguageString::NICK_X_FORBIDDEN, na->nick.c_str());
			return MOD_CONT;
		}

		if (Config->NSSecureAdmins && na->nc->IsServicesOper())
		{
			source.Reply(LanguageString::ACCESS_DENIED);
			return MOD_CONT;
		}

		na->nc->SetFlag(NI_SUSPENDED);
		na->nc->SetFlag(NI_SECURE);
		na->nc->UnsetFlag(NI_KILLPROTECT);
		na->nc->UnsetFlag(NI_KILL_QUICK);
		na->nc->UnsetFlag(NI_KILL_IMMED);

		for (std::list<NickAlias *>::iterator it = na->nc->aliases.begin(), it_end = na->nc->aliases.end(); it != it_end; ++it)
		{
			NickAlias *na2 = *it;

			if (na2->nc == na->nc)
			{
				na2->last_quit = reason;

				User *u2 = finduser(na2->nick);
				if (u2)
				{
					u2->Logout();
					u2->Collide(na2);
				}
			}
		}

		if (Config->WallForbid)
			ircdproto->SendGlobops(NickServ, "\2%s\2 used SUSPEND on \2%s\2", u->nick.c_str(), nick.c_str());

		Log(LOG_ADMIN, u, this) << "for " << nick << " (" << (!reason.empty() ? reason : "No reason") << ")";
		source.Reply(_("Nick %s is now suspended."), nick.c_str());

		FOREACH_MOD(I_OnNickSuspended, OnNickSuspend(na));

		return MOD_CONT;
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand)
	{
		source.Reply(_("Syntax: SUSPEND nickname reason\n"
			"SUSPENDs a nickname from being used."));
		return true;
	}

	void OnSyntaxError(CommandSource &source, const Anope::string &subcommand)
	{
		SyntaxError(source, "SUSPEND", _("SUSPEND nickname reason"));
	}

	void OnServHelp(CommandSource &source)
	{
		source.Reply(_("    SUSPEND    Suspend a given nick"));
	}
};

class CommandNSUnSuspend : public Command
{
 public:
	CommandNSUnSuspend() : Command("UNSUSPEND", 1, 1, "nickserv/suspend")
	{
	}

	CommandReturn Execute(CommandSource &source, const std::vector<Anope::string> &params)
	{
		User *u = source.u;
		const Anope::string &nick = params[0];

		if (readonly)
		{
			source.Reply(LanguageString::READ_ONLY_MODE);
			return MOD_CONT;
		}

		NickAlias *na = findnick(nick);
		if (!na)
		{
			source.Reply(LanguageString::NICK_X_NOT_REGISTERED, nick.c_str());
			return MOD_CONT;
		}

		if (na->HasFlag(NS_FORBIDDEN))
		{
			source.Reply(LanguageString::NICK_X_FORBIDDEN, na->nick.c_str());
			return MOD_CONT;
		}

		if (Config->NSSecureAdmins && na->nc->IsServicesOper())
		{
			source.Reply(LanguageString::ACCESS_DENIED);
			return MOD_CONT;
		}

		na->nc->UnsetFlag(NI_SUSPENDED);

		if (Config->WallForbid)
			ircdproto->SendGlobops(NickServ, "\2%s\2 used UNSUSPEND on \2%s\2", u->nick.c_str(), nick.c_str());

		Log(LOG_ADMIN, u, this) << "for " << na->nick;
		source.Reply(_("Nick %s is now released."), nick.c_str());

		FOREACH_MOD(I_OnNickUnsuspended, OnNickUnsuspended(na));

		return MOD_CONT;
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand)
	{
		source.Reply(_("Syntax: UNSUSPEND nickname\n"
			"UNSUSPENDS a nickname from being used."));
		return true;
	}

	void OnSyntaxError(CommandSource &source, const Anope::string &subcommand)
	{
		SyntaxError(source, "UNSUSPEND", _("UNSUSPEND nickname"));
	}

	void OnServHelp(CommandSource &source)
	{
		source.Reply(_("    UNSUSPEND  Unsuspend a given nick"));
	}
};

class NSSuspend : public Module
{
	CommandNSSuspend commandnssuspend;
	CommandNSUnSuspend commandnsunsuspend;

 public:
	NSSuspend(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator)
	{
		this->SetAuthor("Anope");
		this->SetType(CORE);

		this->AddCommand(NickServ, &commandnssuspend);
		this->AddCommand(NickServ, &commandnsunsuspend);
	}
};

MODULE_INIT(NSSuspend)