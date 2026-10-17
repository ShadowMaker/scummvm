/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef CBLOOD_RESOURCES_H
#define CBLOOD_RESOURCES_H

#include "common/array.h"
#include "common/mutex.h"

namespace CBlood {
class LoadObserver {
public:
	virtual ~LoadObserver() {}
	virtual bool serviceLoading() = 0;
};

class Resources {
public:
	Resources() { memset(flags, 0, sizeof(flags)); }
	bool loadFile(const char *name, Common::Array<byte> &data, LoadObserver &observer);
	bool loadMusic();
	bool loadSpeech();
	bool hasSpeech() const;
	uint8 readU8(uint32 address) const;
	uint16 readU16(uint32 address) const;
	int16 readS16(uint32 address) const;
	uint32 readU32(uint32 address) const;
	void readResource(uint32 address, byte *data, uint size) const;
	Common::Array<byte> bloodM;
	byte flags[1920], digitGlyphs[122];
private:
	static const uint32 kBloodHBase = 0x16000;
	static const uint32 kEthnicolBase = 0x1e084;
	static const uint32 kBloodHDecodedEnd = 0x1e084;
	static const uint32 kBloodHDecodeLength = 0x7e30;
	bool decodeBloodH(const Common::Array<byte> &encoded);
	bool bytesAt(uint32 address, uint32 size, const byte **ptr) const;
	Common::Array<byte> _bloodH, _ethnicol, _speech;
	mutable Common::Mutex _dataMutex;
};
} // namespace CBlood
#endif
