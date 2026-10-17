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
#include "cblood/resources.h"
#include "common/file.h"
#include "common/endian.h"
#include "common/util.h"
namespace CBlood {
bool Resources::loadFile(const char *name, Common::Array<byte> &data, LoadObserver &observer) {
	Common::File file;
	if (!file.open(name)) return false;
	data.resize(file.size());
	for (uint offset = 0; offset < data.size();) {
		uint count = MIN<uint>(16384, data.size() - offset);
		if (file.read(&data[offset], count) != count) return false;
		offset += count;
		if (!observer.serviceLoading()) return false;
	}
	return true;
}

void Resources::readResource(uint32 address, byte *data, uint size) const {
	for (uint i = 0; i < size; ++i) data[i] = readU8(address + i);
}

bool Resources::loadMusic() {
	Common::File file;
	if (!file.open("BLOOD_H"))
		return false;
	Common::Array<byte> encodedBloodH;
	encodedBloodH.resize(file.size());
	if (file.read(&encodedBloodH[0], encodedBloodH.size()) != encodedBloodH.size())
		return false;
	file.close();
	if (!decodeBloodH(encodedBloodH))
		return false;

	if (!file.open("ETHNICOL.JAR"))
		return false;
	_ethnicol.resize(file.size());
	if (file.read(&_ethnicol[0], _ethnicol.size()) != _ethnicol.size())
		return false;
	return true;
}

bool Resources::loadSpeech() {
	Common::File file;
	if (!file.open("BLOOD_VX") || file.size() < 0xb739) return false;
	Common::Array<byte> data;
	data.resize(file.size());
	if (file.read(&data[0], data.size()) != data.size()) return false;
	Common::StackLock lock(_dataMutex);
	_speech = data;
	return true;
}

bool Resources::hasSpeech() const {
	Common::StackLock lock(_dataMutex);
	return !_speech.empty();
}
bool Resources::decodeBloodH(const Common::Array<byte> &encoded) {
	static const byte key1[] = {
		0x49, 0x4e, 0x53, 0x45, 0x52, 0x54, 0x82, 0x44, 0x49, 0x53, 0x4b, 0x87,
		0x32, 0x85, 0x54, 0x48, 0x45, 0x4e, 0x5e, 0x50, 0x52, 0x45, 0x53
	};
	static const byte key2[] = {
		0x7b, 0x1d, 0x65, 0x15, 0x0d, 0x4c, 0x4e, 0xa7, 0x35, 0x5a, 0x89, 0x94,
		0x5d, 0x55, 0x3c, 0x9f, 0x0a, 0x33, 0x1b, 0x74, 0x2e, 0xaa, 0xdd, 0x9c,
		0x5c, 0xf2, 0x28, 0xbc, 0x2d, 0xc6, 0x81
	};

	if (encoded.size() < kBloodHDecodeLength) return false;
	_bloodH.clear();
	_bloodH.resize(kBloodHDecodedEnd - kBloodHBase);
	for (uint i = 0; i < encoded.size() && i < _bloodH.size(); ++i)
		_bloodH[i] = encoded[i];

	if (_bloodH.size() < kBloodHDecodeLength)
		return false;
	for (uint i = 0; i < kBloodHDecodeLength; ++i)
		_bloodH[i] ^= key1[i % ARRAYSIZE(key1)] ^ key2[i % ARRAYSIZE(key2)];
	return true;
}

bool Resources::bytesAt(uint32 address, uint32 size, const byte **ptr) const {
	if (address >= 0x4b400 && address - 0x4b400 <= _speech.size() &&
	    size <= _speech.size() - (address - 0x4b400)) {
		*ptr = &_speech[address - 0x4b400];
		return true;
	}
	if (address >= kBloodHBase && address + size <= kBloodHBase + _bloodH.size()) {
		*ptr = &_bloodH[address - kBloodHBase];
		return true;
	}
	if (address >= kEthnicolBase && address + size <= kEthnicolBase + _ethnicol.size()) {
		*ptr = &_ethnicol[address - kEthnicolBase];
		return true;
	}
	return false;
}

uint8 Resources::readU8(uint32 address) const {
	Common::StackLock lock(_dataMutex);
	const byte *ptr = nullptr;
	if (!bytesAt(address, 1, &ptr))
		return 0;
	return ptr[0];
}

uint16 Resources::readU16(uint32 address) const {
	Common::StackLock lock(_dataMutex);
	const byte *ptr = nullptr;
	if (!bytesAt(address, 2, &ptr))
		return 0;
	return READ_BE_UINT16(ptr);
}

int16 Resources::readS16(uint32 address) const {
	return (int16)readU16(address);
}

uint32 Resources::readU32(uint32 address) const {
	Common::StackLock lock(_dataMutex);
	const byte *ptr = nullptr;
	if (!bytesAt(address, 4, &ptr))
		return 0;
	return READ_BE_UINT32(ptr);
}

} // namespace CBlood
