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

#include "serialization.h"

#include "util/serialize.h"

#include <zlib.h>
#include <zstd.h>

/* report a zlib or i/o error */
static void zerr(int ret)
{
	dstream<<"zerr: ";
	switch (ret) {
	case Z_ERRNO:
		if (ferror(stdin))
			dstream<<"error reading stdin"<<std::endl;
		if (ferror(stdout))
			dstream<<"error writing stdout"<<std::endl;
		break;
	case Z_STREAM_ERROR:
		dstream<<"invalid compression level"<<std::endl;
		break;
	case Z_DATA_ERROR:
		dstream<<"invalid or incomplete deflate data"<<std::endl;
		break;
	case Z_MEM_ERROR:
		dstream<<"out of memory"<<std::endl;
		break;
	case Z_VERSION_ERROR:
		dstream<<"zlib version mismatch!"<<std::endl;
		break;
	default:
		dstream<<"return value = "<<ret<<std::endl;
	}
}

// Make sure that z is deleted in case of exception
template <int (*F)(z_stream*)>
class ZlibAutoDeleter {
public:
	ZlibAutoDeleter(z_stream *ptr) : ptr_(ptr) {}
	~ZlibAutoDeleter() { F(ptr_); }

private:
	z_stream *ptr_;
};

void compressZlib(const u8 *data, size_t data_size, std::ostream &os, int level)
{
	z_stream z;
	const s32 bufsize = 16384;
	char output_buffer[bufsize];
	int status = 0;
	int ret;

	z.zalloc = Z_NULL;
	z.zfree = Z_NULL;
	z.opaque = Z_NULL;

	ret = deflateInit(&z, level);
	if(ret != Z_OK)
		throw SerializationError("compressZlib: deflateInit failed");

	ZlibAutoDeleter<deflateEnd> deleter(&z);

	// Point zlib to our input buffer
	z.next_in = (Bytef*)&data[0];
	z.avail_in = data_size;
	// And get all output
	for(;;)
	{
		z.next_out = (Bytef*)output_buffer;
		z.avail_out = bufsize;

		status = deflate(&z, Z_FINISH);
		if(status == Z_NEED_DICT || status == Z_DATA_ERROR
				|| status == Z_MEM_ERROR)
		{
			zerr(status);
			throw SerializationError("compressZlib: deflate failed");
		}
		int count = bufsize - z.avail_out;
		if(count)
			os.write(output_buffer, count);
		// This determines zlib has given all output
		if(status == Z_STREAM_END)
			break;
	}
}

void compressZlib(const std::string &data, std::ostream &os, int level)
{
	compressZlib((u8*)data.c_str(), data.size(), os, level);
}

void decompressZlib(std::istream &is, std::ostream &os, size_t limit)
{
	z_stream z;
	const s32 bufsize = 16384;
	char input_buffer[bufsize];
	char output_buffer[bufsize];
	int status = 0;
	int ret;
	int bytes_written = 0;
	int input_buffer_len = 0;

	z.zalloc = Z_NULL;
	z.zfree = Z_NULL;
	z.opaque = Z_NULL;

	ret = inflateInit(&z);
	if(ret != Z_OK)
		throw SerializationError("dcompressZlib: inflateInit failed");

	ZlibAutoDeleter<inflateEnd> deleter(&z);

	z.avail_in = 0;

	for(;;)
	{
		int output_size = bufsize;
		z.next_out = (Bytef*)output_buffer;
		z.avail_out = output_size;

		if (limit) {
			int limit_remaining = limit - bytes_written;
			if (limit_remaining <= 0) {
				// we're aborting ahead of time - throw an error?
				break;
			}
			if (limit_remaining < output_size) {
				z.avail_out = output_size = limit_remaining;
			}
		}

		if(z.avail_in == 0)
		{
			z.next_in = (Bytef*)input_buffer;
			is.read(input_buffer, bufsize);
			input_buffer_len = is.gcount();
			z.avail_in = input_buffer_len;
		}
		if(z.avail_in == 0)
		{
			break;
		}

		status = inflate(&z, Z_NO_FLUSH);

		if(status == Z_NEED_DICT || status == Z_DATA_ERROR
				|| status == Z_MEM_ERROR)
		{
			zerr(status);
			throw SerializationError("decompressZlib: inflate failed");
		}
		int count = output_size - z.avail_out;
		if(count)
			os.write(output_buffer, count);
		bytes_written += count;
		if(status == Z_STREAM_END)
		{
			// Unget all the data that inflate didn't take
			is.clear(); // Just in case EOF is set
			for(u32 i=0; i < z.avail_in; i++)
			{
				is.unget();
				if(is.fail() || is.bad())
				{
					dstream<<"unget #"<<i<<" failed"<<std::endl;
					dstream<<"fail="<<is.fail()<<" bad="<<is.bad()<<std::endl;
					throw SerializationError("decompressZlib: unget failed");
				}
			}

			break;
		}
	}
}

struct ZSTD_Deleter {
	void operator() (ZSTD_CStream* cstream) {
		ZSTD_freeCStream(cstream);
	}

	void operator() (ZSTD_DStream* dstream) {
		ZSTD_freeDStream(dstream);
	}
};

void compressZstd(const u8 *data, size_t data_size, std::ostream &os, int level)
{
	// reusing the context is recommended for performance
	// it will be destroyed when the thread ends
	thread_local std::unique_ptr<ZSTD_CStream, ZSTD_Deleter> stream(ZSTD_createCStream());


	ZSTD_initCStream(stream.get(), level);

	const size_t bufsize = 16384;
	char output_buffer[bufsize];

	ZSTD_inBuffer input = { data, data_size, 0 };
	ZSTD_outBuffer output = { output_buffer, bufsize, 0 };

	while (input.pos < input.size) {
		size_t ret = ZSTD_compressStream(stream.get(), &output, &input);
		if (ZSTD_isError(ret)) {
			dstream << ZSTD_getErrorName(ret) << std::endl;
			throw SerializationError("compressZstd: failed");
		}
		if (output.pos) {
			os.write(output_buffer, output.pos);
			output.pos = 0;
		}
	}

	size_t ret;
	do {
		ret = ZSTD_endStream(stream.get(), &output);
		if (ZSTD_isError(ret)) {
			dstream << ZSTD_getErrorName(ret) << std::endl;
			throw SerializationError("compressZstd: failed");
		}
		if (output.pos) {
			os.write(output_buffer, output.pos);
			output.pos = 0;
		}
	} while (ret != 0);

}

void compressZstd(const std::string &data, std::ostream &os, int level)
{
	compressZstd((u8*)data.c_str(), data.size(), os, level);
}

void decompressZstd(std::istream &is, std::ostream &os)
{
	// reusing the context is recommended for performance
	// it will be destroyed when the thread ends
	thread_local std::unique_ptr<ZSTD_DStream, ZSTD_Deleter> stream(ZSTD_createDStream());

	ZSTD_initDStream(stream.get());

	const size_t bufsize = 16384;
	char output_buffer[bufsize];
	char input_buffer[bufsize];

	ZSTD_outBuffer output = { output_buffer, bufsize, 0 };
	ZSTD_inBuffer input = { input_buffer, 0, 0 };
	size_t ret;
	do
	{
		if (input.size == input.pos) {
			is.read(input_buffer, bufsize);
			input.size = is.gcount();
			input.pos = 0;
		}

		ret = ZSTD_decompressStream(stream.get(), &output, &input);
		if (ZSTD_isError(ret)) {
			dstream << ZSTD_getErrorName(ret) << std::endl;
			throw SerializationError("decompressZstd: failed");
		}
		if (output.pos) {
			os.write(output_buffer, output.pos);
			output.pos = 0;
		}
	} while (ret != 0);

	// Unget all the data that ZSTD_decompressStream didn't take
	is.clear(); // Just in case EOF is set
	for (u32 i = 0; i < input.size - input.pos; i++) {
		is.unget();
		if (is.fail() || is.bad())
			throw SerializationError("decompressZstd: unget failed");
	}
}

void compress(u8 *data, u32 size, std::ostream &os, u8 version, int level)
{
	compressZstd(data, size, os, level + 1);
	return;
}

void compress(const SharedBuffer<u8> &data, std::ostream &os, u8 version, int level)
{
	compress(*data, data.getSize(), os, version, level);
}

void compress(const std::string &data, std::ostream &os, u8 version, int level)
{
	compress((u8*)data.c_str(), data.size(), os, version, level);
}

void compressLegacyForMapGen(const SharedBuffer<u8> &data, std::ostream &os, int level)
{
	// //! Map Gen still uses ZLib for created blocks this needs to be resolved 
	// //! before removing this
	compressZlib(*data, data.getSize(), os, level + 1);
	return;
}


void decompress(std::istream &is, std::ostream &os, u8 version)
{
	decompressZstd(is, os);
	return;
}

void decompressLegacyForMapGen(std::istream &is, std::ostream &os)
{
	// //! Map Gen still uses ZLib for created blocks this needs to be resolved 
	// //! before removing this
	decompressZlib(is, os);
	return;
}


