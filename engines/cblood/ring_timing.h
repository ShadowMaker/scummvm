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

#ifndef CBLOOD_RING_TIMING_H
#define CBLOOD_RING_TIMING_H

#include "common/scummsys.h"

namespace CBlood {

struct RingTiming {
	uint32 begin;
	uint32 end;
};

static const RingTiming kSelectorRings[12] = {
	{354960, 364220}, {364312, 380124}, {380216, 407396},
	{407488, 506744}, {506836, 639460}, {639552, 817424},
	{817516, 1042736}, {1042828, 1438692}, {1438784, 1845764},
	{1845856, 2437884}, {2437976, 3123644}, {3123736, 4008052}
};

static const RingTiming kPlanetRings[12] = {
	{505008, 514268}, {514360, 528772}, {528864, 556044},
	{556136, 657264}, {657356, 789980}, {790072, 968036},
	{968128, 1191476}, {1191568, 1588756}, {1588848, 1994900},
	{1994992, 2587952}, {2588044, 3272772}, {3272864, 4158096}
};

inline uint32 ringMillis(uint32 cycles) {
	return (uint64(cycles) * 1000 + 8021246) / 8021247;
}

} // namespace CBlood

#endif
