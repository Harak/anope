/* Inspircd 1.2+ generic TS6 functions
 *
 * (C) 2003-2011 Anope Team
 * Contact us at team@anope.org
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of Epona by Lara.
 * Based on the original code of Services by Andy Church.
 */

class ChannelModeFlood : public ChannelModeParam
{
 public:
	ChannelModeFlood(char modeChar, bool minusNoArg) : ChannelModeParam(CMODE_FLOOD, modeChar, minusNoArg) { }

	bool IsValid(const Anope::string &value) const
	{
		try
		{
			Anope::string rest;
			if (!value.empty() && value[0] != ':' && convertTo<int>(value[0] == '*' ? value.substr(1) : value, rest, false) > 0 && rest[0] == ':' && rest.length() > 1 && convertTo<int>(rest.substr(1), rest, false) > 0 && rest.empty())
				return true;
		}
		catch (const ConvertException &) { }

		return false;
	}
};

class InspIRCdTS6Proto : public IRCDProto
{
 private:
	void SendChgIdentInternal(const Anope::string &nick, const Anope::string &vIdent)
	{
		User *u = finduser(Config->HostServ);
		if (!has_chgidentmod)
			Log() << "CHGIDENT not loaded!";
		else
			send_cmd(u ? u->GetUID() : Config->Numeric, "CHGIDENT %s %s", nick.c_str(), vIdent.c_str());
	}

	void SendChgHostInternal(const Anope::string &nick, const Anope::string &vhost)
	{
		User *u = finduser(Config->HostServ);
		if (!has_chghostmod)
			Log() << "CHGHOST not loaded!";
		else
			send_cmd(u ? u->GetUID() : Config->Numeric, "CHGHOST %s %s", nick.c_str(), vhost.c_str());
	}

 public:

	void SendAkillDel(const XLine *x)
	{
		User *u = finduser(Config->OperServ);
		send_cmd(u ? u->GetUID() : Config->Numeric, "GLINE %s", x->Mask.c_str());
	}

	void SendTopic(BotInfo *whosets, Channel *c)
	{
		send_cmd(whosets->GetUID(), "FTOPIC %s %lu %s :%s", c->name.c_str(), static_cast<unsigned long>(c->topic_time + 1), c->topic_setter.c_str(), c->topic.c_str());
	}

	void SendVhostDel(User *u)
	{
		if (u->HasMode(UMODE_CLOAK))
			this->SendChgHostInternal(u->nick, u->chost);
		else
			this->SendChgHostInternal(u->nick, u->host);

		if (has_chgidentmod && u->GetIdent() != u->GetVIdent())
			this->SendChgIdentInternal(u->nick, u->GetIdent());
	}

	void SendAkill(User *, const XLine *x)
	{
		// Calculate the time left before this would expire, capping it at 2 days
		time_t timeleft = x->Expires - Anope::CurTime;
		if (timeleft > 172800 || !x->Expires)
			timeleft = 172800;
		User *u = finduser(Config->OperServ);
		send_cmd(u ? u->GetUID() : Config->Numeric, "ADDLINE G %s@%s %s %ld %ld :%s", x->GetUser().c_str(), x->GetHost().c_str(), x->By.c_str(), static_cast<long>(Anope::CurTime), static_cast<long>(timeleft), x->Reason.c_str());
	}

	void SendSVSKillInternal(const BotInfo *source, const User *user, const Anope::string &buf)
	{
		send_cmd(source ? source->GetUID() : Config->Numeric, "KILL %s :%s", user->GetUID().c_str(), buf.c_str());
	}

	void SendNumericInternal(const Anope::string &source, int numeric, const Anope::string &dest, const Anope::string &buf)
	{
		send_cmd(Config->Numeric, "PUSH %s ::%s %03d %s %s", dest.c_str(), source.c_str(), numeric, dest.c_str(), buf.c_str());
	}

	void SendModeInternal(const BotInfo *source, const Channel *dest, const Anope::string &buf)
	{
		send_cmd(source ? source->GetUID() : Config->Numeric, "FMODE %s %u %s", dest->name.c_str(), static_cast<unsigned>(dest->creation_time), buf.c_str());
	}

	void SendModeInternal(const BotInfo *bi, const User *u, const Anope::string &buf)
	{
		send_cmd(bi ? bi->GetUID() : Config->Numeric, "MODE %s %s", u->GetUID().c_str(), buf.c_str());
	}

	void SendClientIntroduction(const User *u)
	{
		Anope::string modes = "+" + u->GetModes();
		send_cmd(Config->Numeric, "UID %s %ld %s %s %s %s 0.0.0.0 %ld %s :%s", u->GetUID().c_str(), static_cast<long>(u->timestamp), u->nick.c_str(), u->host.c_str(), u->host.c_str(), u->GetIdent().c_str(), static_cast<long>(u->my_signon), modes.c_str(), u->realname.c_str());
	}

	void SendKickInternal(const BotInfo *source, const Channel *chan, const User *user, const Anope::string &buf)
	{
		if (!buf.empty())
			send_cmd(source->GetUID(), "KICK %s %s :%s", chan->name.c_str(), user->GetUID().c_str(), buf.c_str());
		else
			send_cmd(source->GetUID(), "KICK %s %s :%s", chan->name.c_str(), user->GetUID().c_str(), user->nick.c_str());
	}

	/* SERVER services-dev.chatspike.net password 0 :Description here */
	void SendServer(const Server *server)
	{
		send_cmd("", "SERVER %s %s %d %s :%s", server->GetName().c_str(), Config->Uplinks[CurrentUplink]->password.c_str(), server->GetHops(), server->GetSID().c_str(), server->GetDescription().c_str());
	}

	/* JOIN */
	void SendJoin(User *user, Channel *c, ChannelStatus *status)
	{
		send_cmd(Config->Numeric, "FJOIN %s %ld +%s :,%s", c->name.c_str(), static_cast<long>(c->creation_time), c->GetModes(true, true).c_str(), user->GetUID().c_str());
		/* Note that we can send this with the FJOIN but choose not to
		 * because the mode stacker will handle this and probably will
		 * merge these modes with +nrt and other mlocked modes
		 */
		if (status)
		{
			ChannelStatus cs = *status;
			status->ClearFlags();

			BotInfo *setter = findbot(user->nick);
			for (unsigned i = 0; i < ModeManager::ChannelModes.size(); ++i)
				if (cs.HasFlag(ModeManager::ChannelModes[i]->Name))
					c->SetMode(setter, ModeManager::ChannelModes[i], user->nick, false);
		}
	}

	/* UNSQLINE */
	void SendSQLineDel(const XLine *x)
	{
		send_cmd(Config->Numeric, "DELLINE Q %s", x->Mask.c_str());
	}

	/* SQLINE */
	void SendSQLine(User *, const XLine *x)
	{
		send_cmd(Config->Numeric, "ADDLINE Q %s %s %ld 0 :%s", x->Mask.c_str(), Config->OperServ.c_str(), static_cast<long>(Anope::CurTime), x->Reason.c_str());
	}

	/* SQUIT */
	void SendSquit(const Anope::string &servname, const Anope::string &message)
	{
		send_cmd(Config->Numeric, "SQUIT %s :%s", servname.c_str(), message.c_str());
	}

	/* Functions that use serval cmd functions */

	void SendVhost(User *u, const Anope::string &vIdent, const Anope::string &vhost)
	{
		if (!vIdent.empty())
			this->SendChgIdentInternal(u->nick, vIdent);
		if (!vhost.empty())
			this->SendChgHostInternal(u->nick, vhost);
	}

	void SendConnect()
	{
		SendServer(Me);
		send_cmd(Config->Numeric, "BURST");
		Module *enc = ModuleManager::FindFirstOf(ENCRYPTION);
		send_cmd(Config->Numeric, "VERSION :Anope-%s %s :%s - (%s) -- %s", Anope::Version().c_str(), Config->ServerName.c_str(), ircd->name, enc ? enc->name.c_str() : "unknown", Anope::VersionBuildString().c_str());
	}

	/* SVSHOLD - set */
	void SendSVSHold(const Anope::string &nick)
	{
		BotInfo *bi = findbot(Config->NickServ);
		if (bi)
			send_cmd(bi->GetUID(), "SVSHOLD %s %u :Being held for registered user", nick.c_str(), static_cast<unsigned>(Config->NSReleaseTimeout));
	}

	/* SVSHOLD - release */
	void SendSVSHoldDel(const Anope::string &nick)
	{
		BotInfo *bi = findbot(Config->NickServ);
		if (bi)
			send_cmd(bi->GetUID(), "SVSHOLD %s", nick.c_str());
	}

	/* UNSZLINE */
	void SendSZLineDel(const XLine *x)
	{
		send_cmd(Config->Numeric, "DELLINE Z %s", x->GetHost().c_str());
	}

	/* SZLINE */
	void SendSZLine(User *, const XLine *x)
	{
		send_cmd(Config->Numeric, "ADDLINE Z %s %s %ld 0 :%s", x->GetHost().c_str(), x->By.c_str(), static_cast<long>(Anope::CurTime), x->Reason.c_str());
	}

	void SendSVSJoin(const Anope::string &source, const Anope::string &nick, const Anope::string &chan, const Anope::string &)
	{
		User *u = finduser(nick);
		BotInfo *bi = findbot(source);
		send_cmd(bi->GetUID(), "SVSJOIN %s %s", u->GetUID().c_str(), chan.c_str());
	}

	void SendSWhois(const Anope::string &, const Anope::string &who, const Anope::string &mask)
	{
		User *u = finduser(who);

		send_cmd(Config->Numeric, "METADATA %s swhois :%s", u->GetUID().c_str(), mask.c_str());
	}

	void SendBOB()
	{
		send_cmd(Config->Numeric, "BURST %ld", static_cast<long>(Anope::CurTime));
	}

	void SendEOB()
	{
		send_cmd(Config->Numeric, "ENDBURST");
	}

	void SendGlobopsInternal(BotInfo *source, const Anope::string &buf)
	{
		if (has_globopsmod)
			send_cmd(source ? source->GetUID() : Config->Numeric, "SNONOTICE g :%s", buf.c_str());
		else
			send_cmd(source ? source->GetUID() : Config->Numeric, "SNONOTICE A :%s", buf.c_str());
	}

	void SendAccountLogin(const User *u, const NickCore *account)
	{
		send_cmd(Config->Numeric, "METADATA %s accountname :%s", u->GetUID().c_str(), account->display.c_str());
	}

	void SendAccountLogout(const User *u, const NickCore *account)
	{
		send_cmd(Config->Numeric, "METADATA %s accountname :", u->GetUID().c_str());
	}

	void SendChannel(Channel *c)
	{
		send_cmd(Config->Numeric, "FJOIN %s %ld +%s :", c->name.c_str(), static_cast<long>(c->creation_time), c->GetModes(true, true).c_str());
	}

	bool IsNickValid(const Anope::string &nick)
	{
		/* InspIRCd, like TS6, uses UIDs on collision, so... */
		if (isdigit(nick[0]))
			return false;

		return true;
	}
};

class InspircdIRCdMessage : public IRCdMessage
{
 public:
	bool OnMode(const Anope::string &source, const std::vector<Anope::string> &params)
	{
		if (params[0][0] == '#' || params[0][0] == '&')
			do_cmode(source, params[0], params[2], params[1]);
		else
		{
			/* InspIRCd lets opers change another
			   users modes, we have to kludge this
			   as it slightly breaks RFC1459
			 */
			User *u = finduser(source);
			// This can happen with server-origin modes.
			if (!u)
				u = finduser(params[0]);
			// if it's still null, drop it like fire.
			// most likely situation was that server introduced a nick which we subsequently akilled
			if (!u)
				return true;

			do_umode(u->nick, params[1]);
		}

		return true;
	}

	virtual bool OnUID(const Anope::string &source, const std::vector<Anope::string> &params) = 0;

	bool OnNick(const Anope::string &source, const std::vector<Anope::string> &params)
	{
		do_nick(source, params[0], "", "", "", "", 0, "", "", "", "");
		return true;
	}

	bool OnPrivmsg(const Anope::string &source, const std::vector<Anope::string> &params)
	{
		/* Ignore privmsgs from the server, which can happen. */
		if (Server::Find(source) != NULL)
			return true;

		return IRCdMessage::OnPrivmsg(source, params);
	}

	/*
	 * [Nov 04 00:08:46.308435 2009] debug: Received: SERVER irc.inspircd.com pass 0 964 :Testnet Central!
	 * 0: name
	 * 1: pass
	 * 2: hops
	 * 3: numeric
	 * 4: desc
	 */
	bool OnServer(const Anope::string &source, const std::vector<Anope::string> &params)
	{
		do_server(source, params[0], Anope::string(params[2]).is_pos_number_only() ? convertTo<unsigned>(params[2]) : 0, params[4], params[3]);
		return true;
	}

	bool OnTopic(const Anope::string &source, const std::vector<Anope::string> &params)
	{
		Channel *c = findchan(params[0]);

		if (!c)
		{
			Log() << "TOPIC " << params[1] << " for nonexistent channel " << params[0];
			return true;
		}

		c->ChangeTopicInternal(source, (params.size() > 1 ? params[1] : ""), Anope::CurTime);

		return true;
	}

	virtual bool OnCapab(const Anope::string &, const std::vector<Anope::string> &) = 0;

	bool OnSJoin(const Anope::string &source, const std::vector<Anope::string> &params)
	{
		Channel *c = findchan(params[0]);
		time_t ts = Anope::string(params[1]).is_pos_number_only() ? convertTo<time_t>(params[1]) : 0;
		bool keep_their_modes = true;

		if (!c)
		{
			c = new Channel(params[0], ts);
			c->SetFlag(CH_SYNCING);
		}
		/* Our creation time is newer than what the server gave us */
		else if (c->creation_time > ts)
		{
			c->creation_time = ts;
			c->Reset();

			/* Reset mlock */
			check_modes(c);
		}
		/* Their TS is newer than ours, our modes > theirs, unset their modes if need be */
		else if (ts > c->creation_time)
			keep_their_modes = false;

		/* If we need to keep their modes, and this FJOIN string contains modes */
		if (keep_their_modes && params.size() >= 3)
		{
			Anope::string modes;
			for (unsigned i = 2; i < params.size() - 1; ++i)
				modes += " " + params[i];
			if (!modes.empty())
				modes.erase(modes.begin());
			/* Set the modes internally */
			c->SetModesInternal(NULL, modes);
		}

		spacesepstream sep(params[params.size() - 1]);
		Anope::string buf;
		while (sep.GetToken(buf))
		{
			std::list<ChannelMode *> Status;

			/* Loop through prefixes and find modes for them */
			while (buf[0] != ',')
			{
				ChannelMode *cm = ModeManager::FindChannelModeByChar(buf[0]);
				if (!cm)
				{
					Log() << "Received unknown mode prefix " << buf[0] << " in FJOIN string";
					buf.erase(buf.begin());
					continue;
				}

				buf.erase(buf.begin());
				if (keep_their_modes)
					Status.push_back(cm);
			}
			buf.erase(buf.begin());

			User *u = finduser(buf);
			if (!u)
			{
				Log(LOG_DEBUG) << "FJOIN for nonexistant user " << buf << " on " << c->name;
				continue;
			}

			EventReturn MOD_RESULT;
			FOREACH_RESULT(I_OnPreJoinChannel, OnPreJoinChannel(u, c));

			/* Add the user to the channel */
			c->JoinUser(u);

			/* Update their status internally on the channel
			 * This will enforce secureops etc on the user
			 */
			for (std::list<ChannelMode *>::iterator it = Status.begin(), it_end = Status.end(); it != it_end; ++it)
				c->SetModeInternal(*it, buf);

			/* Now set whatever modes this user is allowed to have on the channel */
			chan_set_correct_modes(u, c, 1);

			/* Check to see if modules want the user to join, if they do
			 * check to see if they are allowed to join (CheckKick will kick/ban them)
			 * Don't trigger OnJoinChannel event then as the user will be destroyed
			 */
			if (MOD_RESULT != EVENT_STOP && c->ci && c->ci->CheckKick(u))
				continue;

			FOREACH_MOD(I_OnJoinChannel, OnJoinChannel(u, c));
		}

		/* Channel is done syncing */
		if (c->HasFlag(CH_SYNCING))
		{
			/* Unset the syncing flag */
			c->UnsetFlag(CH_SYNCING);
			c->Sync();
		}

		return true;
	}
};

