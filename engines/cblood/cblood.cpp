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

#include "cblood/cblood.h"
#include "cblood/music.h"

#include "audio/mixer.h"

#include "common/events.h"
#include "common/file.h"
#include "common/system.h"

#include "engines/advancedDetector.h"
#include "engines/util.h"
#include "graphics/paletteman.h"

namespace CBlood {

static uint16 ror16(uint16 value, int count) {
	return (value >> count) | (value << (16 - count));
}

static int clampInt(int value, int minValue, int maxValue) {
	if (value < minValue)
		return minValue;
	if (value > maxValue)
		return maxValue;
	return value;
}

CBloodEngine::CBloodEngine(OSystem *syst, const ADGameDescription *desc)
	: Engine(syst), _gameDescription(desc), _music(nullptr), _musicHandle(nullptr),
	  _hasCurrentPalette(false), _selectorShowsTitle(true), _keyPressed(false),
	  _selectorCountdown(kSelectorDelayVbls), _cursorX(0xa0), _cursorY(0x32),
	  _cursorYClamp(0x32), _pointerButton(0), _pointerButtonLatch(0), _handAnimation(0) {
	memset(_basePixels, 0, sizeof(_basePixels));
	memset(_currentPalette, 0, sizeof(_currentPalette));
}

CBloodEngine::~CBloodEngine() {
	if (_musicHandle) {
		_mixer->stopHandle(*_musicHandle);
		delete _musicHandle;
	}
	delete _music;
}

bool CBloodEngine::hasFeature(EngineFeature f) const {
	return f == kSupportsReturnToLauncher;
}

Common::Error CBloodEngine::run() {
	initGraphics(kScreenWidth, kScreenHeight);

	if (!runIntroPresentation())
		return Common::kReadingFailed;

	runIdleLoop();

	return Common::kNoError;
}

bool CBloodEngine::runIntroPresentation() {
	byte pixels[kScreenWidth * kScreenHeight];
	uint16 stPalette[kPaletteColors];

	if (!loadBloodIScreen(kTitleScreenOffset, kTitlePaletteOffset, true, pixels, stPalette))
		return false;
	showScreen(pixels, stPalette);
	if (!setupTitleMusic())
		return false;

	if (!runTitleLoopUntilKey())
		return true;

	if (!forceCreatureSelector())
		return false;

	if (!advanceSelectorCallbacks(kArcheLoadDelayAfterCreatureFadeVbls))
		return false;

	if (!forceArcheSelector())
		return false;
	return runInitialLanguageSelector();
}

void CBloodEngine::runIdleLoop() {
	while (!shouldQuit()) {
		Common::Event event;
		while (_eventMan->pollEvent(event)) {
			if (event.type == Common::EVENT_QUIT || event.type == Common::EVENT_RETURN_TO_LAUNCHER)
				return;
		}
		_system->delayMillis(kVblDelayMs);
	}
}

bool CBloodEngine::loadFile(const char *name, Common::Array<byte> &data) {
	Common::File file;
	if (!file.open(name))
		return false;

	data.resize(file.size());
	if (data.empty())
		return true;
	return file.read(&data[0], data.size()) == data.size();
}

bool CBloodEngine::loadBloodIScreen(uint32 screenOffset, uint32 paletteOffset, bool hidePrompt, byte *pixels, uint16 *stPalette) {
	Common::File file;
	if (!file.open("BLOOD_I"))
		return false;

	if (file.size() < paletteOffset + kPaletteColors * 2 || file.size() < screenOffset + kScreenBytes)
		return false;

	byte screen[kScreenBytes];
	byte paletteBytes[kPaletteColors * 2];

	file.seek(screenOffset);
	if (file.read(screen, sizeof(screen)) != sizeof(screen))
		return false;

	file.seek(paletteOffset);
	if (file.read(paletteBytes, sizeof(paletteBytes)) != sizeof(paletteBytes))
		return false;
	if (hidePrompt)
		WRITE_BE_UINT16(paletteBytes + 2, 0);
	for (int i = 0; i < kPaletteColors; ++i)
		stPalette[i] = READ_BE_UINT16(paletteBytes + i * 2);

	decodeAtariLowResScreen(screen, pixels);
	return true;
}

bool CBloodEngine::loadArcheScreen(byte *pixels, uint16 *stPalette) {
	Common::File file;
	if (!file.open("ARCHE"))
		return false;

	byte screen[kScreenBytes];
	if (file.read(screen, sizeof(screen)) != sizeof(screen))
		return false;

	if (!_music && !setupTitleMusic())
		return false;
	for (int i = 0; i < kPaletteColors; ++i)
		stPalette[i] = _music->readU16(0x17fc2 + i * 2);

	decodeAtariLowResScreen(screen, pixels);
	return true;
}

void CBloodEngine::showScreen(const byte *pixels, const uint16 *stPalette) {
	memcpy(_basePixels, pixels, sizeof(_basePixels));
	_system->copyRectToScreen(pixels, kScreenWidth, 0, 0, kScreenWidth, kScreenHeight);
	setPaletteFromStWords(stPalette);
	_system->updateScreen();
	memcpy(_currentPalette, stPalette, sizeof(_currentPalette));
	_hasCurrentPalette = true;
}

void CBloodEngine::transitionToScreen(const byte *pixels, const uint16 *stPalette) {
	if (_hasCurrentPalette)
		fadePaletteFromCodeState(_currentPalette, 0, 1);

	_system->copyRectToScreen(pixels, kScreenWidth, 0, 0, kScreenWidth, kScreenHeight);
	_system->updateScreen();
	fadePaletteFromCodeState(stPalette, 7, -1);
	memcpy(_basePixels, pixels, sizeof(_basePixels));
	memcpy(_currentPalette, stPalette, sizeof(_currentPalette));
	_hasCurrentPalette = true;
}

void CBloodEngine::fadePaletteFromCodeState(const uint16 *stPalette, int fadeValue, int fadeStep) {
	while (!shouldQuit()) {
		uint16 faded[kPaletteColors];
		for (int color = 0; color < kPaletteColors; ++color) {
			uint16 d0 = stPalette[color];
			for (int component = 0; component < 3; ++component) {
				uint16 d1 = d0;
				d0 &= 0x7770;
				d1 -= fadeValue;
				if ((d1 & 8) == 0)
					d0 |= d1;
				d0 = ror16(d0, 4);
			}
			faded[color] = ror16(d0, 4);
		}

		setPaletteFromStWords(faded);
		_system->updateScreen();

		delayVbls(2);

		fadeValue += fadeStep;
		if (fadeValue & 8)
			break;
	}
}

void CBloodEngine::setPaletteFromStWords(const uint16 *stPalette) {
	byte palette[kPaletteColors * 3];
	byte paletteBytes[kPaletteColors * 2];
	for (int i = 0; i < kPaletteColors; ++i)
		WRITE_BE_UINT16(paletteBytes + i * 2, stPalette[i]);
	decodeAtariPalette(paletteBytes, palette);
	_system->getPaletteManager()->setPalette(palette, 0, kPaletteColors);
}

bool CBloodEngine::runTitleLoopUntilKey() {
	int promptRestoreCountdown = kPromptRestoreDelayVbls;
	bool promptRestored = false;
	_keyPressed = false;
	_eventMan->purgeKeyboardEvents();

	while (!shouldQuit()) {
		if (!advanceSelectorCallbacks(1, true))
			return false;
		if (_keyPressed)
			return true;

		if (!promptRestored && --promptRestoreCountdown <= 0) {
			byte pixels[kScreenWidth * kScreenHeight];
			uint16 stPalette[kPaletteColors];
			if (!loadBloodIScreen(kTitleScreenOffset, kTitlePaletteOffset, false, pixels, stPalette))
				return false;
			if (_selectorShowsTitle)
				showScreen(pixels, stPalette);
			promptRestored = true;
		}
	}
	return false;
}

bool CBloodEngine::advanceSelectorCallbacks(int vbls, bool stopOnKey) {
	for (int i = 0; i < vbls && !shouldQuit(); ++i) {
		Common::Event event;
		while (_eventMan->pollEvent(event)) {
			if (event.type == Common::EVENT_QUIT || event.type == Common::EVENT_RETURN_TO_LAUNCHER)
				return false;
			if (event.type == Common::EVENT_KEYDOWN) {
				_keyPressed = true;
				if (stopOnKey)
					return true;
			}
		}

		if (--_selectorCountdown < 0) {
			byte pixels[kScreenWidth * kScreenHeight];
			uint16 stPalette[kPaletteColors];
			_selectorShowsTitle = !_selectorShowsTitle;
			if (_selectorShowsTitle) {
				if (!loadBloodIScreen(kTitleScreenOffset, kTitlePaletteOffset, false, pixels, stPalette))
					return false;
			} else {
				if (!loadBloodIScreen(kCreatureScreenOffset, kCreaturePaletteOffset, false, pixels, stPalette))
					return false;
			}
			transitionToScreen(pixels, stPalette);
			_selectorCountdown = kSelectorDelayVbls;
			i += kTransitionFadeVbls;
		}
		_system->delayMillis(kVblDelayMs);
	}
	return !shouldQuit();
}

bool CBloodEngine::forceCreatureSelector() {
	byte pixels[kScreenWidth * kScreenHeight];
	uint16 stPalette[kPaletteColors];
	if (!loadBloodIScreen(kCreatureScreenOffset, kCreaturePaletteOffset, false, pixels, stPalette))
		return false;
	transitionToScreen(pixels, stPalette);
	_selectorShowsTitle = false;
	_selectorCountdown = kSelectorDelayVbls;
	return true;
}

bool CBloodEngine::forceArcheSelector() {
	byte pixels[kScreenWidth * kScreenHeight];
	uint16 stPalette[kPaletteColors];
	if (!loadArcheScreen(pixels, stPalette))
		return false;
	transitionToScreen(pixels, stPalette);
	_selectorShowsTitle = true;
	_selectorCountdown = kSelectorDelayVbls;
	return true;
}

bool CBloodEngine::runInitialLanguageSelector() {
	Common::Array<byte> bloodM;
	if (!loadFile("BLOOD_M", bloodM))
		return false;
	if (!_music && !setupTitleMusic())
		return false;

	_cursorYClamp = 0x6d;
	_pointerButton = 0;
	_pointerButtonLatch = 0;
	_handAnimation = 0;

	if (!composeInitialLanguageSelector(_basePixels))
		return false;
	stopTitleMusic();

	while (!shouldQuit()) {
		byte frame[kScreenWidth * kScreenHeight];
		memcpy(frame, _basePixels, sizeof(frame));
		drawHand(frame, bloodM);
		_system->copyRectToScreen(frame, kScreenWidth, 0, 0, kScreenWidth, kScreenHeight);
		_system->updateScreen();

		Common::Event event;
		while (_eventMan->pollEvent(event)) {
			if (event.type == Common::EVENT_QUIT || event.type == Common::EVENT_RETURN_TO_LAUNCHER)
				return false;
			if (event.type == Common::EVENT_MOUSEMOVE) {
				_cursorX = clampInt(event.mouse.x, 0, 0x120);
				_cursorY = clampInt(event.mouse.y, 0, _cursorYClamp);
			} else if (event.type == Common::EVENT_LBUTTONDOWN) {
				if (_pointerButton != 2)
					_pointerButtonLatch |= 2;
				_pointerButton = 2;
				_cursorX = clampInt(event.mouse.x, 0, 0x120);
				_cursorY = clampInt(event.mouse.y, 0, _cursorYClamp);
			} else if (event.type == Common::EVENT_LBUTTONUP) {
				_pointerButton = 0;
				_cursorX = clampInt(event.mouse.x, 0, 0x120);
				_cursorY = clampInt(event.mouse.y, 0, _cursorYClamp);
			}
		}

		if (dispatchInitialLanguageSelector())
			return true;
		_system->delayMillis(kVblDelayMs);
	}
	return false;
}

bool CBloodEngine::composeInitialLanguageSelector(byte *pixels) {
	Common::Array<byte> arche;
	if (!loadFile("ARCHE", arche) || arche.size() < kScreenBytes)
		return false;

	Common::Array<byte> screen;
	screen.resize(kScreenBytes);
	memcpy(&screen[0], &arche[0], kScreenBytes);

	uint32 src = 0x1d7be;
	uint32 dst = 0x7b548 - 0x78000;
	for (int row = 0; row < 32; ++row) {
		for (int chunk = 0; chunk < 10; ++chunk) {
			if (dst + 8 > screen.size())
				return false;
			screen[dst + 0] = _music->readU8(src + 0);
			screen[dst + 1] = _music->readU8(src + 1);
			screen[dst + 2] = _music->readU8(src + 2);
			screen[dst + 3] = _music->readU8(src + 3);
			screen[dst + 4] = _music->readU8(src + 4);
			screen[dst + 5] = _music->readU8(src + 5);
			screen[dst + 6] = 0;
			screen[dst + 7] = 0;
			src += 6;
			dst += 8;
		}
		dst += 0x50;
	}

	byte selectorPixels[kScreenWidth * kScreenHeight];
	decodeAtariLowResScreen(&screen[0], selectorPixels);
	for (int y = 0x55; y < 0x55 + 32; ++y)
		memcpy(pixels + y * kScreenWidth + 16, selectorPixels + y * kScreenWidth + 16, 160);
	return true;
}

void CBloodEngine::drawMaskSource(byte *pixels, const Common::Array<byte> &data, uint32 offset, int rows, int x, int y) {
	if (offset + rows * 16 > data.size())
		return;

	for (int row = 0; row < rows; ++row) {
		const int dstY = y + row;
		if (dstY < 0 || dstY >= kScreenHeight)
			continue;
		const uint32 rowOffset = offset + row * 16;
		const uint32 maskPlane = READ_BE_UINT32(&data[rowOffset]);
		const uint32 plane0 = READ_BE_UINT32(&data[rowOffset + 4]);
		const uint32 plane1 = READ_BE_UINT32(&data[rowOffset + 8]);
		const uint32 plane2 = READ_BE_UINT32(&data[rowOffset + 12]);
		for (int bit = 0; bit < 32; ++bit) {
			const int dstX = x + bit;
			if (dstX < 0 || dstX >= kScreenWidth)
				continue;
			const uint32 mask = 1u << (31 - bit);
			const byte color = ((plane0 & mask) ? 1 : 0) |
			                   ((plane1 & mask) ? 2 : 0) |
			                   ((plane2 & mask) ? 4 : 0) |
			                   ((maskPlane & mask) ? 8 : 0);
			if (color)
				pixels[dstY * kScreenWidth + dstX] = color;
		}
	}
}

void CBloodEngine::drawHand(byte *pixels, const Common::Array<byte> &bloodM) {
	uint32 topOffset = 0x58400 - 0x58000;
	int cursorY = _cursorY;
	if (_pointerButton != 0) {
		_handAnimation += 0x60;
		topOffset = 0x58400 - 0x58000 - (0x100 + (_handAnimation & 0x0300));
		cursorY += 6;
	}
	cursorY = clampInt(cursorY, 0, _cursorYClamp);

	const int visibleRows = clampInt(cursorY, 0, 0x6d) + 1;
	const int drawY = 200 - cursorY;
	const int topRows = MIN<int>(visibleRows, 16);
	drawMaskSource(pixels, bloodM, topOffset, topRows, _cursorX, drawY);
	if (visibleRows > 16)
		drawMaskSource(pixels, bloodM, 0x58500 - 0x58000, visibleRows - 16, _cursorX, drawY + 16);
}

bool CBloodEngine::dispatchInitialLanguageSelector() {
	if ((_pointerButtonLatch & 2) == 0)
		return false;

	const int d6 = (_cursorX + 4) >> 1;
	const int d7 = _cursorY;
	_pointerButtonLatch = 0;

	if (d7 < 0x55 || d7 > 0x78)
		return false;
	if (d6 >= 0x28 && d6 <= 0x41)
		return true;
	if (d6 >= 0x44 && d6 <= 0x5c)
		return true;
	if (d6 >= 0x5f && d6 <= 0x78)
		return true;
	return false;
}

bool CBloodEngine::setupTitleMusic() {
	if (!_music) {
		_music = new CBloodMusicStream();
		if (!_music->load())
			return false;
	}

	if (!_musicHandle)
		_musicHandle = new Audio::SoundHandle();

	_music->startTitleMusic();
	_mixer->playStream(Audio::Mixer::kMusicSoundType, _musicHandle, _music, -1,
	                   Audio::Mixer::kMaxChannelVolume, 0, DisposeAfterUse::NO);
	return true;
}

void CBloodEngine::stopTitleMusic() {
	if (_musicHandle)
		_mixer->stopHandle(*_musicHandle);
	if (_music)
		_music->stop();
}

void CBloodEngine::delayVbls(int count) {
	for (int i = 0; i < count && !shouldQuit(); ++i) {
		Common::Event event;
		while (_eventMan->pollEvent(event)) {
			if (event.type == Common::EVENT_QUIT || event.type == Common::EVENT_RETURN_TO_LAUNCHER)
				return;
			if (event.type == Common::EVENT_KEYDOWN)
				_keyPressed = true;
		}
		_system->delayMillis(kVblDelayMs);
	}
}

void CBloodEngine::decodeAtariLowResScreen(const byte *src, byte *dst) const {
	for (int y = 0; y < kScreenHeight; ++y) {
		const byte *line = src + y * 160;
		byte *out = dst + y * kScreenWidth;

		for (int block = 0; block < 20; ++block) {
			const byte *planes = line + block * 8;
			const uint16 plane0 = READ_BE_UINT16(planes);
			const uint16 plane1 = READ_BE_UINT16(planes + 2);
			const uint16 plane2 = READ_BE_UINT16(planes + 4);
			const uint16 plane3 = READ_BE_UINT16(planes + 6);

			for (int bit = 15; bit >= 0; --bit) {
				*out++ = (((plane0 >> bit) & 1) << 0) |
				         (((plane1 >> bit) & 1) << 1) |
				         (((plane2 >> bit) & 1) << 2) |
				         (((plane3 >> bit) & 1) << 3);
			}
		}
	}
}

void CBloodEngine::decodeAtariPalette(const byte *src, byte *dst) const {
	static const byte stLevelToRgb[8] = { 0x00, 0x22, 0x44, 0x66, 0x88, 0xaa, 0xcc, 0xee };

	for (int i = 0; i < kPaletteColors; ++i) {
		const uint16 color = READ_BE_UINT16(src + i * 2);
		dst[i * 3 + 0] = stLevelToRgb[(color >> 8) & 7];
		dst[i * 3 + 1] = stLevelToRgb[(color >> 4) & 7];
		dst[i * 3 + 2] = stLevelToRgb[color & 7];
	}
}

} // End of namespace CBlood
