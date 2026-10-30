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
#ifndef CBLOOD_GALAXY_MAP_SCREEN_H
#define CBLOOD_GALAXY_MAP_SCREEN_H

#include "cblood/screen.h"
#include "cblood/galaxy_map.h"
#include "common/array.h"

namespace CBlood {
class GalaxyMapScreen : public Screen {
public:
	using Screen::Screen;
	bool open() override;
	void close() override;
	void handleEvent(const Common::Event &event) override;
	void handleInput() override;
	void render() override;
	bool ownsFrames() const { return _foreground; }
private:
	enum MapDraw { kStrip, kBackground, kVertical, kHorizontal, kReadout };
	void queue(MapDraw draw, uint value = 0);
	void sync(bool wait = true);
	void change(const GalaxyMap::Selection &selection);
	void animateStrips();
	void requestTravel();
	GalaxyMap _markers;
	Common::Array<byte> _resource;
	MapDraw _draw = kBackground;
	uint _value = 0, _stripOffset = 0, _stripColumns = 0;
	uint _keyAction = 0;
	bool _foreground = false;
	bool _keyboardReadout = false;
	uint16 _pointerX = 0, _pointerY = 0;
	int _countdown = 4;
	uint32 _nextUpdate = 0;
	uint32 _paceRemainder = 0;
};
} // namespace CBlood
#endif
