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

#ifndef CBLOOD_MUSIC_H
#define CBLOOD_MUSIC_H

#include "audio/audiostream.h"
#include "common/array.h"

namespace CBlood {

class CBloodMusicStream : public Audio::AudioStream {
public:
	CBloodMusicStream();
	~CBloodMusicStream() override {}

	bool load();
	void startTitleMusic();
	void stop();
	uint8 readU8(uint32 address) const;
	uint16 readU16(uint32 address) const;

	int readBuffer(int16 *buffer, const int numSamples) override;
	bool isStereo() const override { return false; }
	int getRate() const override { return kSampleRate; }
	bool endOfData() const override { return !_enabled; }
	bool endOfStream() const override { return false; }

private:
	static const int kSampleRate = 44100;
	static const uint32 kBloodHBase = 0x16000;
	static const uint32 kEthnicolBase = 0x1e084;
	static const uint32 kBloodHDecodedEnd = 0x1e084;
	static const uint32 kBloodHDecodeLength = 0x7e30;
	static const uint32 kControlStreamStart = 0x1df8e;
	static const uint32 kSegmentTableBase = 0x1df3e;
	static const uint32 kInitSequence = 0x171e8;
	static const int kMfpClockHz = 2457600;

	bool decodeBloodH(const Common::Array<byte> &encoded);
	bool bytesAt(uint32 address, uint32 size, const byte **ptr) const;
	int16 readS16(uint32 address) const;
	uint32 readU32(uint32 address) const;

	void writePsg(uint8 reg, uint8 value);
	void writeInitSequence(uint32 address);
	void timerStep();
	double timerFrequency() const;
	int16 currentSample() const;

	Common::Array<byte> _bloodH;
	Common::Array<byte> _ethnicol;

	uint32 _controlPtr;
	uint32 _activePtr;
	uint32 _endPtr;
	bool _byteModeForward;
	bool _loop;
	bool _enabled;

	struct DelayFrame {
		int count;
		uint32 returnPtr;
	};
	Common::Array<DelayFrame> _delayStack;
	int _oneShotDelay;

	uint8 _timerData;
	uint8 _timerControl;
	double _samplesUntilTimerStep;
	uint8 _regs[16];
};

} // End of namespace CBlood

#endif
