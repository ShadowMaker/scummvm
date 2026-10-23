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
#ifndef CBLOOD_GAME_STATE_H
#define CBLOOD_GAME_STATE_H
#include "common/array.h"
#include "common/random.h"
#include "cblood/startup.h"
#include "cblood/hand_motion.h"
namespace CBlood {
struct GameState {
	GameState();
	uint16 gameClock(uint32 playTime) const;
	bool initCoordinates();
	bool destroyPlanet();
	void setProtectionFailure(bool enabled);
	bool protectionFailure;
	uint16 forceHandTremor;
	HandMotion handMotion;
	Common::Array<byte> dialogue;
	Startup::Language language;
	Common::RandomSource planetRandom;
	byte alienPlanets[256];
	byte destroyedCoords[256];
	uint16 destroyedCount, destroyWriteOffset;
	uint16 oorxxVisitScan;
	uint16 currentPlanet, alienOffset, alienState, bloodDeadline;
	uint32 localSeed, gameStartMillis;
};
} // namespace CBlood
#endif
