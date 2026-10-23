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
bool CockpitScreen::open() {
	_host.queueDraw(kRestore);
	_renderer.splitPalette = false;
	if (_hasReturned) _panel.pollGameClock();
	_hasReturned = true;
	_revealActive = _panel.revealFrame <= 57;
	if (_revealActive) _host.queueDraw(kOorxx);
	_panel.updateButtons(0x21e);
	debug(1, "cblood: returned to cockpit");
	return !_host.shouldQuit();
}

void CockpitScreen::handleInput() {
	if (_revealActive) {
		_host.waitForDraw();
		if (_panel.revealFrame < 59) _host.queueDraw(kOorxx);
		else _revealActive = false;
	}

	_panel.updateButtons(0x21e);
	if (!(_panel.pointerButtonLatch & 2)) {
		_panel.pollGameClock();
		return;
	}
	_panel.pointerButtonLatch = 0;
	uint x = ((_panel.cursorX + 4) >> 1) & 255, y = _panel.cursorY & 255;
	uint32 rectangle = 0x18806;
	if ((_panel.buttonMask & 8) && x >= _resources.readU8(rectangle) &&
	    x <= _resources.readU8(rectangle + 1) && y >= _resources.readU8(rectangle + 2) &&
	    y <= _resources.readU8(rectangle + 3)) {
		_host.showPlanet();
		return;
	}
	_panel.pollGameClock();
}

} // namespace CBlood
