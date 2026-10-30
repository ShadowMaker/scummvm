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
#include "cblood/screen_controller.h"
#include "cblood/input.h"
#include "common/debug.h"
#include "common/system.h"

namespace CBlood {
StartupScreen::StartupScreen(ScreenController &host, GameState &state, Resources &resources,
	Renderer &renderer, CockpitPanel &panel, Sound &sound, PlanetScreen &planet)
	: Screen(host, state, resources, renderer, panel, sound), _planet(planet),
	_introActive(false), _keyPressed(false), _markerSeen(false), _selectedImage(0), _activePalette(3) {}
bool StartupScreen::loadFile(const char *name, Common::Array<byte> &data) {
	return _resources.loadFile(name, data, _host);
}
void StartupScreen::handleEvent(const Common::Event &event) {
	if (event.type == Common::EVENT_CUSTOM_ENGINE_ACTION_START && event.customType == kContinue)
		_keyPressed = true;
}
void StartupScreen::pollMusic() {
	if (_introActive && !_markerSeen && _sound.introMarkerReached()) {
		_markerSeen = true;
		_intro.request(0);
	}
}
bool StartupScreen::runStartup() {
	_host.enableKeymap("cblood-intro", true);
	Common::Array<byte> images;
	if (!loadFile("BLOOD_I", images) || images.size() < 64064 || !_sound.setupTitleMusic())
		return false;
	memcpy(_renderer.creature, &images[0], 32000);
	memcpy(_renderer.screen, &images[32000], 32000);
	for (int i = 0; i < 16; ++i) {
		_renderer.palettes[0][i] = READ_BE_UINT16(&images[64032 + i * 2]);
		_renderer.palettes[1][i] = READ_BE_UINT16(&images[64000 + i * 2]);
		_renderer.palettes[2][i] = _resources.readU16(0x17fc2 + i * 2);
	}
	_renderer.palettes[0][1] = 0;
	for (int i = 0; i < 1920; ++i) _resources.flags[i] = _resources.readU8(0x1d7be + i);
	_renderer.setPalette(_renderer.palettes[0]);
	_renderer.present();
	_renderer.palettes[0][1] = READ_BE_UINT16(&images[64034]);
	_host.startFrames();
	if (!_sound.loadSpeech()) return false;
	_host.purgeKeys();
	debug(1, "cblood: intro key gate ready");
	if (!runTitleLoop()) return false;
	_host.enableKeymap("cblood-intro", false);
	debug(1, "cblood: intro key accepted");
	_keyPressed = false;
	_markerSeen = true;
	fadeIntro(1);
	debug(1, "cblood: creature hold started");
	for (int tick = 0; tick < 338 && !_host.shouldQuit(); ++tick) _host.waitFrame();
	if (!loadFile("BLOOD_M", _resources.bloodM) || _resources.bloodM.size() < 0x1922 ||
	    !loadFile("BLOOD.DIK", _state.dialogue) || _state.dialogue.size() < 0x16c4)
		return false;
	Common::Array<byte> arche;
	if (!loadFile("ARCHE", arche) || arche.size() < 32000) return false;
	uint16 oldColors[16];
	memcpy(oldColors, _renderer.currentPalette, sizeof(oldColors));
	fadeIntroColors(oldColors, 0, 1);
	memcpy(_renderer.screen, &arche[0], 32000);
	memcpy(_renderer.cockpit, _renderer.screen, 32000);
	_renderer.shownImage = 0;
	_renderer.present();
	fadeIntroColors(_renderer.palettes[2], 7, -1);
	_renderer.setPalette(_renderer.palettes[2]);
	debug(1, "cblood: Arche fade complete");
	_introActive = true;
	_intro.selector = 0xff;
	_intro.hold = 1500;
	_activePalette = _selectedImage = 2;
	_panel.buttonMask = _resources.readU16(0x17dfa);
	for (int index = 0; index < 10; ++index) {
		uint32 descriptor = 0x17dfe + index * 8;
		if (_resources.readU16(descriptor + 4) != 1 || _resources.readU16(descriptor + 6) != 19 ||
		    _resources.readU16(descriptor + 2) + 19 * 160 + 24 > 32000)
			return false;
	}
	// The first cover waits 33 VBLs; subsequent publications wait one.
	_coverWait = 32;
	_panel.updateButtons(0);
	if (_host.shouldQuit()) return false;
	debug(1, "cblood: initial cover publications complete");
	_sound.stopTitleMusic();
	_introActive = false;
	_panel.handActive = true;
	_renderer.shownImage = 0;
	_renderer.setPalette(_renderer.palettes[2]);
	_host.lockMouse(true);
	_panel.updateButtons(0);
	if (!chooseLanguage()) return false;
	if (!initPlanets()) return false;
	_panel.pollGameClock();
	_panel.updateButtons(0x1c);
	_host.showPlanet();
	return !_host.shouldQuit();
}

bool StartupScreen::runTitleLoop() {
	int promptTicks = 1194;
	int imageTicks = 1500;
	bool promptShown = false;
	while (!_host.shouldQuit()) {
		_host.pollInput();
		if (_keyPressed) return true;
		if (--imageTicks < 0) {
			fadeIntro(_renderer.shownImage == 1 ? 0 : 1);
			imageTicks = 1500;
		}
		_host.waitFrame();
		if (!promptShown && --promptTicks <= 0) {
			if (_renderer.shownImage == 0) _renderer.setPalette(_renderer.palettes[0]);
			promptShown = true;
			debug(1, "cblood: title prompt restored");
		}
	}
	return false;
}

void StartupScreen::fadeIntroColors(const uint16 *colors, int level, int step) {
	const uint32 start = _host.millis();
	int lastLevel = -1;
	while (!_host.shouldQuit()) {
		uint32 elapsed = _host.millis() - start;
		if (elapsed >= 320) break;
		int current = level + int(elapsed / 40) * step;
		uint16 faded[16];
		for (int i = 0; i < 16; ++i) faded[i] = Startup::fadeColor(colors[i], current);
		_renderer.setPalette(faded);
		_renderer.present();
		lastLevel = current;
		const uint32 nextStep = (elapsed / 40 + 1) * 40;
		while (!_host.shouldQuit() && _host.millis() - start < nextStep) {
			_host.pollInput();
			_host.delay(1);
		}
	}
	int finalLevel = level + 7 * step;
	if (!_host.shouldQuit() && lastLevel != finalLevel) {
		uint16 faded[16];
		for (int i = 0; i < 16; ++i) faded[i] = Startup::fadeColor(colors[i], finalLevel);
		_renderer.setPalette(faded);
		_renderer.present();
	}
	debug(1, "cblood: intro fade %s completed in %u ms", step > 0 ? "out" : "in",
		_host.millis() - start);
	_host.resyncFrame();
}

void StartupScreen::fadeIntro(int image) {
	uint16 oldColors[16];
	memcpy(oldColors, _renderer.currentPalette, sizeof(oldColors));
	fadeIntroColors(oldColors, 0, 1);
	_renderer.shownImage = image;
	_renderer.present();
	fadeIntroColors(_renderer.palettes[image], 7, -1);
	_renderer.setPalette(_renderer.palettes[image]);
}

bool StartupScreen::chooseLanguage() {
	_host.showRing(false);
	_host.queueDraw(kFlags);
	debug(1, "cblood: language flags queued");
	_panel.cursorYClamp = 109;
	while (!_host.shouldQuit()) {
		_host.pollInput();
		if (_panel.pointerButtonLatch & 2) {
			_state.language = Startup::hitLanguage(_panel.cursorX, _panel.cursorY);
			if (_state.language != Startup::kNoLanguage) {
				Startup::selectLanguage(&_state.dialogue[0], _state.language);
				WRITE_BE_UINT16(_resources.flags, READ_BE_UINT16(_resources.flags) - 1);
				_host.queueDraw(kRestore);
				debug(1, "cblood: language %d selected, restore queued", int(_state.language));
				return true;
			}
			_panel.pointerButtonLatch = 0;
		}
		_host.waitFrame();
	}
	return false;
}

bool StartupScreen::initPlanets() {
	_resources.readResource(0x16b5c, _state.alienPlanets, sizeof(_state.alienPlanets));
	_planet.initData();
	_panel.initData();
	_state.localSeed = (uint32(_resources.readU16(0x1a8ea)) << 16) | _resources.readU16(0x1a8ec);
	do {
		_state.gameStartMillis = _host.playTime();
		_panel.resetClock();
		// IKBD clock-reset command: 1,310,952 cycles at 8,021,247 Hz.
		const uint32 clockResetStart = _host.millis();
		while (!_host.shouldQuit() && _host.millis() - clockResetStart < 164) {
			_host.pumpFrames();
			_host.delay(1);
		}
		if (_host.shouldQuit()) return false;
		debug(1, "cblood: startup clock reset completed in %u ms", _host.millis() - clockResetStart);
		_panel.revealFrame = 0;
		int sampled;
		do {
			sampled = _panel.revealFrame;
			_host.queueDraw(kOorxx);
		} while (sampled <= 57 && !_host.shouldQuit());
		if (_host.shouldQuit()) return false;
	} while (!_state.initCoordinates());
	debug(1, "cblood: initial reveal publication complete, counter=%u", _panel.revealFrame);
	_state.currentPlanet = READ_BE_UINT16(_state.alienPlanets + _state.alienOffset);
	_planet.setPlanet(_state.currentPlanet);
	_state.alienRewrite = _state.alienOffset;
	uint bit = 100 + (_state.alienOffset / 4) * 5;
	uint offset = 0x1730 + bit / 8;
	if (offset + 1 >= _state.dialogue.size()) return false;
	_state.alienState = ((_state.dialogue[offset] | (_state.dialogue[offset + 1] << 8)) >> (bit & 7)) & 31;
	_host.waitForDraw();
	Planet::formatDigits(_panel.digits, byte(_state.currentPlanet));
	Planet::formatDigits(_panel.digits + 6, byte(_state.currentPlanet >> 8) & 127);
	Planet::formatDigits(_panel.digits + 24, byte(_state.currentPlanet));
	Planet::formatDigits(_panel.digits + 30, byte(_state.currentPlanet >> 8) & 127);
	_host.queueDraw(kDigits);
	debug(1, "cblood: planet coordinates %u,%u, alien %u", byte(_state.currentPlanet),
		_state.currentPlanet >> 8, _state.alienOffset / 4 + 1);
	return !_host.shouldQuit();
}
void StartupScreen::tickFrame() {
		if (_coverWait > 0) {
			--_coverWait;
			return;
		}
		int actions = _intro.tick();
		if (actions & 2)
			_selectedImage = (_intro.selector & 0x80) ? 2 : _intro.selector ? 1 : 0;
		if (actions & 4) {
			uint16 colors[16];
			for (int i = 0; i < 16; ++i)
				colors[i] = Startup::fadeColor(_renderer.palettes[_activePalette][i], _intro.level);
			_renderer.setPalette(colors);
			if (_intro.advanceFade()) {
				_renderer.shownImage = _selectedImage == 1 ? 1 : 0;
				_activePalette = _selectedImage;
			}
		}
		if (actions & 1) _host.executeDraw();
}

} // namespace CBlood
