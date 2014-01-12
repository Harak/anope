/* ChanServ core functions
 *
 * (C) 2003-2014 Anope Team
 * Contact us at team@anope.org
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of Epona by Lara.
 * Based on the original code of Services by Andy Church.
 */

struct CSSuspendInfo
{
	Anope::string chan, by, reason;
	time_t time, expires;

	virtual ~CSSuspendInfo() { }
 protected:
	CSSuspendInfo() { }
};
