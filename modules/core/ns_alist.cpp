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

class CommandNSAList : public Command
{
 public:
	CommandNSAList() : Command("ALIST", 0, 2)
	{
	}

	CommandReturn Execute(CommandSource &source, const std::vector<Anope::string> &params)
	{
		/*
		 * List the channels that the given nickname has access on
		 *
		 * /ns ALIST [level]
		 * /ns ALIST [nickname] [level]
		 *
		 * -jester
		 */

		User *u = source.u;
		Anope::string nick;
		NickAlias *na;

		int is_servadmin = u->Account()->IsServicesOper();
		unsigned lev_param = 0;

		if (!is_servadmin)
			/* Non service admins can only see their own levels */
			na = findnick(u->Account()->display);
		else
		{
			/* Services admins can request ALIST on nicks.
			 * The first argument for service admins must
			 * always be a nickname.
			 */
			nick = !params.empty() ? params[0] : "";
			lev_param = 1;

			/* If an argument was passed, use it as the nick to see levels
			 * for, else check levels for the user calling the command */
			na = findnick(!nick.empty() ? nick : u->nick);
		}

		/* If available, get level from arguments */
		Anope::string lev = params.size() > lev_param ? params[lev_param] : "";

		/* if a level was given, make sure it's an int for later */
		int min_level = ACCESS_INVALID;
		if (!lev.empty())
		{
			if (lev.equals_ci("FOUNDER"))
				min_level = ACCESS_FOUNDER;
			else if (lev.equals_ci("SOP"))
				min_level = ACCESS_SOP;
			else if (lev.equals_ci("AOP"))
				min_level = ACCESS_AOP;
			else if (lev.equals_ci("HOP"))
				min_level = ACCESS_HOP;
			else if (lev.equals_ci("VOP"))
				min_level = ACCESS_VOP;
			else
			{
				try
				{
					min_level = convertTo<int>(lev);
				}
				catch (const ConvertException &) { }
			}
		}

		if (!na)
			source.Reply(LanguageString::NICK_X_NOT_REGISTERED, nick.c_str());
		else if (na->HasFlag(NS_FORBIDDEN))
			source.Reply(LanguageString::NICK_X_FORBIDDEN, na->nick.c_str());
		else if (min_level <= ACCESS_INVALID || min_level > ACCESS_FOUNDER)
			source.Reply(LanguageString::CHAN_ACCESS_LEVEL_RANGE, ACCESS_INVALID + 1, ACCESS_FOUNDER - 1);
		else
		{
			int chan_count = 0;
			int match_count = 0;

			if (is_servadmin)
				source.Reply(_("Channels that \002%s\002 has access on:\n"
					"  Num  Channel              Level    Description"), na->nick.c_str());
			else
				source.Reply(_("Channels that you have access on:\n"
						"  Num  Channel              Level    Description"));

			for (registered_channel_map::const_iterator it = RegisteredChannelList.begin(), it_end = RegisteredChannelList.end(); it != it_end; ++it)
			{
				ChannelInfo *ci = it->second;

				ChanAccess *access = ci->GetAccess(na->nc);
				if (access)
				{
					++chan_count;

					if (min_level > access->level)
						continue;

					++match_count;

					if (ci->HasFlag(CI_XOP) || access->level == ACCESS_FOUNDER)
					{
						Anope::string xop = get_xop_level(access->level);

						source.Reply(_("  %3d %c%-20s %-8s %s"), match_count, ci->HasFlag(CI_NO_EXPIRE) ? '!' : ' ', ci->name.c_str(), xop.c_str(), !ci->desc.empty() ? ci->desc.c_str() : "");
					}
					else
						source.Reply(_("  %3d %c%-20s %-8d %s"), match_count, ci->HasFlag(CI_NO_EXPIRE) ? '!' : ' ', ci->name.c_str(), access->level, !ci->desc.empty() ? ci->desc.c_str() : "");
				}
			}

			source.Reply(_("End of list - %d/%d channels shown."), match_count, chan_count);
		}
		return MOD_CONT;
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand)
	{
		User *u = source.u;
		if (u->Account() && u->Account()->IsServicesOper())
			source.Reply(_("Syntax: \002ALIST [\037nickname\037] [\037level\037]\002\n"
					" \n"
					"With no parameters, lists channels you have access on. With\n"
					"one parameter, lists channels that \002nickname\002 has access \n"
					"on. With two parameters lists channels that \002nickname\002 has \n"
					"\002level\002 access or greater on.\n"
					"This use limited to \002Services Operators\002."));
		else
			source.Reply(_("Syntax: \002ALIST [\037level\037]\002\n"
					" \n"
					"Lists all channels you have access on. Optionally, you can specify\n"
					"a level in XOP or ACCESS format. The resulting list will only\n"
					"include channels where you have the given level of access.\n"
					"Examples:\n"
					"    \002ALIST Founder\002\n"
					"        Lists all channels where you have Founder\n"
					"        access.\n"
					"    \002ALIST AOP\002\n"
					"        Lists all channels where you have AOP\n"
					"        access or greater.\n"
					"    \002ALIST 10\002\n"
					"        Lists all channels where you have level 10\n"
					"        access or greater.\n"
					"Channels that have the \037NOEXPIRE\037 option set will be\n"
					"prefixed by an exclamation mark."));

		return true;
	}

	void OnServHelp(CommandSource &source)
	{
		source.Reply(_("    ALIST      List channels you have access on"));
	}
};

class NSAList : public Module
{
	CommandNSAList commandnsalist;

 public:
	NSAList(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator)
	{
		this->SetAuthor("Anope");
		this->SetType(CORE);

		this->AddCommand(NickServ, &commandnsalist);
	}
};

MODULE_INIT(NSAList)