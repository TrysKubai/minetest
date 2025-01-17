/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "mapblock.h"

#include <sstream>
#include "map.h"
#include "light.h"
#include "nodedef.h"
#include "nodemetadata.h"
#include "gamedef.h"
#include "irrlicht_changes/printing.h"
#include "log.h"
#include "nameidmapping.h"
#include "serialization.h"
#ifndef SERVER
#include "client/mapblock_mesh.h"
#endif
#include "porting.h"
#include "util/string.h"
#include "util/serialize.h"
#include "util/basic_macros.h"

static const char *modified_reason_strings[] = {
	"initial",
	"reallocate",
	"setIsUnderground",
	"setLightingExpired",
	"setGenerated",
	"setNode",
	"setNodeNoCheck",
	"setTimestamp",
	"NodeMetaRef::reportMetadataChange",
	"clearAllObjects",
	"Timestamp expired (step)",
	"addActiveObjectRaw",
	"removeRemovedObjects/remove",
	"removeRemovedObjects/deactivate",
	"Stored list cleared in activateObjects due to overflow",
	"deactivateFarObjects: Static data moved in",
	"deactivateFarObjects: Static data moved out",
	"deactivateFarObjects: Static data changed considerably",
	"finishBlockMake: expireDayNightDiff",
	"unknown",
};

/*
	MapBlock
*/

MapBlock::MapBlock(Map *parent, v3s16 pos, IGameDef *gamedef):
		m_parent(parent),
		m_pos(pos),
		m_pos_relative(pos * MAP_BLOCKSIZE),
		m_gamedef(gamedef)
{
	reallocate();
}

MapBlock::~MapBlock()
{
#ifndef SERVER
	{
		delete mesh;
		mesh = nullptr;
	}
#endif
}

bool MapBlock::onObjectsActivation()
{
	// Ignore if no stored objects (to not set changed flag)
	if (m_static_objects.getAllStored().empty())
		return false;

	const auto count = m_static_objects.getStoredSize();
	verbosestream << "MapBlock::onObjectsActivation(): "
			<< "activating " << count << "objects in block " << getPos()
			<< std::endl;

	if (count > g_settings->getU16("max_objects_per_block")) {
		errorstream << "suspiciously large amount of objects detected: "
			<< count << " in " << getPos() << "; removing all of them."
			<< std::endl;
		// Clear stored list
		m_static_objects.clearStored();
		raiseModified(MOD_STATE_WRITE_NEEDED, MOD_REASON_TOO_MANY_OBJECTS);
		return false;
	}

	return true;
}

bool MapBlock::saveStaticObject(u16 id, const StaticObject &obj, u32 reason)
{
	if (m_static_objects.getStoredSize() >= g_settings->getU16("max_objects_per_block")) {
		warningstream << "MapBlock::saveStaticObject(): Trying to store id = " << id
				<< " statically but block " << getPos() << " already contains "
				<< m_static_objects.getStoredSize() << " objects."
				<< std::endl;
		return false;
	}

	m_static_objects.insert(id, obj);
	if (reason != MOD_REASON_UNKNOWN) // Do not mark as modified if requested
		raiseModified(MOD_STATE_WRITE_NEEDED, reason);

	return true;
}

// This method is only for Server, don't call it on client
void MapBlock::step(float dtime, const std::function<bool(v3s16, MapNode, f32)> &on_timer_cb)
{
	// Run script callbacks for elapsed node_timers
	std::vector<NodeTimer> elapsed_timers = m_node_timers.step(dtime);
	if (!elapsed_timers.empty()) {
		MapNode n;
		v3s16 p;
		for (const NodeTimer &elapsed_timer : elapsed_timers) {
			n = getNodeNoEx(elapsed_timer.position);
			p = elapsed_timer.position + getPosRelative();
			if (on_timer_cb(p, n, elapsed_timer.elapsed))
				setNodeTimer(NodeTimer(elapsed_timer.timeout, 0, elapsed_timer.position));
		}
	}
}

bool MapBlock::isValidPositionParent(v3s16 p)
{
	if (isValidPosition(p)) {
		return true;
	}

	return m_parent->isValidPosition(getPosRelative() + p);
}

MapNode MapBlock::getNodeParent(v3s16 p, bool *is_valid_position)
{
	if (!isValidPosition(p))
		return m_parent->getNode(getPosRelative() + p, is_valid_position);

	if (is_valid_position)
		*is_valid_position = true;
	return data[p.Z * zstride + p.Y * ystride + p.X];
}

std::string MapBlock::getModifiedReasonString()
{
	std::string reason;

	const u32 ubound = MYMIN(sizeof(m_modified_reason) * CHAR_BIT,
		ARRLEN(modified_reason_strings));

	for (u32 i = 0; i != ubound; i++) {
		if ((m_modified_reason & (1 << i)) == 0)
			continue;

		reason += modified_reason_strings[i];
		reason += ", ";
	}

	if (reason.length() > 2)
		reason.resize(reason.length() - 2);

	return reason;
}


void MapBlock::copyTo(VoxelManipulator &dst)
{
	v3s16 data_size(MAP_BLOCKSIZE, MAP_BLOCKSIZE, MAP_BLOCKSIZE);
	VoxelArea data_area(v3s16(0,0,0), data_size - v3s16(1,1,1));

	// Copy from data to VoxelManipulator
	dst.copyFrom(data, data_area, v3s16(0,0,0),
			getPosRelative(), data_size);
}

void MapBlock::copyFrom(VoxelManipulator &dst)
{
	v3s16 data_size(MAP_BLOCKSIZE, MAP_BLOCKSIZE, MAP_BLOCKSIZE);
	VoxelArea data_area(v3s16(0,0,0), data_size - v3s16(1,1,1));

	// Copy from VoxelManipulator to data
	dst.copyTo(data, data_area, v3s16(0,0,0),
			getPosRelative(), data_size);
}

void MapBlock::actuallyUpdateDayNightDiff()
{
	const NodeDefManager *nodemgr = m_gamedef->ndef();

	// Running this function un-expires m_day_night_differs
	m_day_night_differs_expired = false;

	bool differs = false;

	/*
		Check if any lighting value differs
	*/

	MapNode previous_n(CONTENT_IGNORE);
	for (u32 i = 0; i < nodecount; i++) {
		MapNode n = data[i];

		// If node is identical to previous node, don't verify if it differs
		if (n == previous_n)
			continue;

		differs = !n.isLightDayNightEq(nodemgr->getLightingFlags(n));
		if (differs)
			break;
		previous_n = n;
	}

	/*
		If some lighting values differ, check if the whole thing is
		just air. If it is just air, differs = false
	*/
	if (differs) {
		bool only_air = true;
		for (u32 i = 0; i < nodecount; i++) {
			MapNode &n = data[i];
			if (n.getContent() != CONTENT_AIR) {
				only_air = false;
				break;
			}
		}
		if (only_air)
			differs = false;
	}

	// Set member variable
	m_day_night_differs = differs;
}

void MapBlock::expireDayNightDiff()
{
	m_day_night_differs_expired = true;
}

/*
	Serialization
*/

// List relevant id-name pairs for ids in the block using nodedef
// Renumbers the content IDs (starting at 0 and incrementing)
static void getBlockNodeIdMapping(NameIdMapping *nimap, MapNode *nodes,
	const NodeDefManager *nodedef)
{
	// The static memory requires about 65535 * sizeof(int) RAM in order to be
	// sure we can handle all content ids. But it's absolutely worth it as it's
	// a speedup of 4 for one of the major time consuming functions on storing
	// mapblocks.
	thread_local std::unique_ptr<content_t[]> mapping;
	static_assert(sizeof(content_t) == 2, "content_t must be 16-bit");
	if (!mapping)
		mapping = std::make_unique<content_t[]>(USHRT_MAX + 1);

	memset(mapping.get(), 0xFF, (USHRT_MAX + 1) * sizeof(content_t));

	std::unordered_set<content_t> unknown_contents;
	content_t id_counter = 0;
	for (u32 i = 0; i < MapBlock::nodecount; i++) {
		content_t global_id = nodes[i].getContent();
		content_t id = CONTENT_IGNORE;

		// Try to find an existing mapping
		if (mapping[global_id] != 0xFFFF) {
			id = mapping[global_id];
		} else {
			// We have to assign a new mapping
			id = id_counter++;
			mapping[global_id] = id;

			const ContentFeatures &f = nodedef->get(global_id);
			const std::string &name = f.name;
			if (name.empty())
				unknown_contents.insert(global_id);
			else
				nimap->set(id, name);
		}

		// Update the MapNode
		nodes[i].setContent(id);
	}
	for (u16 unknown_content : unknown_contents) {
		errorstream << "getBlockNodeIdMapping(): IGNORING ERROR: "
				<< "Name for node id " << unknown_content << " not known" << std::endl;
	}
}

// Correct ids in the block to match nodedef based on names.
// Unknown ones are added to nodedef.
// Will not update itself to match id-name pairs in nodedef.
static void correctBlockNodeIds(const NameIdMapping *nimap, MapNode *nodes,
		IGameDef *gamedef)
{
	const NodeDefManager *nodedef = gamedef->ndef();
	// This means the block contains incorrect ids, and we contain
	// the information to convert those to names.
	// nodedef contains information to convert our names to globally
	// correct ids.
	std::unordered_set<content_t> unnamed_contents;
	std::unordered_set<std::string> unallocatable_contents;

	bool previous_exists = false;
	content_t previous_local_id = CONTENT_IGNORE;
	content_t previous_global_id = CONTENT_IGNORE;

	for (u32 i = 0; i < MapBlock::nodecount; i++) {
		content_t local_id = nodes[i].getContent();
		// If previous node local_id was found and same than before, don't lookup maps
		// apply directly previous resolved id
		// This permits to massively improve loading performance when nodes are similar
		// example: default:air, default:stone are massively present
		if (previous_exists && local_id == previous_local_id) {
			nodes[i].setContent(previous_global_id);
			continue;
		}

		std::string name;
		if (!nimap->getName(local_id, name)) {
			unnamed_contents.insert(local_id);
			previous_exists = false;
			continue;
		}

		content_t global_id;
		if (!nodedef->getId(name, global_id)) {
			global_id = gamedef->allocateUnknownNodeId(name);
			if (global_id == CONTENT_IGNORE) {
				unallocatable_contents.insert(name);
				previous_exists = false;
				continue;
			}
		}
		nodes[i].setContent(global_id);

		// Save previous node local_id & global_id result
		previous_local_id = local_id;
		previous_global_id = global_id;
		previous_exists = true;
	}

	for (const content_t c: unnamed_contents) {
		errorstream << "correctBlockNodeIds(): IGNORING ERROR: "
				<< "Block contains id " << c
				<< " with no name mapping" << std::endl;
	}
	for (const std::string &node_name: unallocatable_contents) {
		errorstream << "correctBlockNodeIds(): IGNORING ERROR: "
				<< "Could not allocate global id for node name \""
				<< node_name << "\"" << std::endl;
	}
}


void MapBlock::serialize(std::ostream &result, const u8 version, bool disk, int compression_level)
{
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapBlock format not supported");

	FATAL_ERROR_IF(version < SER_FMT_VER_LOWEST_WRITE, "Serialization version error");

	if(version == 1)
		serializeV1(result, disk, compression_level);
	else
		throw VersionMismatchException("ERROR: MapBlock format not supported");
}

void MapBlock::serializeV1(std::ostream &result, bool disk, int compression_level)
{
	const u8 nameIdMappingVersion = 1;
	const u8 paramsVersion = 1;
	const u8 nodeMetadataVersion = 1;
	const u8 staticObjectVersion = 1;
	const u8 nodeTimerVersion = 1;
	const u8 compressionVersion = 1; 

	std::ostringstream os_raw(std::ios_base::binary);
	std::ostream &os = os_raw;
	
	//	Flags
	u8 flags = 0;
	if(is_underground)
		flags |= 0x01;
	if(getDayNightDiff())
		flags |= 0x02;
	if (!m_generated)
		flags |= 0x08;
	writeU8(os, flags);


	//	Lighting 
	writeU16(os, m_lighting_complete);


	//	Bulk node data & name-id map
	NameIdMapping nimap;
	SharedBuffer<u8> buf;
	
 	if(disk)
	{
		MapNode *tmp_nodes = new MapNode[nodecount];
		memcpy(tmp_nodes, data, nodecount * sizeof(MapNode));
		getBlockNodeIdMapping(&nimap, tmp_nodes, m_gamedef->ndef());

		buf = MapNode::serializeBulk(paramsVersion, tmp_nodes, nodecount);
		delete[] tmp_nodes;

		// write timestamp and node/id mapping first	
		writeU32(os, getTimestamp());
		nimap.serialize(os, nameIdMappingVersion);
	}
	else
	{
		buf = MapNode::serializeBulk(paramsVersion, data, nodecount);
	}

	writeU8(os, paramsVersion);
	os.write(reinterpret_cast<char*>(*buf), buf.getSize());
	

	//	Node metadata
	m_node_metadata.serialize(os, nodeMetadataVersion, disk);


	//	Data that goes to disk, but not the network
	if (disk) {
		
		// Static objects
		m_static_objects.serialize(os, staticObjectVersion);

		// Node Timers
		m_node_timers.serialize(os, nodeTimerVersion);
	}

	compress(os_raw.str(), result, compressionVersion, compression_level);
}

void MapBlock::serializeNetworkSpecific(std::ostream &os)
{
	writeU8(os, 2); // version
}

void MapBlock::deserialize(std::istream &in_compressed, u8 version, bool disk)
{
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapBlock format not supported");

	TRACESTREAM(<<"MapBlock::deSerialize "<< getPos() <<std::endl);

	if(version == 1)
		deserializeV1(in_compressed, disk);
	else
		throw VersionMismatchException("ERROR: MapBlock format not supported");

	TRACESTREAM(<<"MapBlock::deSerialize "<< getPos() <<": Done."<<std::endl);
	
}

void MapBlock::deserializeV1(std::istream &isCompressed, bool disk)
{
	const u8 compressionVersion = 1; 

	m_day_night_differs_expired = false;

	// Decompress the whole block
	std::stringstream in_raw(std::ios_base::binary | std::ios_base::in | std::ios_base::out);
	decompress(isCompressed, in_raw, compressionVersion);
	std::istream &is = in_raw;

	// Flags
	u8 flags = readU8(is);
	is_underground = (flags & 0x01) != 0;
	m_day_night_differs = (flags & 0x02) != 0;
	m_lighting_complete = readU16(is);
	m_generated = (flags & 0x08) == 0;

	NameIdMapping nimap;

	if (disk) {
		// Timestamp
		TRACESTREAM(<<"MapBlock::deSerialize "<<getPos()
				<<": Timestamp"<<std::endl);
		setTimestampNoChangedFlag(readU32(is));
		m_disk_timestamp = m_timestamp;

		// Node/id mapping
		TRACESTREAM(<<"MapBlock::deSerialize "<<getPos()
				<<": NameIdMapping"<<std::endl);
		nimap.deSerialize(is);
	}

	TRACESTREAM(<<"MapBlock::deSerialize "<<getPos()
			<<": Bulk node data"<<std::endl);

	
	//	Bulk node data
	u8 paramsVersion = readU8(is);

	if(paramsVersion != 1)
		throw VersionMismatchException("ERROR: Missmatched map data and params version");
	
	MapNode::deSerializeBulk(is, paramsVersion, data, nodecount);

	
	//	NodeMetadata
	TRACESTREAM(<<"MapBlock::deSerialize "<< getPos() <<": Node metadata"<<std::endl);

	m_node_metadata.deserialize(is, m_gamedef->idef());

	//	Data that is only on disk
	if (disk) {
		// Static objects
		TRACESTREAM(<<"MapBlock::deSerialize "<<getPos()
				<<": Static objects"<<std::endl);
		m_static_objects.deserialize(is);

		// Dynamically re-set ids based on node names
		correctBlockNodeIds(&nimap, data, m_gamedef);

		TRACESTREAM(<<"MapBlock::deSerialize "<< getPos() <<": Node timers"<<std::endl);
			m_node_timers.deserialize(is);
	}
}

void MapBlock::deSerializeNetworkSpecific(std::istream &is)
{
	try {
		readU8(is);
		//const u8 version = readU8(is);
		//if (version != 1)
			//throw SerializationError("unsupported MapBlock version");

	} catch(SerializationError &e) {
		warningstream<<"MapBlock::deSerializeNetworkSpecific(): Ignoring an error"
				<<": "<<e.what()<<std::endl;
	}
}

bool MapBlock::storeActiveObject(u16 id)
{
	if (m_static_objects.storeActiveObject(id)) {
		raiseModified(MOD_STATE_WRITE_NEEDED,
			MOD_REASON_REMOVE_OBJECTS_DEACTIVATE);
		return true;
	}

	return false;
}

u32 MapBlock::clearObjects()
{
	u32 size = m_static_objects.size();
	if (size > 0) {
		m_static_objects.clear();
		raiseModified(MOD_STATE_WRITE_NEEDED, MOD_REASON_CLEAR_ALL_OBJECTS);
	}
	return size;
}

/*
	Get a quick string to describe what a block actually contains
*/
std::string analyze_block(MapBlock *block)
{
	if (block == NULL)
		return "NULL";

	std::ostringstream desc;

	v3s16 p = block->getPos();
	char spos[25];
	porting::mt_snprintf(spos, sizeof(spos), "(%2d,%2d,%2d), ", p.X, p.Y, p.Z);
	desc<<spos;

	switch(block->getModified())
	{
	case MOD_STATE_CLEAN:
		desc<<"CLEAN,           ";
		break;
	case MOD_STATE_WRITE_AT_UNLOAD:
		desc<<"WRITE_AT_UNLOAD, ";
		break;
	case MOD_STATE_WRITE_NEEDED:
		desc<<"WRITE_NEEDED,    ";
		break;
	default:
		desc<<"unknown getModified()="+itos(block->getModified())+", ";
	}

	if(block->isGenerated())
		desc<<"is_gen [X], ";
	else
		desc<<"is_gen [ ], ";

	if(block->getIsUnderground())
		desc<<"is_ug [X], ";
	else
		desc<<"is_ug [ ], ";

	desc<<"lighting_complete: "<<block->getLightingComplete()<<", ";

	bool full_ignore = true;
	bool some_ignore = false;
	bool full_air = true;
	bool some_air = false;
	for(s16 z0=0; z0<MAP_BLOCKSIZE; z0++)
	for(s16 y0=0; y0<MAP_BLOCKSIZE; y0++)
	for(s16 x0=0; x0<MAP_BLOCKSIZE; x0++)
	{
		v3s16 p(x0,y0,z0);
		MapNode n = block->getNodeNoEx(p);
		content_t c = n.getContent();
		if(c == CONTENT_IGNORE)
			some_ignore = true;
		else
			full_ignore = false;
		if(c == CONTENT_AIR)
			some_air = true;
		else
			full_air = false;
	}

	desc<<"content {";

	std::ostringstream ss;

	if(full_ignore)
		ss<<"IGNORE (full), ";
	else if(some_ignore)
		ss<<"IGNORE, ";

	if(full_air)
		ss<<"AIR (full), ";
	else if(some_air)
		ss<<"AIR, ";

	if(ss.str().size()>=2)
		desc<<ss.str().substr(0, ss.str().size()-2);

	desc<<"}, ";

	return desc.str().substr(0, desc.str().size()-2);
}


//END
