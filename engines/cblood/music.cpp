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

#include "cblood/music.h"

#include "common/file.h"

namespace CBlood {

static const double kYmVolume[16] = {
	0.0000, 0.0046, 0.0068, 0.0102,
	0.0154, 0.0230, 0.0347, 0.0518,
	0.0782, 0.1166, 0.1758, 0.2625,
	0.3955, 0.5909, 0.8913, 1.0000
};

CBloodMusicStream::CBloodMusicStream()
	: _controlPtr(kControlStreamStart), _activePtr(0), _endPtr(0), _byteModeForward(true),
	  _loop(true), _enabled(false), _oneShotDelay(-1), _timerData(0x6e),
	  _timerControl(1), _samplesUntilTimerStep(0.0) {
	memset(_regs, 0, sizeof(_regs));
}

bool CBloodMusicStream::load() {
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

void CBloodMusicStream::startTitleMusic() {
	_controlPtr = kControlStreamStart;
	_activePtr = 0;
	_endPtr = 0;
	_byteModeForward = true;
	_loop = true;
	_enabled = true;
	_delayStack.clear();
	_oneShotDelay = -1;
	_timerData = 0x6e;
	_timerControl = 1;
	memset(_regs, 0, sizeof(_regs));
	writeInitSequence(kInitSequence);
	_samplesUntilTimerStep = (double)kSampleRate / timerFrequency();
}

void CBloodMusicStream::stop() {
	_enabled = false;
}

int CBloodMusicStream::readBuffer(int16 *buffer, const int numSamples) {
	for (int i = 0; i < numSamples; ++i) {
		if (!_enabled) {
			buffer[i] = 0;
			continue;
		}

		buffer[i] = currentSample();
		_samplesUntilTimerStep -= 1.0;
		while (_samplesUntilTimerStep <= 0.0 && _enabled) {
			timerStep();
			_samplesUntilTimerStep += (double)kSampleRate / timerFrequency();
		}
	}
	return numSamples;
}

bool CBloodMusicStream::decodeBloodH(const Common::Array<byte> &encoded) {
	static const byte key1[] = {
		0x49, 0x4e, 0x53, 0x45, 0x52, 0x54, 0x82, 0x44, 0x49, 0x53, 0x4b, 0x87,
		0x32, 0x85, 0x54, 0x48, 0x45, 0x4e, 0x5e, 0x50, 0x52, 0x45, 0x53
	};
	static const byte key2[] = {
		0x7b, 0x1d, 0x65, 0x15, 0x0d, 0x4c, 0x4e, 0xa7, 0x35, 0x5a, 0x89, 0x94,
		0x5d, 0x55, 0x3c, 0x9f, 0x0a, 0x33, 0x1b, 0x74, 0x2e, 0xaa, 0xdd, 0x9c,
		0x5c, 0xf2, 0x28, 0xbc, 0x2d, 0xc6, 0x81
	};

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

bool CBloodMusicStream::bytesAt(uint32 address, uint32 size, const byte **ptr) const {
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

uint8 CBloodMusicStream::readU8(uint32 address) const {
	const byte *ptr = nullptr;
	if (!bytesAt(address, 1, &ptr))
		return 0;
	return ptr[0];
}

uint16 CBloodMusicStream::readU16(uint32 address) const {
	const byte *ptr = nullptr;
	if (!bytesAt(address, 2, &ptr))
		return 0;
	return READ_BE_UINT16(ptr);
}

int16 CBloodMusicStream::readS16(uint32 address) const {
	return (int16)readU16(address);
}

uint32 CBloodMusicStream::readU32(uint32 address) const {
	const byte *ptr = nullptr;
	if (!bytesAt(address, 4, &ptr))
		return 0;
	return READ_BE_UINT32(ptr);
}

void CBloodMusicStream::writePsg(uint8 reg, uint8 value) {
	_regs[reg & 0x0f] = value;
}

void CBloodMusicStream::writeInitSequence(uint32 address) {
	while (true) {
		const uint16 word = readU16(address);
		if (word & 0x8000)
			break;
		writePsg(word >> 8, word & 0xff);
		address += 2;
	}
}

void CBloodMusicStream::timerStep() {
	if (_oneShotDelay >= 0) {
		--_oneShotDelay;
		if (_oneShotDelay < 0)
			_oneShotDelay = -1;
		return;
	}

	if (_activePtr != _endPtr) {
		uint8 streamByte;
		if (_byteModeForward)
			streamByte = readU8(_activePtr++);
		else
			streamByte = readU8(--_activePtr);

		const uint32 entry = 0x17658 + (streamByte & 0xfc);
		writePsg(readU8(entry), readU8(entry + 1));
		writePsg(readU8(entry + 2), readU8(entry + 3));
		return;
	}

	const uint16 raw = readU16(_controlPtr);
	const int16 signedWord = (int16)raw;
	_controlPtr += 2;

	if (signedWord < 0) {
		if (signedWord < -0x100) {
			_timerData = raw & 0xff;
			return;
		}

		const int16 tableOffset = (int16)(raw & 0xfffe);
		const uint32 entry = kSegmentTableBase - tableOffset;
		const uint32 first = readU32(entry);
		const uint32 second = readU32(entry + 4);
		if (raw & 1) {
			_activePtr = second;
			_endPtr = first;
			_byteModeForward = false;
		} else {
			_activePtr = first;
			_endPtr = second;
			_byteModeForward = true;
		}
		return;
	}

	const int delay = signedWord >> 1;
	if (delay == 0) {
		if (!_delayStack.empty()) {
			DelayFrame &frame = _delayStack.back();
			--frame.count;
			if (frame.count >= 0) {
				_controlPtr = frame.returnPtr;
				return;
			}
			_delayStack.pop_back();
			return;
		}
		if (_loop) {
			_controlPtr = kControlStreamStart;
			return;
		}
		_enabled = false;
		return;
	}

	if (raw & 1) {
		_oneShotDelay = delay;
		return;
	}

	DelayFrame frame;
	frame.count = delay - 1;
	frame.returnPtr = _controlPtr;
	_delayStack.push_back(frame);
}

double CBloodMusicStream::timerFrequency() const {
	static const int kPrescale[8] = { 0, 4, 10, 16, 50, 64, 100, 200 };
	const int data = _timerData ? _timerData : 256;
	const int control = _timerControl & 7;
	const int prescale = kPrescale[control] ? kPrescale[control] : 4;
	return (double)kMfpClockHz / (double)(prescale * data);
}

int16 CBloodMusicStream::currentSample() const {
	const double a = kYmVolume[_regs[8] & 0x0f];
	const double b = kYmVolume[_regs[9] & 0x0f];
	const double c = kYmVolume[_regs[10] & 0x0f];
	const double sample = ((a + b + c) / 3.0 - 0.12) * 22000.0;
	return (int16)CLIP<int>((int)sample, -32768, 32767);
}

} // End of namespace CBlood
