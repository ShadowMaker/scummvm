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
#ifndef CBLOOD_PLANET_SCREEN_H
#define CBLOOD_PLANET_SCREEN_H
#include "cblood/screen.h"
#include "cblood/planet.h"
namespace CBlood {
class PlanetScreen : public Screen {
public:
	PlanetScreen(ScreenController &host, GameState &state, Resources &resources,
		Renderer &renderer, CockpitPanel &panel, Sound &sound);
	void initData();
	void setPlanet(uint16 coordinate);
	bool open() override;
	void close() override { _planetActive = false; }
	void update(uint32 now) override;
	void render() override;
private:
	byte _planetState[124], _planetColors[64];
	byte _terrain[0x4145], _packedPlanet[4944], _depthPoints[3072], _depthState[28];
	uint _renderPoint, _planetFrames;
	bool _planetActive;
	Planet::RenderPacer _planetPacer;
};
} // namespace CBlood
#endif
