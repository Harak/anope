/* Main processing code for Services.
 *
 * (C) 2003-2014 Anope Team
 * Contact us at team@anope.org
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of Epona by Lara.
 * Based on the original code of Services by Andy Church.
 */

#include "services.h"
#include "modules.h"
#include "protocol.h"
#include "servers.h"
#include "users.h"
#include "regchannel.h"

void Anope::Process(const Anope::string &buffer)
{
	/* If debugging, log the buffer */
	Log(LOG_RAWIO) << "Received: " << buffer;

	if (buffer.empty())
		return;

	spacesepstream buf_sep(buffer);

	Anope::string source;
	if (buffer[0] == ':')
	{
		buf_sep.GetToken(source);
		source.erase(0, 1);
	}

	Anope::string command;
	if (!buf_sep.GetToken(command))
		return;
	
	Anope::string buf_token;
	std::vector<Anope::string> params;
	while (buf_sep.GetToken(buf_token))
	{
		if (buf_token[0] == ':')
		{
			if (!buf_sep.StreamEnd())
				params.push_back(buf_token.substr(1) + " " + buf_sep.GetRemaining());
			else
				params.push_back(buf_token.substr(1));
			break;
		}
		else
			params.push_back(buf_token);
	}

	if (Anope::ProtocolDebug)
	{
		Log() << "Source : " << (source.empty() ? "No source" : source);
		Log() << "Command: " << command;

		if (params.empty())
			Log() << "No params";
		else
			for (unsigned i = 0; i < params.size(); ++i)
				Log() << "params " << i << ": " << params[i];
	}

	static const Anope::string proto_name = ModuleManager::FindFirstOf(PROTOCOL) ? ModuleManager::FindFirstOf(PROTOCOL)->name : "";

	MessageSource src(source);
	
	EventReturn MOD_RESULT;
	FOREACH_RESULT(OnMessage, MOD_RESULT, (src, command, params));
	if (MOD_RESULT == EVENT_STOP)
		return;

	ServiceReference<IRCDMessage> m("IRCDMessage", proto_name + "/" + command.lower());
	if (!m)
	{
		Log(LOG_DEBUG) << "unknown message from server (" << buffer << ")";
		return;
	}

	if (m->HasFlag(IRCDMESSAGE_SOFT_LIMIT) ? (params.size() < m->GetParamCount()) : (params.size() != m->GetParamCount()))
		Log(LOG_DEBUG) << "invalid parameters for " << command << ": " << params.size() << " != " << m->GetParamCount();
	else if (m->HasFlag(IRCDMESSAGE_REQUIRE_USER) && !src.GetUser())
		Log(LOG_DEBUG) << "unexpected non-user source " << source << " for " << command;
	else if (m->HasFlag(IRCDMESSAGE_REQUIRE_SERVER) && !source.empty() && !src.GetServer())
		Log(LOG_DEBUG) << "unexpected non-server source " << source << " for " << command;
	else
		m->Run(src, params);
}

