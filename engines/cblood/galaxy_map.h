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
#ifndef CBLOOD_GALAXY_MAP_H
#define CBLOOD_GALAXY_MAP_H

#include "common/endian.h"
#include "cblood/input.h"

namespace CBlood {

struct GalaxyMap {
	enum Axis { kNoReadout, kPreview, kXAxis, kYAxis };
	struct Selection {
		Axis axis;
		uint16 x, y;
	};
	uint16 mask = 0, column = 0, row = 0;

	static Selection keyboard(uint16 selected, uint action, uint16 tremor) {
		uint16 x = byte(selected), y = byte(selected >> 8);
		if (tremor) return {kNoReadout, x, y};
		switch (action) {
		case kMapUp: return {kYAxis, x, uint16((y + 1) & 127)};
		case kMapDown: return {kYAxis, x, uint16((y - 1) & 127)};
		case kMapLeft: return {kXAxis, uint16((x - 1) & 255), y};
		case kMapRight: return {kXAxis, uint16((x + 1) & 255), y};
		default: return {kNoReadout, x, y};
		}
	}

	static uint16 selected(uint16 previous, const Selection &change) {
		if (change.axis == kXAxis) return (previous & 0xff00) | byte(change.x);
		if (change.axis == kYAxis) return (previous & 0xff) | (uint16(byte(change.y)) << 8);
		return previous;
	}

	static Selection pointer(uint16 selected, uint16 handX, uint16 dispatchY, byte held) {
		uint16 x = handX - 30;
		if (int16(x) < 0) x = 0;
		if (x > 255) x = 255;
		uint16 y = dispatchY - 54;
		if (int16(y) < 0) return {kNoReadout, x, y};
		if (held & 2) {
			byte dx = byte(selected) - byte(x);
			byte dy = byte(selected >> 8) - byte(y);
			if (dx & 128) dx = byte(-dx);
			if (dy & 128) dy = byte(-dy);
			if (dx < dy) return {kXAxis, x, uint16((y & 0xff00) | byte(selected >> 8))};
			if (dx > dy) return {kYAxis, byte(selected), y};
		}
		return {kPreview, x, y};
	}

	static void paint(byte *dest, uint16 bits) {
		for (uint plane = 0; plane < 3; ++plane)
			WRITE_BE_UINT16(dest + plane * 2, READ_BE_UINT16(dest + plane * 2) & ~bits);
		WRITE_BE_UINT16(dest + 6, READ_BE_UINT16(dest + 6) | bits);
	}

	void vertical(byte *screen, const byte *backing, byte x) {
		for (uint y = 0; y < 127; ++y)
			memcpy(screen + 0xb50 + column + y * 160, backing + 16 + column + y * 160, 8);
		paint(screen + 0xb50 + column + row, 0xffff);
		column = (x & 0xf0) >> 1;
		mask = 0x8000 >> (x & 15);
		for (uint y = 0; y < 127; ++y) paint(screen + 0xb50 + column + y * 160, mask);
	}

	void horizontal(byte *screen, const byte *backing, byte y) {
		memcpy(screen + 0xb40 + row, backing + row, 160);
		paint(screen + 0xb50 + row + column, mask);
		row = (127 - y) * 160;
		for (uint group = 0; group < 20; ++group) paint(screen + 0xb40 + row + group * 8, 0xffff);
	}
};

} // namespace CBlood
#endif
