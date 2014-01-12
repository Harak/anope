/* OperServ core functions
 *
 * (C) 2003-2014 Anope Team
 * Contact us at team@anope.org
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of Epona by Lara.
 * Based on the original code of Services by Andy Church.
 */

#include "module.h"
#include "modules/os_ignore.h"

class OSIgnoreService : public IgnoreService
{
 public:
	OSIgnoreService(Module *o) : IgnoreService(o) { }

	IgnoreData* AddIgnore(const Anope::string &mask, const Anope::string &creator, const Anope::string &reason, time_t delta = Anope::CurTime) anope_override
	{
		/* Check if we already got an identical entry. */
		IgnoreData *ign = this->Find(mask);
		if (ign != NULL)
		{
			if (!delta)
				ign->time = 0;
			else
				ign->time = Anope::CurTime + delta;
			return ign;
		}
		/* Create new entry.. */
		else
		{
			IgnoreData newign;
			newign.mask = mask;
			newign.creator = creator;
			newign.reason = reason;
			newign.time = delta ? Anope::CurTime + delta : 0;
			this->ignores.push_back(newign);
			return &this->ignores.back();
		}
	}

	bool DelIgnore(const Anope::string &mask) anope_override
	{
		for (std::list<IgnoreData>::iterator it = this->ignores.begin(), it_end = this->ignores.end(); it != it_end; ++it)
		{
			IgnoreData &idn = *it;
			if (idn.mask.equals_ci(mask))
			{
				this->ignores.erase(it);
				return true;
			}
		}

		return false;
	}

	IgnoreData *Find(const Anope::string &mask) anope_override
	{
		User *u = User::Find(mask, true);
		std::list<IgnoreData>::iterator ign = this->ignores.begin(), ign_end = this->ignores.end();
		
		if (u)
		{
			for (; ign != ign_end; ++ign)
			{
				Entry ignore_mask("", ign->mask);
				if (ignore_mask.Matches(u, true))
					break;
			}
		}
		else
		{
			size_t user, host;
			Anope::string tmp;
			/* We didn't get a user.. generate a valid mask. */
			if ((host = mask.find('@')) != Anope::string::npos)
			{
				if ((user = mask.find('!')) != Anope::string::npos)
				{
					/* this should never happen */
					if (user > host)
						return NULL;
					tmp = mask;
				}
				else
					/* We have user@host. Add nick wildcard. */
				tmp = "*!" + mask;
			}
			/* We only got a nick.. */
			else
				tmp = mask + "!*@*";

			for (; ign != ign_end; ++ign)
				if (Anope::Match(tmp, ign->mask, false, true))
					break;
		}

		/* Check whether the entry has timed out */
		if (ign != ign_end)
		{
			IgnoreData &id = *ign;

			if (id.time && !Anope::NoExpire && id.time <= Anope::CurTime)
			{
				Log(LOG_NORMAL, "expire/ignore", Config->GetClient("OperServ")) << "Expiring ignore entry " << id.mask;
				this->ignores.erase(ign);
			}
			else
				return &id;
		}

		return NULL;
	}
};

class CommandOSIgnore : public Command
{
 private:
	Anope::string RealMask(const Anope::string &mask)
	{
		/* If it s an existing user, we ignore the hostmask. */
		User *u = User::Find(mask, true);
		if (u)
			return "*!*@" + u->host;
		
		size_t host = mask.find('@');
		/* Determine whether we get a nick or a mask. */
		if (host != Anope::string::npos)
		{
			size_t user = mask.find('!');
			/* Check whether we have a nick too.. */
			if (user != Anope::string::npos)
			{
				if (user > host)
					/* this should never happen */
					return "";
				else
					return mask;
			}
			else
				/* We have user@host. Add nick wildcard. */
				return "*!" + mask;
		}

		/* We only got a nick.. */
		return mask + "!*@*";
	}

	void DoAdd(CommandSource &source, const std::vector<Anope::string> &params)
	{
		if (!ignore_service)
			return;

		const Anope::string &time = params.size() > 1 ? params[1] : "";
		const Anope::string &nick = params.size() > 2 ? params[2] : "";
		const Anope::string &reason = params.size() > 3 ? params[3] : "";

		if (time.empty() || nick.empty())
		{
			this->OnSyntaxError(source, "ADD");
			return;
		}
		else
		{
			time_t t = Anope::DoTime(time);

			if (t <= -1)
			{
				source.Reply(BAD_EXPIRY_TIME);
				return;
			}

			Anope::string mask = RealMask(nick);
			if (mask.empty())
			{
				source.Reply(BAD_USERHOST_MASK);
				return;
			}

			if (Anope::ReadOnly)
				source.Reply(READ_ONLY_MODE);

			ignore_service->AddIgnore(mask, source.GetNick(), reason, t);
			if (!t)
			{
				source.Reply(_("\002%s\002 will now permanently be ignored."), mask.c_str());
				Log(LOG_ADMIN, source, this) << "to add a permanent ignore for " << mask;
			}
			else
			{
				source.Reply(_("\002%s\002 will now be ignored for \002%s\002."), mask.c_str(), Anope::Duration(t, source.GetAccount()).c_str());
				Log(LOG_ADMIN, source, this) << "to add an ignore on " << mask << " for " << Anope::Duration(t);
			}
		}
	}

	void DoList(CommandSource &source)
	{
		if (!ignore_service)
			return;

		std::list<IgnoreData> &ignores = ignore_service->GetIgnores();

		for (std::list<IgnoreData>::iterator it = ignores.begin(), next_it; it != ignores.end(); it = next_it)
		{
			IgnoreData &id = *it;
			next_it = it;
			++next_it;

			if (id.time && !Anope::NoExpire && id.time <= Anope::CurTime)
			{
				Log(LOG_NORMAL, "expire/ignore", Config->GetClient("OperServ")) << "Expiring ignore entry " << id.mask;
				ignores.erase(it);
			}
		}

		if (ignores.empty())
			source.Reply(_("Ignore list is empty."));
		else
		{
			ListFormatter list(source.GetAccount());
			list.AddColumn(_("Mask")).AddColumn(_("Creator")).AddColumn(_("Reason")).AddColumn(_("Expires"));
			for (std::list<IgnoreData>::const_iterator ign = ignores.begin(), ign_end = ignores.end(); ign != ign_end; ++ign)
			{
				const IgnoreData &ignore = *ign;

				ListFormatter::ListEntry entry;
				entry["Mask"] = ignore.mask;
				entry["Creator"] = ignore.creator;
				entry["Reason"] = ignore.reason;
				entry["Expires"] = Anope::Expires(ignore.time, source.GetAccount());
				list.AddEntry(entry);
			}

			source.Reply(_("Services ignore list:"));

			std::vector<Anope::string> replies;
			list.Process(replies);

			for (unsigned i = 0; i < replies.size(); ++i)
				source.Reply(replies[i]);
		}
	}

	void DoDel(CommandSource &source, const std::vector<Anope::string> &params)
	{
		if (!ignore_service)
			return;

		const Anope::string nick = params.size() > 1 ? params[1] : "";
		if (nick.empty())
		{
			this->OnSyntaxError(source, "DEL");
			return;
		}

		Anope::string mask = RealMask(nick);
		if (mask.empty())
		{
			source.Reply(BAD_USERHOST_MASK);
			return;
		}

		if (ignore_service->DelIgnore(mask))
		{
			if (Anope::ReadOnly)
				source.Reply(READ_ONLY_MODE);

			Log(LOG_ADMIN, source, this) << "to remove an ignore on " << mask;
			source.Reply(_("\002%s\002 will no longer be ignored."), mask.c_str());
		}
		else
			source.Reply(_("\002%s\002 not found on ignore list."), mask.c_str());
	}

	void DoClear(CommandSource &source)
	{
		if (!ignore_service)
			return;

		if (Anope::ReadOnly)
			source.Reply(READ_ONLY_MODE);

		ignore_service->ClearIgnores();
		Log(LOG_ADMIN, source, this) << "to CLEAR the list";
		source.Reply(_("Ignore list has been cleared."));

		return;
	}

 public:
	CommandOSIgnore(Module *creator) : Command(creator, "operserv/ignore", 1, 4)
	{
		this->SetDesc(_("Modify the Services ignore list"));
		this->SetSyntax(_("ADD \037expiry\037 {\037nick\037|\037mask\037} [\037reason\037]"));
		this->SetSyntax(_("DEL {\037nick\037|\037mask\037}"));
		this->SetSyntax("LIST");
		this->SetSyntax("CLEAR");
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) anope_override
	{
		const Anope::string &cmd = params[0];

		if (cmd.equals_ci("ADD"))
			return this->DoAdd(source, params);
		else if (cmd.equals_ci("LIST"))
			return this->DoList(source);
		else if (cmd.equals_ci("DEL"))
			return this->DoDel(source, params);
		else if (cmd.equals_ci("CLEAR"))
			return this->DoClear(source);
		else
			this->OnSyntaxError(source, "");

		return;
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand) anope_override
	{
		this->SendSyntax(source);
		source.Reply(" ");
		source.Reply(_("Allows Services Operators to make Services ignore a nick or mask\n"
				"for a certain time or until the next restart. The default\n"
				"time format is seconds. You can specify it by using units.\n"
				"Valid units are: \037s\037 for seconds, \037m\037 for minutes,\n"
				"\037h\037 for hours and \037d\037 for days.\n"
				"Combinations of these units are not permitted.\n"
				"To make Services permanently ignore the user, type 0 as time.\n"
				"When adding a \037mask\037, it should be in the format nick!user@host,\n"
				"everything else will be considered a nick. Wildcards are permitted.\n"
				" \n"
				"Ignores will not be enforced on IRC Operators."));

		const Anope::string &regexengine = Config->GetBlock("options")->Get<const Anope::string>("regexengine");
		if (!regexengine.empty())
		{
			source.Reply(" ");
			source.Reply(_("Regex matches are also supported using the %s engine.\n"
					"Enclose your pattern in // if this is desired."), regexengine.c_str());
		}

		return true;
	}
};

class OSIgnore : public Module
{
	Serialize::Type ignoredata_type;
	OSIgnoreService osignoreservice;
	CommandOSIgnore commandosignore;

 public:
	OSIgnore(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, VENDOR),
		ignoredata_type("IgnoreData", IgnoreData::Unserialize), osignoreservice(this), commandosignore(this)
	{

	}

	EventReturn OnBotPrivmsg(User *u, BotInfo *bi, Anope::string &message) anope_override
	{
		if (!u->HasMode("OPER") && this->osignoreservice.Find(u->nick))
			return EVENT_STOP;

		return EVENT_CONTINUE;
	}
};

MODULE_INIT(OSIgnore)
