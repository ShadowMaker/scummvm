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
GameState::GameState() : protectionFailure(false), forceHandTremor(0),
	language(Startup::kNoLanguage), planetRandom("cblood-planets"),
	currentPlanet(0), alienOffset(0), alienState(0), bloodDeadline(12000),
	localSeed(0), gameStartMillis(0) {
	memset(alienPlanets, 0, sizeof(alienPlanets));
}
uint16 GameState::gameClock(uint32 playTime) const {
	return Planet::clockValue(playTime - gameStartMillis);
}
void GameState::setProtectionFailure(bool enabled) {
	protectionFailure = enabled;
	forceHandTremor = enabled ? 1 : 0;
	bloodDeadline = enabled ? 0 : 12000;
	// Simulate scan failure without reading protected sectors.
	// TODO: Start the failed-scan countdown at 7; decrement on planet generation, redirect clicks to destruction when its high word is negative
	// TODO: Reject loads with negative diskScanCheck after planet restore, before restoring the remaining 940 bytes
	// TODO: For nonzero diskScanCheck, skip the first save and retry setup. Preserve the native close/error/retry path
	// TODO: Save deadline/forced tremor, but not diskScanCheck
	// TODO: Block map keyboard controls during tremor
	// TODO: Add qualifying disintegration credit to bloodDeadline
	// TODO: Set and later clear forceHandTremor during the ending. Neither gameplay effect depends on protectionFailure
}
bool GameState::initCoordinates() {
	return Planet::initCoordinates(alienPlanets, alienOffset,
		[this]() { return planetRandom.getRandomNumber(0xffffff); });
}

} // namespace CBlood
