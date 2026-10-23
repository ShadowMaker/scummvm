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
	destroyedCount(0), destroyWriteOffset(0), oorxxVisitScan(0),
	currentPlanet(0), alienOffset(0), alienState(0), bloodDeadline(12000),
	localSeed(0), gameStartMillis(0) {
	memset(destroyedCoords, 0, sizeof(destroyedCoords));
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

bool GameState::destroyPlanet() {
	if (dialogue.size() < 0x1770) return false;
	if (int16(alienOffset) >= 0 && (alienOffset > 252 || (alienOffset & 3))) return false;
	if (int16(alienOffset) < 0 && destroyWriteOffset > 254) return false;
	byte *bits = &dialogue[0x1730];
	auto clearBit = [bits](uint bit) { bits[bit / 8] &= ~(1 << (bit & 7)); };
	++destroyedCount;
	if (destroyedCount <= 10) clearBit(39 + destroyedCount);
	if (int16(alienOffset) < 0) {
		WRITE_BE_UINT16(destroyedCoords + destroyWriteOffset, currentPlanet | 0x8000);
		// Native code writes the byte-wrapped next offset into the cache, not its cursor.
		uint16 next = (destroyWriteOffset & 0xff00) | byte(destroyWriteOffset + 2);
		WRITE_BE_UINT16(destroyedCoords + 128, next);
	} else {
		alienPlanets[alienOffset] |= 0x80;
		uint firstBit = 100 + (alienOffset / 4) * 5;
		alienState = 0;
		for (uint field = 0; field < 5; ++field) {
			uint bit = firstBit + field;
			alienState |= ((bits[bit / 8] >> (bit & 7)) & 1) << field;
		}
		clearBit(firstBit);
		clearBit(firstBit + 1);
		clearBit(firstBit + 4);
		alienOffset = 0xffff;
	}
	return true;
}

} // namespace CBlood
