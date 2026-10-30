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
#ifndef CBLOOD_SCREEN_H
#define CBLOOD_SCREEN_H
#include "common/events.h"
namespace CBlood {
class ScreenController;
class GameState;
class Resources;
class Renderer;
class CockpitPanel;
class Sound;
enum DrawJob { kNoJob, kCover, kReveal, kBackup, kRestore, kFlags, kOorxx, kDigits, kPlanet, kMap };
class Screen {
public:
	Screen(ScreenController &host, GameState &state, Resources &resources,
		Renderer &renderer, CockpitPanel &panel, Sound &sound)
		: _host(host), _state(state), _resources(resources), _renderer(renderer), _panel(panel), _sound(sound) {}
	virtual ~Screen() {}
	virtual bool open() { return true; }
	virtual void close() {}
	virtual void handleEvent(const Common::Event &event) {}
	virtual void handleInput() {}
	virtual void update(uint32 now) {}
	virtual void render() {}
protected:
	ScreenController &_host;
	GameState &_state;
	Resources &_resources;
	Renderer &_renderer;
	CockpitPanel &_panel;
	Sound &_sound;
};
} // namespace CBlood
#endif
