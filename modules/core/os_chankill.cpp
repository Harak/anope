/* OperServ core functions
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

class CommandOSChanKill : public Command
{
 public:
	CommandOSChanKill() : Command("CHANKILL", 2, 3, "operserv/chankill")
	{
	}

	CommandReturn Execute(CommandSource &source, const std::vector<Anope::string> &params)
	{
		User *u = source.u;
		Anope::string expiry, channel;
		time_t expires;
		unsigned last_param = 1;
		Channel *c;

		channel = params[0];
		if (!channel.empty() && channel[0] == '+')
		{
			expiry = channel;
			channel = params[1];
			last_param = 2;
		}

		expires = !expiry.empty() ? dotime(expiry) : Config->ChankillExpiry;
		if (!expiry.empty() && isdigit(expiry[expiry.length() - 1]))
			expires *= 86400;
		if (expires && expires < 60)
		{
			source.Reply(LanguageString::BAD_EXPIRY_TIME);
			return MOD_CONT;
		}
		else if (expires > 0)
			expires += Anope::CurTime;

		if (params.size() <= last_param)
		{
			this->OnSyntaxError(source, "");
			return MOD_CONT;
		}

		Anope::string reason = params[last_param];
		if (params.size() > last_param + 1)
			reason += params[last_param + 1];
		if (!reason.empty())
		{
			Anope::string realreason;
			if (Config->AddAkiller)
				realreason = "[" + u->nick + "] " + reason;
			else
				realreason = reason;

			if ((c = findchan(channel)))
			{
				for (CUserList::iterator it = c->users.begin(), it_end = c->users.end(); it != it_end; )
				{
					UserContainer *uc = *it++;

					if (uc->user->HasMode(UMODE_OPER))
						continue;

					SGLine->Add(OperServ, u, "*@" + uc->user->host, expires, realreason);
					SGLine->Check(uc->user);
				}
				if (Config->WallOSAkill)
					ircdproto->SendGlobops(OperServ, "%s used CHANKILL on %s (%s)", u->nick.c_str(), channel.c_str(), realreason.c_str());
			}
			else
				source.Reply(LanguageString::CHAN_X_NOT_IN_USE, channel.c_str());
		}
		return MOD_CONT;
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand)
	{
		source.Reply(_("Syntax: \002CHANKILL [+\037expiry\037] \037channel\037 \037reason\037\002\n"
				"Puts an AKILL for every nick on the specified channel. It\n"
				"uses the entire and complete real ident@host for every nick,\n"
				"then enforces the AKILL."));
		return true;
	}

	void OnSyntaxError(CommandSource &source, const Anope::string &subcommand)
	{
		SyntaxError(source, "CHANKILL", _("CHANKILL [+\037expiry\037] {\037#channel\037} [\037reason\037]"));
	}

	void OnServHelp(CommandSource &source)
	{
		source.Reply(_("    CHANKILL    AKILL all users on a specific channel"));
	}
};

class OSChanKill : public Module
{
	CommandOSChanKill commandoschankill;

 public:
	OSChanKill(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator)
	{
		this->SetAuthor("Anope");
		this->SetType(CORE);

		this->AddCommand(OperServ, &commandoschankill);
	}
};

MODULE_INIT(OSChanKill)