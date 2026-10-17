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
#include "cblood/sound.h"
#include "cblood/resources.h"
#include "audio/audiostream.h"
#include "common/array.h"
#include "common/mutex.h"

namespace CBlood {
class CBloodMusicStream : public Audio::AudioStream {
public:
	explicit CBloodMusicStream(Resources &resources);
	~CBloodMusicStream() override {}

	void startTitleMusic();
	void startSpeech(uint32 phrase);
	bool isPlaying();
	void stop();
	bool introMarkerReached();

	int readBuffer(int16 *buffer, const int numSamples) override;
	bool isStereo() const override { return false; }
	int getRate() const override { return kSampleRate; }
	bool endOfData() const override { return !_enabled; }
	bool endOfStream() const override { return false; }

private:
	static const int kSampleRate = 44100;
	static const uint32 kControlStreamStart = 0x1df8e;
	static const uint32 kSegmentTableBase = 0x1df3e;
	static const uint32 kInitSequence = 0x171e8;
	static const int kMfpClockHz = 2457600;

	void writePsg(uint8 reg, uint8 value);
	void writeInitSequence(uint32 address);
	void timerStep();
	double timerFrequency() const;
	int16 currentSample() const;

	const Resources &_resources;
	uint32 _controlPtr;
	uint32 _controlStart, _segmentTable;
	Common::Mutex _markerMutex;
	bool _introMarker;
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

static const double kYmVolume[16] = {
	0.0000, 0.0046, 0.0068, 0.0102,
	0.0154, 0.0230, 0.0347, 0.0518,
	0.0782, 0.1166, 0.1758, 0.2625,
	0.3955, 0.5909, 0.8913, 1.0000
};

CBloodMusicStream::CBloodMusicStream(Resources &resources)
	: _resources(resources), _controlPtr(kControlStreamStart), _controlStart(kControlStreamStart),
	  _segmentTable(kSegmentTableBase), _introMarker(false), _activePtr(0), _endPtr(0), _byteModeForward(true),
	  _loop(true), _enabled(false), _oneShotDelay(-1), _timerData(0x6e),
	  _timerControl(1), _samplesUntilTimerStep(0.0) {
	memset(_regs, 0, sizeof(_regs));
}

void CBloodMusicStream::startTitleMusic() {
	Common::StackLock lock(_markerMutex);
	_introMarker = false;
	_controlStart = kControlStreamStart;
	_segmentTable = kSegmentTableBase;
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
	Common::StackLock lock(_markerMutex);
	_enabled = false;
}

void CBloodMusicStream::startSpeech(uint32 phrase) {
	Common::StackLock lock(_markerMutex);
	_controlStart = _controlPtr = phrase;
	_segmentTable = 0x1cf1a;
	_activePtr = _endPtr = 0;
	_byteModeForward = true;
	_loop = false;
	_enabled = _resources.hasSpeech();
	_delayStack.clear();
	_oneShotDelay = -1;
	_timerData = _resources.readU8(0x1d1ec);
	_timerControl = 1;
	writeInitSequence(kInitSequence);
	_samplesUntilTimerStep = (double)kSampleRate / timerFrequency();
}

int CBloodMusicStream::readBuffer(int16 *buffer, const int numSamples) {
	Common::StackLock lock(_markerMutex);
	for (int i = 0; i < numSamples; ++i) {
		if (!_enabled) {
			buffer[i] = 0;
			continue;
		}

		buffer[i] = currentSample();
		_samplesUntilTimerStep -= 1.0;
		while (_samplesUntilTimerStep <= 0.0 && _enabled) {
			timerStep();
			if (_controlPtr == 0x1dfcc)
				_introMarker = true;
			_samplesUntilTimerStep += (double)kSampleRate / timerFrequency();
		}
	}
	return numSamples;
}

bool CBloodMusicStream::introMarkerReached() {
	Common::StackLock lock(_markerMutex);
	return _introMarker;
}

bool CBloodMusicStream::isPlaying() {
	Common::StackLock lock(_markerMutex);
	return _enabled;
}

void CBloodMusicStream::writePsg(uint8 reg, uint8 value) {
	_regs[reg & 0x0f] = value;
}

void CBloodMusicStream::writeInitSequence(uint32 address) {
	while (true) {
		const uint16 word = _resources.readU16(address);
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
			streamByte = _resources.readU8(_activePtr++);
		else
			streamByte = _resources.readU8(--_activePtr);

		const uint32 entry = 0x17658 + (streamByte & 0xfc);
		writePsg(_resources.readU8(entry), _resources.readU8(entry + 1));
		writePsg(_resources.readU8(entry + 2), _resources.readU8(entry + 3));
		return;
	}

	const uint16 raw = _resources.readU16(_controlPtr);
	const int16 signedWord = (int16)raw;
	_controlPtr += 2;

	if (signedWord < 0) {
		if (signedWord < -0x100) {
			_timerData = raw & 0xff;
			return;
		}

		const int16 tableOffset = (int16)(raw & 0xfffe);
		const uint32 entry = _segmentTable - tableOffset;
		const uint32 first = _resources.readU32(entry);
		const uint32 second = _resources.readU32(entry + 4);
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
			_controlPtr = _controlStart;
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

Sound::Sound(Audio::Mixer &mixer, Resources &resources)
	: _mixer(mixer), _resources(resources), _stream(new CBloodMusicStream(resources)) {}

Sound::~Sound() { stopTitleMusic(); }

bool Sound::setupTitleMusic() {
	_mixer.stopHandle(_handle);
	if (!_resources.loadMusic()) return false;
	_stream->startTitleMusic();
	_mixer.playStream(Audio::Mixer::kMusicSoundType, &_handle, _stream.get(), -1,
		Audio::Mixer::kMaxChannelVolume, 0, DisposeAfterUse::NO);
	return true;
}

bool Sound::loadSpeech() { return _resources.loadSpeech(); }
bool Sound::isPlaying() { return _stream->isPlaying(); }
bool Sound::introMarkerReached() { return _stream->introMarkerReached(); }

void Sound::startSpeech(uint32 phrase) {
	_mixer.stopHandle(_handle);
	_stream->startSpeech(phrase);
	_mixer.playStream(Audio::Mixer::kSpeechSoundType, &_handle, _stream.get(), -1,
		Audio::Mixer::kMaxChannelVolume, 0, DisposeAfterUse::NO);
}

void Sound::stopTitleMusic() {
	_mixer.stopHandle(_handle);
	_stream->stop();
}
} // namespace CBlood
