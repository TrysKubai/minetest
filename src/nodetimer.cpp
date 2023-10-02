/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "nodetimer.h"
#include "log.h"
#include "serialization.h"
#include "util/serialize.h"
#include "constants.h" // MAP_BLOCKSIZE

/*
	NodeTimer
*/

void NodeTimer::serialize(std::ostream &os) const
{
	writeF1000(os, timeout);
	writeF1000(os, elapsed);
}

void NodeTimer::deSerialize(std::istream &is)
{
	timeout = readF1000(is);
	elapsed = readF1000(is);
}

/*
	NodeTimerList
*/

void NodeTimerList::serialize(std::ostream &os, const u8 nodeTimerVersion) const
{

	if(nodeTimerVersion == 0)
	{
		// Indicate empty node timer list
		writeU8(os, 0);
		return;
	}
	else if(nodeTimerVersion == 1)
		serializeV1(os);
	else
		throw VersionMismatchException("ERROR: Unsupported node timer version");
}

void NodeTimerList::serializeV1(std::ostream &os) const
{
	const u8 nodeTimerVersion = 1;
	//const u8 singleNodeTimerLength = 10;

	writeU8(os, nodeTimerVersion);
	writeU16(os, m_timers.size());

	for (const auto &timer : m_timers) {
		NodeTimer t = timer.second;
		NodeTimer nt = NodeTimer(t.timeout,
			t.timeout - (f32)(timer.first - m_time), t.position);
		v3s16 p = t.position;

		u16 p16 = p.Z * MAP_BLOCKSIZE * MAP_BLOCKSIZE + p.Y * MAP_BLOCKSIZE + p.X;
		writeU16(os, p16);
		nt.serialize(os);
	}
}


void NodeTimerList::deserialize(std::istream &is)
{
	clear();

	const u8 nodeTimerVersion = readU8(is); 
	
	if(nodeTimerVersion == 0)
	{
		// Empty NodeTimer list
		return;
	}
	else if (nodeTimerVersion == 1)
	{
		deserializeV1(is);
	}
	else
		throw VersionMismatchException("ERROR: Unsupported node timer version");

	
}

void NodeTimerList::deserializeV1(std::istream &is)
{
	u16 count = readU16(is);

	for (u16 i = 0; i < count; i++) {
		u16 p16 = readU16(is);

		v3s16 p;
		p.Z = p16 / MAP_BLOCKSIZE / MAP_BLOCKSIZE;
		p16 &= MAP_BLOCKSIZE * MAP_BLOCKSIZE - 1;
		p.Y = p16 / MAP_BLOCKSIZE;
		p16 &= MAP_BLOCKSIZE - 1;
		p.X = p16;

		NodeTimer t(p);
		t.deSerialize(is);

		if (t.timeout <= 0) {
			warningstream<<"NodeTimerList::deSerialize(): "
					<<"invalid data at position"
					<<"("<<p.X<<","<<p.Y<<","<<p.Z<<"): Ignoring."
					<<std::endl;
			continue;
		}

		if (m_iterators.find(p) != m_iterators.end()) {
			warningstream<<"NodeTimerList::deSerialize(): "
					<<"already set data at position"
					<<"("<<p.X<<","<<p.Y<<","<<p.Z<<"): Ignoring."
					<<std::endl;
			continue;
		}

		insert(t);
	}
}


std::vector<NodeTimer> NodeTimerList::step(float dtime)
{
	std::vector<NodeTimer> elapsed_timers;
	m_time += dtime;
	if (m_next_trigger_time == -1. || m_time < m_next_trigger_time) {
		return elapsed_timers;
	}
	std::multimap<double, NodeTimer>::iterator i = m_timers.begin();
	// Process timers
	for (; i != m_timers.end() && i->first <= m_time; ++i) {
		NodeTimer t = i->second;
		t.elapsed = t.timeout + (f32)(m_time - i->first);
		elapsed_timers.push_back(t);
		m_iterators.erase(t.position);
	}
	// Delete elapsed timers
	m_timers.erase(m_timers.begin(), i);
	if (m_timers.empty())
		m_next_trigger_time = -1.;
	else
		m_next_trigger_time = m_timers.begin()->first;
	return elapsed_timers;
}


