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
#include "backends/keymapper/keymapper.h"
#include "backends/keymapper/keymap.h"
#include "cblood/ring_timing.h"
#include "common/debug.h"
#include "common/system.h"
#include "cblood/cblood.h"

namespace CBlood {
ScreenController::ScreenController(CBloodEngine &engine, OSystem &system, Common::EventManager &events,
	GameState &state, Resources &resources, Renderer &renderer, Sound &sound)
	: _engine(engine), _system(system), _events(events), _renderer(renderer),
	_panel(*this, state, resources, renderer),
	_cockpit(*this, state, resources, renderer, _panel, sound),
	_planet(*this, state, resources, renderer, _panel, sound),
	_map(*this, state, resources, renderer, _panel, sound),
	_startup(*this, state, resources, renderer, _panel, sound, _planet),
	_active(nullptr), _nextScreen(nullptr), _clockActive(false), _nextFrame(0),
	_pending(kNoJob), _pendingButton(0) {}
ScreenController::~ScreenController() {
	if (_active) _active->close();
	_system.lockMouse(false);
}
bool ScreenController::shouldQuit() const { return _engine.shouldQuit(); }
uint32 ScreenController::millis() const { return _system.getMillis(); }
uint32 ScreenController::playTime() const { return _engine.getTotalPlayTime(); }
void ScreenController::delay(uint32 milliseconds) { _system.delayMillis(milliseconds); }
void ScreenController::lockMouse(bool lock) { _system.lockMouse(lock); }
void ScreenController::purgeKeys() { _events.purgeKeyboardEvents(); }
void ScreenController::enableKeymap(const char *id, bool enabled) {
	Common::Keymap *keymap = _events.getKeymapper()->getKeymap(id);
	if (keymap) keymap->setEnabled(enabled);
}
void ScreenController::startFrames() { _clockActive = true; resyncFrame(); }
void ScreenController::resyncFrame() { _nextFrame = millis() + 20; }
bool ScreenController::serviceLoading() { pumpFrames(); return !shouldQuit(); }
bool ScreenController::runStartup() {
	_active = &_startup;
	return _startup.runStartup();
}
void ScreenController::activate(Screen &screen) {
	Screen *previous = _active;
	_active = nullptr;
	if (previous) previous->close();
	if (screen.open()) _active = &screen;
}
void ScreenController::showPlanet() { activate(_planet); }
void ScreenController::handleInput() {
	if (_nextScreen) {
		Screen *next = _nextScreen;
		_nextScreen = nullptr;
		activate(*next);
	}
	if (_active) _active->handleInput();
}
void ScreenController::pollInput() {
	Common::Event event;
	while (_events.pollEvent(event)) {
		_panel.handleEvent(event);
		if (_active) _active->handleEvent(event);
	}
	_startup.pollMusic();
}
void ScreenController::pumpFrames() {
	if (!_clockActive) return;
	pollInput();
	uint32 now = millis();
	while (!shouldQuit() && int32(now - _nextFrame) >= 0) {
		if (_active) _active->update(_nextFrame);
		_nextFrame += 20;
		tickFrame();
	}
	if (_active) _active->update(now);
}

void ScreenController::waitFrame() {
	if (_map.ownsFrames()) return;
	uint32 next = _nextFrame;
	do {
		pumpFrames();
		if (_nextFrame != next || shouldQuit()) return;
		delay(1);
	} while (true);
}

void ScreenController::queueDraw(DrawJob job, int button) {
	while (_pending != kNoJob && !shouldQuit()) waitFrame();
	if (shouldQuit()) return;
	_pendingButton = button;
	_pending = job;
}

void ScreenController::showRing(bool planetEntry) {
	const uint32 start = millis();
	const RingTiming *timings = planetEntry ? kPlanetRings : kSelectorRings;
	int step = 0;
	auto waitUntil = [this, start](uint32 cycles) {
		const uint32 deadline = ringMillis(cycles);
		while (!shouldQuit() && millis() - start < deadline) {
			pumpFrames();
			delay(1);
		}
	};
	queueDraw(kBackup);
	while (_pending != kNoJob && !shouldQuit()) waitFrame();
	for (int radius = 2, increment = 1; radius <= 128 && !shouldQuit(); increment += 2) {
		waitUntil(timings[step].begin);
		Startup::ring(_renderer.screen, _renderer.backup, radius, false);
		waitUntil(timings[step].end);
		Startup::ring(_renderer.screen, _renderer.backup, radius, true);
		pumpFrames();
		++step;
		radius += increment;
	}
	debug(1, "cblood: %s ring completed in %u ms", planetEntry ? "planet" : "selector", millis() - start);
}

void ScreenController::waitForDraw() {
	while (_pending != kNoJob && !shouldQuit()) waitFrame();
}
void ScreenController::executeDraw() {
	DrawJob job = _pending;
	_pending = kNoJob;
	if (job == kPlanet) {
		_planet.render();
	} else if (job == kMap) {
		_map.render();
	} else {
		_panel.draw(job, _pendingButton);
		if (job == kOorxx) ++_panel.revealFrame;
	}
}
void ScreenController::tickFrame() {
	if (_map.ownsFrames()) return;
	if (!_planet.paletteFrame()) _panel.tickClock();
	if (_startup.introActive()) {
		_startup.tickFrame();
	} else {
		drawHandFrame();
		return;
	}
	_renderer.present();
}
void ScreenController::drawHandFrame() {
	if (_panel.handActive) _panel.restoreHand();
	executeDraw();
	if (_panel.handActive) {
		_panel.updateHand();
		_panel.drawHand();
	}
	_renderer.present();
}

} // namespace CBlood
