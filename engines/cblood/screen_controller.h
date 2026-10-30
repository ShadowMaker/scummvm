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
#ifndef CBLOOD_SCREEN_CONTROLLER_H
#define CBLOOD_SCREEN_CONTROLLER_H
#include "cblood/game_state.h"
#include "cblood/resources.h"
#include "cblood/renderer.h"
#include "cblood/sound.h"
#include "cblood/cockpit_panel.h"
#include "cblood/cockpit_screen.h"
#include "cblood/planet_screen.h"
#include "cblood/startup_screen.h"
#include "cblood/galaxy_map_screen.h"
class OSystem;
namespace Common { class EventManager; }
namespace CBlood {
class CBloodEngine;
class ScreenController : public LoadObserver {
public:
	ScreenController(CBloodEngine &engine, OSystem &system, Common::EventManager &events,
		GameState &state, Resources &resources, Renderer &renderer, Sound &sound);
	~ScreenController();
	bool runStartup();
	void handleInput();
	void pumpFrames();
	void waitFrame();
	void pollInput();
	void startFrames();
	void resyncFrame();
	void queueDraw(DrawJob job, int button = 0);
	void executeDraw();
	void waitForDraw();
	void publishPlanet() { _pending = kPlanet; }
	void showRing(bool planetEntry);
	void showPlanet();
	void requestMap() { _nextScreen = &_map; }
	void requestMapPlanet() { _planet.enterFromMap(); _nextScreen = &_planet; }
	bool hasPendingDraw() const { return _pending != kNoJob; }
	void drawHandFrame();
	void requestCockpit() { _nextScreen = &_cockpit; }
	bool serviceLoading() override;
	bool shouldQuit() const;
	uint32 millis() const;
	uint32 playTime() const;
	void delay(uint32 milliseconds);
	void lockMouse(bool lock);
	void purgeKeys();
	void enableKeymap(const char *id, bool enabled);
private:
	void activate(Screen &screen);
	void tickFrame();
	CBloodEngine &_engine;
	OSystem &_system;
	Common::EventManager &_events;
	Renderer &_renderer;
	CockpitPanel _panel;
	CockpitScreen _cockpit;
	PlanetScreen _planet;
	GalaxyMapScreen _map;
	StartupScreen _startup;
	Screen *_active, *_nextScreen;
	bool _clockActive;
	uint32 _nextFrame;
	DrawJob _pending;
	int _pendingButton;
};
} // namespace CBlood
#endif
