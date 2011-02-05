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

class CommandNSGetPass : public Command
{
 public:
	CommandNSGetPass() : Command("GETPASS", 1, 1, "nickserv/getpass")
	{
	}

	CommandReturn Execute(CommandSource &source, const std::vector<Anope::string> &params)
	{
		User *u = source.u;
		const Anope::string &nick = params[0];
		Anope::string tmp_pass;
		NickAlias *na;
		NickRequest *nr = NULL;

		if (!(na = findnick(nick)))
		{
			if ((nr = findrequestnick(nick)))
			{
				Log(LOG_ADMIN, u, this) << "for " << nr->nick;
				if (Config->WallGetpass)
					ircdproto->SendGlobops(NickServ, "\2%s\2 used GETPASS on \2%s\2", u->nick.c_str(), nick.c_str());
				source.Reply(_("Passcode for %s is \002%s\002."), nick.c_str(), nr->passcode.c_str());
			}
			else
				source.Reply(LanguageString::NICK_X_NOT_REGISTERED, nick.c_str());
		}
		else if (na->HasFlag(NS_FORBIDDEN))
			source.Reply(LanguageString::NICK_X_FORBIDDEN, na->nick.c_str());
		else if (Config->NSSecureAdmins && na->nc->IsServicesOper())
			source.Reply(LanguageString::ACCESS_DENIED);
		else
		{
			if (enc_decrypt(na->nc->pass, tmp_pass) == 1)
			{
				Log(LOG_ADMIN, u, this) << "for " << nick;
				if (Config->WallGetpass)
					ircdproto->SendGlobops(NickServ, "\2%s\2 used GETPASS on \2%s\2", u->nick.c_str(), nick.c_str());
				source.Reply(_("Password for %s is \002%s\002."), nick.c_str(), tmp_pass.c_str());
			}
			else
				source.Reply(_("GETPASS command unavailable because encryption is in use."));
		}
		return MOD_CONT;
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand)
	{
		source.Reply(_("Syntax: \002GETPASS \037nickname\037\002\n"
				" \n"
				"Returns the password for the given nickname.  \002Note\002 that\n"
				"whenever this command is used, a message including the\n"
				"person who issued the command and the nickname it was used\n"
				"on will be logged and sent out as a WALLOPS/GLOBOPS."));
		return true;
	}

	void OnSyntaxError(CommandSource &source, const Anope::string &subcommand)
	{
		SyntaxError(source, "GETPASS", _("GETPASS \037nickname\037"));
	}

	void OnServHelp(CommandSource &source)
	{
		source.Reply(_("    GETPASS    Retrieve the password for a nickname"));
	}
};

class NSGetPass : public Module
{
	CommandNSGetPass commandnsgetpass;

 public:
	NSGetPass(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator)
	{
		Anope::string tmp_pass = "plain:tmp";
		if (enc_decrypt(tmp_pass, tmp_pass) == -1)
			throw ModuleException("Incompatible with the encryption module being used");

		this->SetAuthor("Anope");
		this->SetType(CORE);

		this->AddCommand(NickServ, &commandnsgetpass);
	}
};

MODULE_INIT(NSGetPass)