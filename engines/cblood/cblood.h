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

#ifndef CBLOOD_CBLOOD_H
#define CBLOOD_CBLOOD_H

#include "common/array.h"
#include "engines/engine.h"

struct ADGameDescription;

namespace Audio {
class SoundHandle;
}

namespace CBlood {

class CBloodMusicStream;

class CBloodEngine : public Engine {
public:
	CBloodEngine(OSystem *syst, const ADGameDescription *desc);
	~CBloodEngine() override;

	Common::Error run() override;
	bool hasFeature(EngineFeature f) const override;

private:
	static const int kScreenWidth = 320;
	static const int kScreenHeight = 200;
	static const int kScreenBytes = 32000;
	static const int kCreatureScreenOffset = 0;
	static const int kTitleScreenOffset = 32000;
	static const int kCreaturePaletteOffset = 64000;
	static const int kTitlePaletteOffset = 64032;
	static const int kPaletteColors = 16;
	static const int kVblDelayMs = 20;
	static const int kSelectorDelayVbls = 0x05dc;
	static const int kTransitionFadeVbls = 32;
	static const int kPromptRestoreDelayVbls = 1194;
	static const int kArcheLoadDelayAfterCreatureFadeVbls = 338;

	bool runIntroPresentation();
	void runIdleLoop();
	bool loadFile(const char *name, Common::Array<byte> &data);
	bool loadBloodIScreen(uint32 screenOffset, uint32 paletteOffset, bool hidePrompt, byte *pixels, uint16 *stPalette);
	bool loadArcheScreen(byte *pixels, uint16 *stPalette);
	void showScreen(const byte *pixels, const uint16 *stPalette);
	void transitionToScreen(const byte *pixels, const uint16 *stPalette);
	void fadePaletteFromCodeState(const uint16 *stPalette, int fadeValue, int fadeStep);
	void setPaletteFromStWords(const uint16 *stPalette);
	bool runTitleLoopUntilKey();
	bool advanceSelectorCallbacks(int vbls, bool stopOnKey = false);
	bool forceCreatureSelector();
	bool forceArcheSelector();
	bool runInitialLanguageSelector();
	bool composeInitialLanguageSelector(byte *pixels);
	void drawMaskSource(byte *pixels, const Common::Array<byte> &data, uint32 offset, int rows, int x, int y);
	void drawHand(byte *pixels, const Common::Array<byte> &bloodM);
	bool dispatchInitialLanguageSelector();
	bool setupTitleMusic();
	void stopTitleMusic();
	void delayVbls(int count);
	void decodeAtariLowResScreen(const byte *src, byte *dst) const;
	void decodeAtariPalette(const byte *src, byte *dst) const;

	const ADGameDescription *_gameDescription;
	CBloodMusicStream *_music;
	Audio::SoundHandle *_musicHandle;
	byte _basePixels[kScreenWidth * kScreenHeight];
	uint16 _currentPalette[kPaletteColors];
	bool _hasCurrentPalette;
	bool _selectorShowsTitle;
	bool _keyPressed;
	int _selectorCountdown;
	int _cursorX;
	int _cursorY;
	int _cursorYClamp;
	byte _pointerButton;
	byte _pointerButtonLatch;
	uint16 _handAnimation;
};

} // End of namespace CBlood

#endif
