/* NickServ core functions
 *
 * (C) 2003-2014 Anope Team
 * Contact us at team@anope.org
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of Epona by Lara.
 * Based on the original code of Services by Andy Church.
 */

struct NSSuspendInfo
{
	Anope::string nick, by, reason;
	time_t when, expires;

	virtual ~NSSuspendInfo() { }
 protected:
	NSSuspendInfo() { }
};
