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
#include "common/debug.h"
#include "common/system.h"

namespace CBlood {
PlanetScreen::PlanetScreen(ScreenController &host, GameState &state, Resources &resources,
	Renderer &renderer, CockpitPanel &panel, Sound &sound)
	: Screen(host, state, resources, renderer, panel, sound),
	_renderPoint(0), _planetFrames(0), _planetActive(false) {
	memset(_terrain, 0, sizeof(_terrain));
	memset(_packedPlanet, 0, sizeof(_packedPlanet));
}
void PlanetScreen::initData() {
	_resources.readResource(0x1ac36, _planetState, sizeof(_planetState));
	_resources.readResource(0x180a2, _planetColors, sizeof(_planetColors));
	_resources.readResource(0x1db46, _depthPoints, sizeof(_depthPoints));
	_resources.readResource(0x19df0, _depthState, sizeof(_depthState));
}
void PlanetScreen::setPlanet(uint16 coordinate) {
	WRITE_BE_UINT16(_planetState, coordinate);
	_planetState[24] &= 0x7f;
	WRITE_BE_UINT16(_planetState + 26, coordinate);
}
bool PlanetScreen::open() {
	_panel.cursorYClamp = 50;
	_panel.updateButtons(0x1c0);
	_host.showRing(true);
	if (_host.shouldQuit()) return false;
	memset(_renderer.screen + 18 * 160, 0, 128 * 160);
	_panel.revealFrame = 57;
	_panel.pointerButton = _panel.pointerButtonLatch = 0;
	WRITE_BE_UINT16(_depthState + 26, Planet::initPoints(_depthPoints, _state.localSeed));
	uint16 palette;
	Planet::terrain(_terrain, _planetState, _state.currentPlanet, _state.localSeed, palette,
		[this](int) { _host.pumpFrames(); });
	byte paletteSource[450];
	_resources.readResource(0x1aa32, paletteSource, sizeof(paletteSource));
	Planet::palettes(_planetColors, paletteSource, palette);
	_renderer.setPlanetPalette(_planetColors);
	_renderer.splitPalette = true;
	Planet::projectPoints(_renderer.screen, _depthPoints, _depthState, _state.localSeed);
	Planet::initBuffer(_packedPlanet, _renderer.screen);
	_sound.startSpeech(0x1d016);
	_panel.cursorYClamp = 109;
	_panel.pointerButton = _panel.pointerButtonLatch = 0;
	WRITE_BE_UINT16(_planetState + 28, 40);
	Planet::curve(_planetState + 28);
	_host.waitForDraw();
	_renderPoint = _planetFrames = 0;
	_planetPacer.reset(_host.millis());
	_planetActive = true;
	debug(1, "cblood: Exterior Vision started, Vision Space speech playing");
	return !_host.shouldQuit();
}
void PlanetScreen::update(uint32 now) {
	if (!_planetActive) return;
	uint points = _planetPacer.advance(now, _sound.isPlaying());
	const uint count = 48 * (READ_BE_UINT16(_planetState + 28) + 1) * 4;
	while (points && _planetActive) {
		uint part = MIN(points, count - _renderPoint);
		Planet::renderRange(_packedPlanet, _terrain + 4, _planetState + 28,
			_renderPoint, _renderPoint + part, [](int, int, uint16) {});
		_renderPoint += part;
		points -= part;
		if (_renderPoint != count) continue;
		_host.publishPlanet();
		_planetState[31] -= 2;
		_renderPoint = 0;
		++_planetFrames;
		debug(1, "cblood: planet render %u queued, next phase=%u", _planetFrames, _planetState[31]);
		if (_panel.pointerButtonLatch & 2) {
			_panel.pointerButtonLatch &= ~2;
			if (_panel.cursorX <= 170) {
				_planetActive = false;
				_host.requestCockpit();
			} else {
				warning("cblood: planet action controls are outside the startup implementation");
			}
		}
	}
}

void PlanetScreen::render() {
	Planet::blit(_renderer.screen, _packedPlanet);
	debug(1, "cblood: planet display %u, partial points=%u", _planetFrames, _renderPoint);
}

} // namespace CBlood
