/*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*    In addition, as a special exception, the author gives permission to
*    link the code of this program with the Half-Life Game Engine ("HL
*    Engine") and Modified Game Libraries ("MODs") developed by Valve,
*    L.L.C ("Valve").  You must obey the GNU General Public License in all
*    respects for all of the code used other than the HL Engine and MODs
*    from Valve.  If you modify this file, you may extend this exception
*    to your version of the file, but you are not obligated to do so.  If
*    you do not wish to do so, delete this exception statement from your
*    version.
*
*/

#include "precompiled.h"

CIPRateLimit rateChecker;

cvar_t max_queries_sec        = { "max_queries_sec",        "3.0", FCVAR_SERVER | FCVAR_PROTECTED, 0.0f, nullptr };
cvar_t max_queries_sec_global = { "max_queries_sec_global", "30",  FCVAR_SERVER | FCVAR_PROTECTED, 0.0f, nullptr };
cvar_t max_queries_window     = { "max_queries_window",     "60",  FCVAR_SERVER | FCVAR_PROTECTED, 0.0f, nullptr };
cvar_t sv_logblocks           = { "sv_logblocks",           "0",   FCVAR_SERVER, 0.0f, nullptr };

CIPRateLimit::CIPRateLimit() : m_IPTree(0, START_TREE_SIZE, LessIP)
{
	m_iGlobalCount = 0;
	m_lLastTime = -1;
}

CIPRateLimit::~CIPRateLimit()
{
}

// Sort func for rb tree
bool CIPRateLimit::LessIP(const iprate_t &lhs, const iprate_t &rhs)
{
	return (lhs.ip < rhs.ip);
}

// Return false if this IP has exceeded limits
bool CIPRateLimit::CheckIP(netadr_t adr)
{
	time_t curTime = CRehldsPlatformHolder::get()->time(nullptr);

// Guard against a zero/negative window, which would turn the rate
	// computation into a division by zero (inf) and block all traffic.
	float window = max_queries_window.value;
	if (window <= 0.0f) {
		window = 1.0f;
	}

	// check the per ip rate (do this first, so one person dosing doesn't add to the global max rate
	ip_t clientIP;
	Q_memcpy(&clientIP, adr.ip, sizeof(ip_t));

	// if we have stored too many items
	if (m_IPTree.Count() > MAX_TREE_SIZE)
	{
		ip_t tmp = m_IPTree.LastInorder(); // we step BACKWARD's through the tree
		int i = m_IPTree.FirstInorder();

		// trim 1/3 the entries from the tree and only traverse the max nodes
		while ((m_IPTree.Count() > (2 * MAX_TREE_SIZE) / 3) && i < m_IPTree.MaxElement())
		{
			if (m_IPTree.IsValidIndex(tmp) &&
				(curTime - m_IPTree[tmp].lastTime) > FLUSH_TIMEOUT &&
				m_IPTree[tmp].ip != clientIP)
			{
				ip_t removeIPT = tmp;
				tmp = m_IPTree.PrevInorder(tmp);
				m_IPTree.RemoveAt(removeIPT);
				continue;
			}

			i++;
			tmp = m_IPTree.PrevInorder(tmp);
		}
	}

	// now find the entry and check if its within our rate limits
	iprate_t findEntry;
	findEntry.ip = clientIP;

	ip_t entry = m_IPTree.Find(findEntry);
	if (m_IPTree.IsValidIndex(entry))
	{
		m_IPTree[entry].count++; // a new hit

		if ((curTime - m_IPTree[entry].lastTime) > window)
		{
			m_IPTree[entry].lastTime = curTime;
			m_IPTree[entry].count = 1;
		}
		else
		{
			float query_rate = static_cast<float>(m_IPTree[entry].count) / window;
			if (query_rate > max_queries_sec.value)
			{
				return false;
			}
		}
	}
	else
	{
		// not found, insert this new guy
		iprate_t newEntry;
		newEntry.count = 1;
		newEntry.lastTime = curTime;
		newEntry.ip = clientIP;
		m_IPTree.Insert(newEntry);
	}

	// now check the global rate
	m_iGlobalCount++;

	if ((curTime - m_lLastTime) > window)
	{
		m_lLastTime = curTime;
		m_iGlobalCount = 1;
	}
	else
	{
		float query_rate = static_cast<float>(m_iGlobalCount) / window; // add one so the bottom is never zero
		if (query_rate > max_queries_sec_global.value)
		{
			return false;
		}
	}

	return true;
}

// Return false if this IP exceeds rate limits
bool SV_CheckConnectionLessRateLimits(netadr_t &adr)
{
	bool ret = rateChecker.CheckIP(adr);
	if (!ret && sv_logblocks.value)
	{
		Log_Printf("Traffic from %s was blocked for exceeding rate limits\n", NET_AdrToString(adr));
	}

	return ret;
}
