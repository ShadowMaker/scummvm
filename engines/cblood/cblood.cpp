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
#include "cblood/screen_controller.h"
#include "engines/util.h"
namespace CBlood {
CBloodEngine::CBloodEngine(OSystem *syst, const ADGameDescription *desc)
	: Engine(syst), _sound(*_mixer, _resources), _renderer(*syst),
	_screens(new ScreenController(*this, *syst, *_eventMan, _state, _resources, _renderer, _sound)) {}
CBloodEngine::~CBloodEngine() {}
bool CBloodEngine::hasFeature(EngineFeature f) const {
	return f == kSupportsReturnToLauncher;
}
Common::Error CBloodEngine::run() {
	initGraphics(320, 200);
	if (!_screens->runStartup())
		return shouldQuit() ? Common::kNoError : Common::kReadingFailed;
	while (!shouldQuit()) {
		_screens->handleInput();
		_screens->waitFrame();
	}
	return Common::kNoError;
}

} // namespace CBlood
