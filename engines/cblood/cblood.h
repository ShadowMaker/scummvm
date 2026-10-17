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
#include "engines/engine.h"
#include "cblood/game_state.h"
#include "cblood/resources.h"
#include "cblood/renderer.h"
#include "cblood/sound.h"
#include "common/ptr.h"
struct ADGameDescription;
namespace CBlood {
class ScreenController;
class CBloodEngine : public Engine {
public:
	CBloodEngine(OSystem *syst, const ADGameDescription *desc);
	~CBloodEngine() override;
	Common::Error run() override;
	bool hasFeature(EngineFeature f) const override;
private:
	GameState _state;
	Resources _resources;
	Sound _sound;
	Renderer _renderer;
	Common::ScopedPtr<ScreenController> _screens;
};
} // namespace CBlood
#endif
