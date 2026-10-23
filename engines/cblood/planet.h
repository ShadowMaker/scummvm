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
 */
#ifndef CBLOOD_PLANET_H
#define CBLOOD_PLANET_H
#include "common/endian.h"

namespace CBlood {
namespace Planet {

template<class Random>
bool initCoordinates(byte *records, uint16 &selected, Random next) {
	WRITE_BE_UINT16(records + 252, 0);
	for (int write = 252; write >= 0; write -= 4) {
		uint16 candidate;
		do { candidate = uint16(next()) >> 1; }
		while (byte(candidate) <= 8 || byte(candidate) > 247 || candidate <= 0x1400 || candidate > 0x7800);
		int scan = 252;
		do {
			if (READ_BE_UINT16(records + scan) == candidate) return false;
			scan -= 4;
		} while (scan > write);
		WRITE_BE_UINT16(records + write, candidate);
		WRITE_BE_UINT16(records + write + 2, candidate);
	}
	selected = uint16(next()) & 0x30;
	return true;
}

inline uint16 clockValue(uint32 milliseconds) {
	uint32 seconds = milliseconds / 1000;
	return uint16(((seconds / 3600) % 24) * 6000 +
		((seconds / 60) % 60) * 100 + seconds % 60);
}

inline void formatClock(byte *descriptor, uint16 value) {
	memset(descriptor + 2, 10, 4);
	byte *digit = descriptor + 8;
	do {
		*--digit = value % 10;
		value /= 10;
	} while (value);
}

inline void formatDigits(byte *descriptor, byte value) {
	descriptor[2] = value / 100;
	descriptor[3] = (value / 10) % 10;
	descriptor[4] = value % 10;
}

inline void drawDigits(byte *screen, const byte *glyphs, const byte *descriptor) {
	int offset = int16(READ_BE_UINT16(descriptor));
	int half = offset & 1, base = offset - half;
	for (const byte *digit = descriptor + 2; !(*digit & 128); ++digit) {
		for (int row = 0; row < 9; ++row) {
			byte *pixel = screen + base + row * 160 + half;
			pixel[0] = pixel[2] = 0;
			pixel[4] = pixel[6] = glyphs[*digit * 9 + row];
		}
		half ^= 1;
		if (!half) base += 8;
	}
}

inline uint32 swapWords(uint32 value) { return (value << 16) | (value >> 16); }

inline int32 nextRandom(uint32 &seed) {
	uint32 low = seed & 65535, high = seed >> 16;
	seed = low * 0xe62d + swapWords(high * 0xe62d) + swapWords(low * 0xbb40) + 1;
	return int32(seed) >> 2;
}

inline uint16 initPoints(byte *records, uint32 &seed) {
	uint16 count = (uint16(nextRandom(seed)) >> 8) | 0x40;
	for (int i = 0; i <= count; ++i) {
		byte *record = records + i * 12;
		WRITE_BE_UINT32(record, 0);
		WRITE_BE_UINT16(record + 4, nextRandom(seed));
		WRITE_BE_UINT16(record + 6, int16(nextRandom(seed)) >> 2);
		WRITE_BE_UINT16(record + 8, nextRandom(seed) & 0xfff);
	}
	return count;
}

inline int16 shiftPoint(int16 value, uint count) {
	count &= 63;
	return value >> (count < 16 ? count : 15);
}

inline uint16 projectPoints(byte *screen, byte *records, byte *context, uint32 &seed,
	uint32 reseedShifts = 2) {
	uint16 lastWord = 0;
	uint16 count = READ_BE_UINT16(context + 26), step = READ_BE_UINT16(context + 22);
	for (int i = 0; i <= count; ++i) {
		byte *record = records + i * 12;
		uint16 mask = READ_BE_UINT16(record);
		byte *old = screen + int16(READ_BE_UINT16(record + 2));
		for (int plane = 0; plane < 4; ++plane)
			WRITE_BE_UINT16(old + plane * 2, READ_BE_UINT16(old + plane * 2) & ~mask);
		uint16 depth = READ_BE_UINT16(record + 8);
		if (depth & 0x8000) {
			WRITE_BE_UINT16(record + 4, shiftPoint(int16(nextRandom(seed)), reseedShifts >> 16));
			WRITE_BE_UINT16(record + 6, shiftPoint(int16(nextRandom(seed)), reseedShifts));
			lastWord = uint16(((seed & 65535) * 0xbb40) >> 16);
			WRITE_BE_UINT16(record + 8, (nextRandom(seed) & 0xfff) | 0x1800);
			continue;
		}
		WRITE_BE_UINT16(record + 8, depth + step);
		byte color = byte(~(depth >> 6));
		context[3] = color < 0x90 ? 0x90 : color;
		int16 scale = int16(0xfffff / (depth + 16));
		int16 x = int16((int16(READ_BE_UINT16(record + 4)) * int32(scale) >> 16) + 160);
		lastWord = uint16(x);
		int16 y = int16((int16(READ_BE_UINT16(record + 6)) * int32(scale) >> 16) + 80);
		if (x < 0 || x >= 319 || y <= 18 || y > 143) {
			WRITE_BE_UINT16(record + 8, 0);
			continue;
		}
		int offset = y * 160 + ((x & 0xfff0) >> 1);
		mask = 0x8000 >> (x & 15);
		for (int plane = 0; plane < 4; ++plane) {
			byte *pixel = screen + offset + plane * 2;
			uint16 word = READ_BE_UINT16(pixel) & ~mask;
			if (context[3] & (0x10 << plane)) word |= mask;
			WRITE_BE_UINT16(pixel, word);
		}
		WRITE_BE_UINT16(record, mask);
		WRITE_BE_UINT16(record + 2, offset);
	}
	return lastWord;
}

template<class Checkpoint>
void terrain(byte *memory, byte *state, uint16 coordinates, uint32 &seed,
             uint16 &palette, Checkpoint checkpoint, int variationMode = 0) {
	byte *field = memory + 4;
	WRITE_BE_UINT16(state + 0x1a, coordinates);
	seed = READ_BE_UINT32(state + 0x18);
	uint16 draws[6];
	for (int i = 0; i < 6; ++i) draws[i] = nextRandom(seed);
	uint16 params[6] = {uint16(draws[5] & 3), uint16(draws[4] & 28),
		uint16(draws[3] & 28), draws[2], draws[1], uint16(draws[0] & 31)};
	for (int i = 0; i < 6; ++i) WRITE_BE_UINT16(state + 4 + 2 * i, params[i]);
	WRITE_BE_UINT16(state + 0x20, params[5] << 8);
	palette = draws[0] & 511;
	uint16 warp = 0;
	if ((draws[1] & 255) <= 32) {
		warp = draws[1] & 0x8007;
		if (warp & 0x8000) warp = (warp & 0xff00) | ((-warp) & 255);
	}
	WRITE_BE_UINT16(state + 0x22, warp);
	int level = params[0], initialX = 1 << level, initialY = 4 << level;
	int initialDx = 64 >> level, initialDy = 4096 >> level;
	for (int row = 0; row <= initialY; ++row)
		for (int column = 0; column <= initialX; ++column)
			field[row * initialDy + column * initialDx] = uint16(nextRandom(seed)) >> 6;
	checkpoint(0);
	uint16 midpointWord = 0;
	auto midpoint = [&](int offset, int stride, int shift, bool clearWord) {
		uint16 high = (seed >> 16) * coordinates + 1;
		seed = (uint32(high) << 16) | (seed & 65535);
		int variation = int8(high) >> (variationMode == 1 ? 2 : shift);
		byte average = (int(field[offset]) + field[offset + stride]) >> 1;
		midpointWord = (clearWord ? 0 : midpointWord & 0xff00) | average;
		if (variationMode == 2) {
			midpointWord = (midpointWord & 0xff00) | ((midpointWord - (high & 7)) & 255);
			variation = 0;
		}
		midpointWord += variation;
		if (int16(midpointWord) < 0) midpointWord = 0;
		if (midpointWord > 255) midpointWord = 65535;
		field[offset + stride / 2] = midpointWord;
	};
	auto vertical = [&](int start, int rows, int cells, int dx, int dy, int shift) {
		for (int row = 0; row < rows; ++row)
			for (int column = 0; column < cells; ++column)
				midpoint(start + row * dy + column * dx, dx, shift, true);
	};
	auto horizontal = [&](int start, int columns, int cells, int dx, int dy, int shift) {
		for (int column = 0; column < columns; ++column)
			for (int row = 0; row < cells; ++row)
				midpoint(start + column * dx + row * dy, dy, shift, false);
	};
	int x = initialX, dx = initialDx, shift = 0;
	do {
		field[64] = field[0];
		WRITE_BE_UINT16(memory, x);
		vertical(0, 1, x, dx, initialDy, shift++);
		x *= 2; dx /= 2;
	} while (dx != 1);
	memcpy(field + 0x4000, field, 64);
	int y = initialY, dy = initialDy;
	shift = 0;
	do {
		WRITE_BE_UINT16(memory + 2, y);
		horizontal(0, 1, y, initialDx, dy, shift++);
		y *= 2; dy /= 2;
	} while (dy != 64);
	checkpoint(1);
	x = initialX; y = initialY; dx = initialDx; dy = initialDy; shift = 0;
	do {
		WRITE_BE_UINT16(memory, x); WRITE_BE_UINT16(memory + 2, y);
		vertical(dy, y - 1, x, dx, dy, shift);
		dx /= 2; x *= 2;
		WRITE_BE_UINT16(memory, x); WRITE_BE_UINT16(memory + 2, y);
		horizontal(dx, x - 1, y, dx, dy, ++shift);
		y *= 2; dy /= 2;
	} while (dx != 1);
	checkpoint(2);
	WRITE_BE_UINT32(state + 0x14, READ_BE_UINT32(state + 0x14) - 1);
	if (params[4] & 0x1000) {
		bool reverse = params[4] & 0x800;
		int pointer = reverse ? 0x4040 : 0, displacement = reverse ? -2 : 1;
		for (int group = 0; group < 64; ++group) {
			for (int i = 0; i < 256; ++i) {
				byte value = field[pointer + displacement];
				if (reverse) --pointer;
				field[pointer] = value;
				if (!reverse) ++pointer;
			}
			displacement += reverse ? -1 : 1;
		}
	}
	checkpoint(3);
	if (warp) {
		byte index = 0;
		for (int column = 1; column < 64; ++column) {
			for (int row = 0; row < 256; ++row) field[0x4041 + row] = field[column + row * 64];
			index += warp;
			for (int row = 0; row < 256; ++row) field[column + row * 64] = field[0x4041 + index++];
		}
	}
	checkpoint(4);
}

inline void palettes(byte *destination, const byte *source, uint16 selector) {
	int offset = selector > 422 ? selector - 256 : selector;
	int base = (offset & ~1) + 2;
	for (int i = 0; i < 13; ++i) {
		int from = base + ((offset & 1) ? 22 - 2 * i : 2 * i);
		memcpy(destination + 6 + 2 * i, source + from, 2);
	}
	for (int i = 0; i < 7; ++i) {
		int from = base + ((offset & 1) ? 22 - 4 * i : 4 * i);
		memcpy(destination + 34 + 2 * i, source + from, 2);
	}
}

inline void curve(byte *state) {
	uint16 span = READ_BE_UINT16(state);
	int height = 0, cursor = 8;
	for (int x = span - 1; x >= 1; --x) {
		state[cursor++] = (height * 0x1400) >> 12;
		int difference = span * span - x * x;
		do { ++height; } while (height * height < difference);
		--height;
	}
	state[cursor] = state[cursor - 1];
	WRITE_BE_UINT16(state, span - 2);
}

inline void initBuffer(byte *packed, const byte *screen) {
	for (int row = 0; row < 77; ++row) memcpy(packed + 8 + row * 64, screen + 0x1f60 + row * 160, 64);
}

inline void blit(byte *screen, const byte *packed) {
	for (int row = 0; row < 77; ++row) memcpy(screen + 0x1f68 + row * 160, packed + 16 + row * 64, 48);
}

template<class PointHook>
void renderRange(byte *packed, const byte *field, const byte *state,
                 uint first, uint end, PointHook pointHook) {
	uint16 span = READ_BE_UINT16(state), phase = READ_BE_UINT16(state + 2);
	auto point = [&](int x, int y, uint16 column) {
		pointHook(x, y, column);
		int destination = 0x9a8 + int16(y * 64 + (int16(x & 0xfff0) >> 1));
		uint16 mask = 0x8000 >> (x & 15);
		int foldedY = int8(y) + 32;
		if (foldedY < 0) foldedY = -foldedY;
		else if (foldedY > 62) foldedY = 124 - foldedY;
		byte color = field[1 + int16((column << 6) + foldedY)] >> 5;
		if (!color) color = 1;
		for (int plane = 0; plane < 4; ++plane) {
			byte *pixel = packed + destination + plane * 2;
			uint16 word = READ_BE_UINT16(pixel) & ~mask;
			if (color & (1 << plane)) word |= mask;
			WRITE_BE_UINT16(pixel, word);
		}
	};
	for (uint index = first; index < end; ++index) {
		uint band = index / (4 * (span + 1));
		uint row = (index / 4) % (span + 1), quadrant = index & 3;
		int scale = 0xfaca - band * 0x556;
		int x = (state[9 + row] * scale) >> 16, y = span - row;
		uint16 column = (phase & 0xff00) | (quadrant < 2 ?
			byte(phase - state[48 + band]) : byte(phase + state[48 + band]));
		point(quadrant < 2 ? -x : x, quadrant == 1 || quadrant == 2 ? -y : y, column);
	}
}

template<class PointHook>
void render(byte *packed, const byte *field, const byte *state, PointHook pointHook) {
	renderRange(packed, field, state, 0, 48 * (READ_BE_UINT16(state) + 1) * 4, pointHook);
}

struct RenderPacer {
	uint64 credit;
	uint32 lastMillis;
	RenderPacer() : credit(0), lastMillis(0) {}
	void reset(uint32 now) { credit = 0; lastMillis = now; }
	uint advance(uint32 now, bool speech) {
		if (int32(now - lastMillis) < 0) return 0;
		uint cost = speech ? 117 : 75;
		credit += uint64(uint32(now - lastMillis)) * 1000;
		lastMillis = now;
		uint points = uint(credit / cost);
		credit %= cost;
		return points;
	}
};
} // namespace Planet
} // namespace CBlood
#endif
