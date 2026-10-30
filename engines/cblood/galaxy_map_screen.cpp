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
#include "common/translation.h"
#include "gui/message.h"

namespace CBlood {

bool GalaxyMapScreen::open() {
	if (_resource.empty()) {
		if (!_resources.loadFile("BLOOD_K", _resource, _host) || _resource.size() < 0x5022) {
			_resource.clear();
			GUI::MessageDialog(_("Could not load BLOOD_K.")).runModal();
			_host.requestCockpit();
			return false;
		}
	}
	_panel.pointerButtonLatch = 0;
	_panel.updateButtons(0x20);
	_panel.revealFrame = 57;
	_host.showRing(true);
	animateStrips();
	uint16 colors[16];
	for (uint i = 0; i < 16; ++i) colors[i] = _resources.readU16(0x18002 + i * 2);
	_renderer.splitPalette = false;
	_renderer.setPalette(colors);
	queue(kBackground);
	_sound.startSpeech(0x1cfea);
	queue(kVertical, byte(_state.mapPlanet));
	queue(kHorizontal, byte(_state.mapPlanet >> 8));
	_panel.handRows = 38;
	_panel.cursorYClamp = 188;
	_host.waitForDraw();
	_keyAction = 0;
	_keyboardReadout = false;
	_countdown = 4;
	_nextUpdate = _host.millis();
	_paceRemainder = 0;
	_foreground = true;
	_host.enableKeymap("cblood-map", true);
	debug(1, "cblood: galaxy map ready %u,%u", byte(_state.mapPlanet), byte(_state.mapPlanet >> 8));
	return !_host.shouldQuit();
}

void GalaxyMapScreen::close() {
	_foreground = false;
	_keyAction = 0;
	_host.enableKeymap("cblood-map", false);
	_panel.handRows = _panel.cursorYClamp = 109;
	_renderer.splitPalette = false;
	_renderer.setPalette(_renderer.palettes[2]);
	_host.resyncFrame();
}

void GalaxyMapScreen::queue(MapDraw draw, uint value) {
	if (_foreground) {
		while (_host.hasPendingDraw() && !_host.shouldQuit()) sync();
	} else {
		_host.waitForDraw();
	}
	_draw = draw;
	_value = value;
	_host.queueDraw(kMap);
}

void GalaxyMapScreen::animateStrips() {
	const uint placements = _resources.readU16(0x17e7c) + 1;
	for (uint i = 0; i < placements && !_host.shouldQuit(); ++i) {
		_host.waitForDraw();
		_stripOffset = _resources.readU16(0x17e7e + i * 4);
		_stripColumns = _resources.readU16(0x17e80 + i * 4) + 1;
		for (uint frame = 0; frame < 6 && !_host.shouldQuit(); ++frame) {
			queue(kStrip, frame % 3);
			_host.waitForDraw();
		}
		queue(kStrip, 3);
	}
}

void GalaxyMapScreen::render() {
	byte *screen = _renderer.screen;
	const byte *backing = &_resource[0x22];
	switch (_draw) {
	case kStrip:
		for (uint row = 0; row < 5; ++row)
			for (uint column = 0; column < _stripColumns; ++column)
				_resources.readResource(0x18174 + _value * 40 + row * 8,
					screen + _stripOffset + row * 160 + column * 8, 8);
		break;
	case kBackground:
		memcpy(screen + 0xb40, backing, 20480);
		break;
	case kVertical:
	case kHorizontal: {
		byte panels[16 * 160];
		memcpy(panels, screen + 21 * 160, sizeof(panels));
		if (_draw == kVertical)
			_markers.vertical(screen, backing, _value);
		else
			_markers.horizontal(screen, backing, _value);
		// Keep crosshairs behind the readout panels.
		for (uint y = 21; y <= 36; ++y) {
			for (uint x = 0; x < 320; x += 16) {
				uint16 mask = 0;
				for (uint bit = 0; bit < 16; ++bit)
					if ((x + bit >= 4 && x + bit <= 86) || x + bit >= 236)
						mask |= 0x8000 >> bit;
				for (uint plane = 0; plane < 4; ++plane) {
					byte *dest = screen + y * 160 + x / 2 + plane * 2;
					const byte *saved = panels + (y - 21) * 160 + x / 2 + plane * 2;
					WRITE_BE_UINT16(dest, (READ_BE_UINT16(dest) & ~mask) | (READ_BE_UINT16(saved) & mask));
				}
			}
		}
		Planet::drawDigits(screen, _resources.digitGlyphs, _panel.digits + 24);
		Planet::drawDigits(screen, _resources.digitGlyphs, _panel.digits + 30);
		break;
	}
	case kReadout:
		Planet::drawDigits(screen, _resources.digitGlyphs, _panel.digits + 12);
		Planet::drawDigits(screen, _resources.digitGlyphs, _panel.digits + 18);
		break;
	}
}

void GalaxyMapScreen::sync(bool wait) {
	// Measured blocking-gate cadence; outer-control polling keeps its shorter mean.
	const uint32 cycles = wait ? 160256 : 57348;
	_paceRemainder += cycles * 1000;
	_nextUpdate += _paceRemainder / 8021247;
	_paceRemainder %= 8021247;
	while (!_host.shouldQuit() && int32(_nextUpdate - _host.millis()) > 0) {
		_host.pumpFrames();
		_host.delay(1);
	}
	_host.pollInput();
	_host.drawHandFrame();
	--_countdown;
}

void GalaxyMapScreen::change(const GalaxyMap::Selection &selection) {
	if (selection.axis == GalaxyMap::kNoReadout) return;
	_state.mapPlanet = GalaxyMap::selected(_state.mapPlanet, selection);
	if (selection.axis == GalaxyMap::kXAxis || selection.axis == GalaxyMap::kYAxis) {
		bool xAxis = selection.axis == GalaxyMap::kXAxis;
		uint value = xAxis ? selection.x : selection.y;
		Planet::formatDigits(_panel.digits + (xAxis ? 24 : 30), byte(value));
		MapDraw marker = xAxis ? kVertical : kHorizontal;
		// Native axis setters replace the pending coordinate readout.
		if (_foreground && _host.hasPendingDraw()) {
			_draw = marker;
			_value = value;
		} else {
			queue(marker, value);
		}
		sync();
		debug(1, "cblood: map selection %u,%u", byte(_state.mapPlanet), byte(_state.mapPlanet >> 8));
	}
	sync();
	Planet::formatDigits(_panel.digits + 12, selection.x & 255);
	Planet::formatDigits(_panel.digits + 18, selection.y & 127);
	queue(kReadout);
}

void GalaxyMapScreen::handleEvent(const Common::Event &event) {
	if (event.type == Common::EVENT_CUSTOM_ENGINE_ACTION_START &&
	    event.customType >= kMapUp && event.customType <= kMapRight)
		_keyAction = event.customType;
	else if (event.type == Common::EVENT_CUSTOM_ENGINE_ACTION_END)
		_keyAction = 0;
}

void GalaxyMapScreen::requestTravel() {
	if (!_state.mapPlanet || _state.mapPlanet == _state.currentPlanet) {
		while (_host.hasPendingDraw() && !_host.shouldQuit()) sync();
		_host.queueDraw(kRestore);
		_host.requestMap();
		return;
	}
	_host.requestMapPlanet();
}

void GalaxyMapScreen::handleInput() {
	if (!_foreground || _host.shouldQuit()) return;
	const uint x = ((_panel.cursorX + 4) >> 1) & 255;
	const uint y = _panel.cursorY & 255;
	int hit = -1;
	for (uint i = 0; i < 4; ++i) {
		uint address = 0x188b4 + i * 8;
		if (x >= _resources.readU8(address) && x <= _resources.readU8(address + 1) &&
		    y >= _resources.readU8(address + 2) && y <= _resources.readU8(address + 3)) {
			hit = i;
			break;
		}
	}
	if (hit == 0) {
		sync();
		_panel.pointerButtonLatch = 0;
		if (_panel.cursorX != _pointerX || _panel.cursorY != _pointerY || _panel.pointerButton)
			_keyboardReadout = false;
		if (_keyboardReadout)
			sync();
		else
			change(GalaxyMap::pointer(_state.mapPlanet, _panel.cursorX, y, _panel.pointerButton));
	} else if (hit == 1) {
		byte buttons = _panel.pointerButtonLatch;
		_panel.pointerButtonLatch &= ~1;
		if (buttons & 1) {
			_host.requestMapPlanet();
			return;
		}
		_panel.pointerButtonLatch &= ~2;
		if (buttons & 2) {
			requestTravel();
			return;
		}
		sync(false);
	} else if (hit == 2) {
		byte buttons = _panel.pointerButtonLatch;
		_panel.pointerButtonLatch &= ~2;
		if (buttons & 2) {
			_host.requestCockpit();
			debug(1, "cblood: galaxy map cancelled");
			return;
		}
		sync(false);
	} else {
		if (hit < 0) _panel.pointerButtonLatch = 0;
		sync();
	}
	if (_countdown < 0) {
		const GalaxyMap::Selection selection = GalaxyMap::keyboard(_state.mapPlanet, _keyAction, _state.handMotion.gate);
		if (selection.axis != GalaxyMap::kNoReadout) {
			change(selection);
			_keyboardReadout = true;
			_pointerX = _panel.cursorX;
			_pointerY = _panel.cursorY;
		}
		_countdown = 4;
	}
}

} // namespace CBlood
