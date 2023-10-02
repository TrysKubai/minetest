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

#pragma once

#include "irrlichttypes.h"
#include "exceptions.h"
#include <iostream>
#include "util/pointer.h"

/*
	Map format serialization version
	--------------------------------

	For map data (blocks, nodes, sectors).

	NOTE: The goal is to increment this so that saved maps will be
	      loadable by any version. Other compatibility is not
		  maintained.

	1: Initial map format after moving from MT
*/
// This represents an uninitialized or invalid format
#define SER_FMT_VER_INVALID 255
// Highest supported serialization version
#define SER_FMT_VER_HIGHEST_READ 1
// Saved on disk version
#define SER_FMT_VER_HIGHEST_WRITE 1
// Lowest supported serialization version
#define SER_FMT_VER_LOWEST_READ 0

#define SER_FMT_VER_LOWEST_WRITE 1

inline bool ser_ver_supported(s32 v) {
	return v >= SER_FMT_VER_LOWEST_READ && v <= SER_FMT_VER_HIGHEST_READ;
}

/*
	Misc. serialization functions
*/

void compressZlib(const u8 *data, size_t data_size, std::ostream &os, int level = -1);
void compressZlib(const std::string &data, std::ostream &os, int level = -1);
void decompressZlib(std::istream &is, std::ostream &os, size_t limit = 0);

void compressZstd(const u8 *data, size_t data_size, std::ostream &os, int level = 0);
void compressZstd(const std::string &data, std::ostream &os, int level = 0);
void decompressZstd(std::istream &is, std::ostream &os);

// These choose between zlib and a self-made one according to version
void compress(const SharedBuffer<u8> &data, std::ostream &os, u8 version, int level = -1);
void compressLegacyForMapGen(const SharedBuffer<u8> &data, std::ostream &os, int level = -1);
void compress(const std::string &data, std::ostream &os, u8 version, int level = -1);
void compress(u8 *data, u32 size, std::ostream &os, u8 version, int level = -1);
void decompress(std::istream &is, std::ostream &os, u8 version);
void decompressLegacyForMapGen(std::istream &is, std::ostream &os);
