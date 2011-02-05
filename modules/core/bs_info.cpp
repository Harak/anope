/* BotServ core functions
 *
 * (C) 2003-2011 Anope Team
 * Contact us at team@anope.org
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of Epona by Lara.
 * Based on the original code of Services by Andy Church.
 *
 *
 */
/*************************************************************************/

#include "module.h"

class CommandBSInfo : public Command
{
 private:
	void send_bot_channels(CommandSource &source, BotInfo *bi)
	{
		Anope::string buf;
		for (registered_channel_map::const_iterator it = RegisteredChannelList.begin(), it_end = RegisteredChannelList.end(); it != it_end; ++it)
		{
			ChannelInfo *ci = it->second;

			if (ci->bi == bi)
			{
				if (buf.length() + ci->name.length() > 300)
				{
					source.Reply("%s", buf.c_str());
					buf.clear();
				}
				buf += " " + ci->name + " ";
			}
		}

		if (!buf.empty())
			source.Reply("%s", buf.c_str());
		return;
	}
 public:
	CommandBSInfo() : Command("INFO", 1, 1)
	{
		this->SetFlag(CFLAG_STRIP_CHANNEL);
	}

	CommandReturn Execute(CommandSource &source, const std::vector<Anope::string> &params)
	{
		const Anope::string &query = params[0];

		bool need_comma = false;
		char buf[BUFSIZE], *end;

		User *u = source.u;
		BotInfo *bi = findbot(query);
		ChannelInfo *ci;
		if (bi)
		{
			source.Reply(_("Information for bot \002%s\002:"), bi->nick.c_str());
			source.Reply(_("       Mask : %s@%s"), bi->GetIdent().c_str(), bi->host.c_str());
			source.Reply(_("  Real name : %s"), bi->realname.c_str());
			source.Reply(_("    Created : %s"), do_strftime(bi->created).c_str());
			source.Reply(_("    Options : %s"), bi->HasFlag(BI_PRIVATE) ? _("Private") : _("None"));
			source.Reply(_("    Used on : %d channel(s)"), bi->chancount);

			if (u->Account()->HasPriv("botserv/administration"))
				this->send_bot_channels(source, bi);
		}
		else if ((ci = cs_findchan(query)))
		{
			if (!check_access(u, ci, CA_FOUNDER) && !u->Account()->HasPriv("botserv/administration"))
			{
				source.Reply(LanguageString::ACCESS_DENIED);
				return MOD_CONT;
			}

			source.Reply(LanguageString::CHAN_INFO_HEADER, ci->name.c_str());
			if (ci->bi)
				source.Reply(_("           Bot nick : %s"), ci->bi->nick.c_str());
			else
				source.Reply(_("           Bot nick : not assigned yet."));

			if (ci->botflags.HasFlag(BS_KICK_BADWORDS))
			{
				if (ci->ttb[TTB_BADWORDS])
					source.Reply(_("   Bad words kicker : %s (%d kick(s) to ban)"), GetString(u->Account(), LanguageString::ENABLED).c_str(), ci->ttb[TTB_BADWORDS]);
				else
					source.Reply(_("   Bad words kicker : %s"), GetString(u->Account(), LanguageString::ENABLED).c_str());
			}
			else
				source.Reply(_("   Bad words kicker : %s"), GetString(u->Account(), LanguageString::DISABLED).c_str());
			if (ci->botflags.HasFlag(BS_KICK_BOLDS))
			{
				if (ci->ttb[TTB_BOLDS])
					source.Reply(_("       Bolds kicker : %s (%d kick(s) to ban)"), GetString(u->Account(), LanguageString::ENABLED).c_str(), ci->ttb[TTB_BOLDS]);
				else
					source.Reply(_("       Bolds kicker : %s"), GetString(u->Account(), LanguageString::ENABLED).c_str());
			}
			else
				source.Reply(_("       Bolds kicker : %s"), GetString(u->Account(), LanguageString::DISABLED).c_str());
			if (ci->botflags.HasFlag(BS_KICK_CAPS))
			{
				if (ci->ttb[TTB_CAPS])
					source.Reply(_("        Caps kicker : %s (%d kick(s) to ban; minimum %d/%d%%)"), GetString(u->Account(), LanguageString::ENABLED).c_str(), ci->ttb[TTB_CAPS], ci->capsmin, ci->capspercent);
				else
					source.Reply(_("        Caps kicker : %s (minimum %d/%d%%)"), GetString(u->Account(), LanguageString::ENABLED).c_str(), ci->capsmin, ci->capspercent);
			}
			else
				source.Reply(_("        Caps kicker : %s"), GetString(u->Account(), LanguageString::DISABLED).c_str());
			if (ci->botflags.HasFlag(BS_KICK_COLORS))
			{
				if (ci->ttb[TTB_COLORS])
					source.Reply(_("      Colors kicker : %s (%d kick(s) to ban)"), GetString(u->Account(), LanguageString::ENABLED).c_str(), ci->ttb[TTB_COLORS]);
				else
					source.Reply(_("      Colors kicker : %s"), GetString(u->Account(), LanguageString::ENABLED).c_str());
			}
			else
				source.Reply(_("      Colors kicker : %s"), GetString(u->Account(), LanguageString::DISABLED).c_str());
			if (ci->botflags.HasFlag(BS_KICK_FLOOD))
			{
				if (ci->ttb[TTB_FLOOD])
					source.Reply(_("       Flood kicker : %s (%d kick(s) to ban; %d lines in %ds)"), GetString(u->Account(), LanguageString::ENABLED).c_str(), ci->ttb[TTB_FLOOD], ci->floodlines, ci->floodsecs);
				else
					source.Reply(_("       Flood kicker : %s (%d lines in %ds)"), GetString(u->Account(), LanguageString::ENABLED).c_str(), ci->floodlines, ci->floodsecs);
			}
			else
				source.Reply(_("       Flood kicker : %s"), GetString(u->Account(), LanguageString::DISABLED).c_str());
			if (ci->botflags.HasFlag(BS_KICK_REPEAT))
			{
				if (ci->ttb[TTB_REPEAT])
					source.Reply(_("      Repeat kicker : %s (%d kick(s) to ban; %d times)"), GetString(u->Account(), LanguageString::ENABLED).c_str(), ci->ttb[TTB_REPEAT], ci->repeattimes);
				else
					source.Reply(_("      Repeat kicker : %s (%d times)"), GetString(u->Account(), LanguageString::ENABLED).c_str(), ci->repeattimes);
			}
			else
				source.Reply(_("      Repeat kicker : %s"), GetString(u->Account(), LanguageString::DISABLED).c_str());
			if (ci->botflags.HasFlag(BS_KICK_REVERSES))
			{
				if (ci->ttb[TTB_REVERSES])
					source.Reply(_("    Reverses kicker : %s (%d kick(s) to ban)"), GetString(u->Account(), LanguageString::ENABLED).c_str(), ci->ttb[TTB_REVERSES]);
				else
					source.Reply(_("    Reverses kicker : %s"), GetString(u->Account(), LanguageString::ENABLED).c_str());
			}
			else
				source.Reply(_("    Reverses kicker : %s"), GetString(u->Account(), LanguageString::DISABLED).c_str());
			if (ci->botflags.HasFlag(BS_KICK_UNDERLINES))
			{
				if (ci->ttb[TTB_UNDERLINES])
					source.Reply(_("  Underlines kicker : %s (%d kick(s) to ban)"), GetString(u->Account(), LanguageString::ENABLED).c_str(), ci->ttb[TTB_UNDERLINES]);
				else
					source.Reply(_("  Underlines kicker : %s"), GetString(u->Account(), LanguageString::ENABLED).c_str());
			}
			else
				source.Reply(_("  Underlines kicker : %s"), GetString(u->Account(), LanguageString::DISABLED).c_str());
                        if (ci->botflags.HasFlag(BS_KICK_ITALICS))
			{
				if (ci->ttb[TTB_ITALICS])
					source.Reply(_("     Italics kicker : %s (%d kick(s) to ban)"), GetString(u->Account(), LanguageString::ENABLED).c_str(), ci->ttb[TTB_ITALICS]);
				else
					source.Reply(_("     Italics kicker : %s"), GetString(u->Account(), LanguageString::ENABLED).c_str());
			}
			else
				source.Reply(_("     Italics kicker : %s"), GetString(u->Account(), LanguageString::DISABLED).c_str());
			if (ci->botflags.HasFlag(BS_KICK_AMSGS))
			{
				if (ci->ttb[TTB_AMSGS])
					source.Reply(_("     AMSG kicker    : %s (%d kick(s) to ban)"), GetString(u->Account(), LanguageString::ENABLED).c_str(), ci->ttb[TTB_AMSGS]);
				else
					source.Reply(_("     AMSG kicker    : %s"), GetString(u->Account(), LanguageString::ENABLED).c_str());
			}
			else
				source.Reply(_("     AMSG kicker    : %s"), GetString(u->Account(), LanguageString::DISABLED).c_str());

			if (ci->botflags.HasFlag(BS_MSG_PRIVMSG))
				source.Reply(_("      Fantasy reply : %s"), "PRIVMSG");
			else if (ci->botflags.HasFlag(BS_MSG_NOTICE))
				source.Reply(_("      Fantasy reply : %s"), "NOTICE");
			else if (ci->botflags.HasFlag(BS_MSG_NOTICEOPS))
				source.Reply(_("      Fantasy reply : %s"), "NOTICEOPS");
			
			end = buf;
			*end = 0;
			if (ci->botflags.HasFlag(BS_DONTKICKOPS))
			{
				end += snprintf(end, sizeof(buf) - (end - buf), "%s", GetString(u->Account(), _("Ops protection")).c_str());
				need_comma = true;
			}
			if (ci->botflags.HasFlag(BS_DONTKICKVOICES))
			{
				end += snprintf(end, sizeof(buf) - (end - buf), "%s%s", need_comma ? ", " : "", GetString(u->Account(), _("Voices protection")).c_str());
				need_comma = true;
			}
			if (ci->botflags.HasFlag(BS_FANTASY))
			{
				end += snprintf(end, sizeof(buf) - (end - buf), "%s%s", need_comma ? ", " : "", GetString(u->Account(), _("Fantasy")).c_str());
				need_comma = true;
			}
			if (ci->botflags.HasFlag(BS_GREET))
			{
				end += snprintf(end, sizeof(buf) - (end - buf), "%s%s", need_comma ? ", " : "", GetString(u->Account(), _("Greet")).c_str());
				need_comma = true;
			}
			if (ci->botflags.HasFlag(BS_NOBOT))
			{
				end += snprintf(end, sizeof(buf) - (end - buf), "%s%s", need_comma ? ", " : "", GetString(u->Account(), _("No bot")).c_str());
				need_comma = true;
			}
			if (ci->botflags.HasFlag(BS_SYMBIOSIS))
			{
				end += snprintf(end, sizeof(buf) - (end - buf), "%s%s", need_comma ? ", " : "", GetString(u->Account(), _("Symbiosis")).c_str());
				need_comma = true;
			}
			source.Reply(_("            Options : %s"), *buf ? buf : GetString(u->Account(), _("None")).c_str());
		}
		else
			source.Reply(_("\002%s\002 is not a valid bot or registered channel."), query.c_str());
		return MOD_CONT;
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand)
	{
		source.Reply(_("Syntax: \002INFO {\037chan\037 | \037nick\037}\002\n"
				" \n"
				"Allows you to see %s information about a channel or a bot.\n"
				"If the parameter is a channel, then you'll get information\n"
				"such as enabled kickers. If the parameter is a nick,\n"
				"you'll get information about a bot, such as creation\n"
				"time or number of channels it is on."), NickServ->nick.c_str());
		return true;
	}

	void OnSyntaxError(CommandSource &source, const Anope::string &subcommand)
	{
		SyntaxError(source, "INFO", _("INFO {\037chan\037 | \037nick\037}"));
	}

	void OnServHelp(CommandSource &source)
	{
		source.Reply(_("    INFO           Allows you to see BotServ information about a channel or a bot"));
	}
};

class BSInfo : public Module
{
	CommandBSInfo commandbsinfo;

 public:
	BSInfo(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator)
	{
		this->SetAuthor("Anope");
		this->SetType(CORE);

		this->AddCommand(BotServ, &commandbsinfo);
	}
};

MODULE_INIT(BSInfo)