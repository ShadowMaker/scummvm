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
#include "cblood/game_state.h"
#include "cblood/planet.h"
#include "common/str.h"
namespace CBlood {
GameState::GameState() : language(Startup::kNoLanguage), planetRandom("cblood-planets"),
	currentPlanet(0), alienOffset(0), alienState(0), bloodDeadline(12000),
	localSeed(0), gameStartMillis(0) {
	memset(alienPlanets, 0, sizeof(alienPlanets));
}
uint16 GameState::gameClock(uint32 playTime) const {
	return Planet::clockValue(playTime - gameStartMillis);
}
bool GameState::initCoordinates() {
	return Planet::initCoordinates(alienPlanets, alienOffset,
		[this]() { return planetRandom.getRandomNumber(0xffffff); });
}

} // namespace CBlood
