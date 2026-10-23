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
#ifndef CBLOOD_DESTRUCTION_H
#define CBLOOD_DESTRUCTION_H

#include "common/endian.h"

namespace CBlood {

struct PaletteRaster {
	uint16 toggle, countdown, step, offset;

	void tick() {
		toggle ^= 1;
		if (toggle) {
			--countdown;
			offset = (offset + step) & 0x3fe;
		}
	}

	template<class Emit>
	void bands(const byte *delays, Emit emit) const {
		uint16 forward = offset, reverse = offset + 40;
		int row = 18, pendingDelay = 1;
		bool descending = false;
		for (uint cursor = 0; cursor < 40; ++cursor) {
			int delay = delays[cursor];
			if (!descending && !delay) {
				descending = true;
				delay = 1;
				emit(row, reverse);
				reverse = (reverse - 2) & 0x3fe;
			} else if (descending) {
				if (!delay) { emit(row, -1); break; }
				emit(row, reverse);
				reverse = (reverse - 2) & 0x3fe;
			} else {
				emit(row, forward);
				forward = (forward + 2) & 0x3fe;
			}
			// Timer B has already reloaded before its handler writes the next delay.
			row += pendingDelay;
			pendingDelay = delay;
		}
	}
};

struct PaletteSequence {
	uint16 channels[3];
	uint used;

	bool build(const byte *script, uint length, byte *output, uint capacity) {
		memset(channels, 0, sizeof(channels));
		used = 0;
		for (uint cursor = 0; cursor + 2 <= length;) {
			int16 color = int16(READ_BE_UINT16(script + cursor));
			cursor += 2;
			if (color < 0) {
				int gap = int16(-color) - int(used);
				if (gap >= 0) {
					uint bytes = ((uint16(gap) >> 1) + 1) * 2;
					if (bytes > capacity - used) return false;
					memset(output + used, 0, bytes);
					used += bytes;
				}
				continue;
			}
			if (cursor + 2 > length) return false;
			int16 count = int16(READ_BE_UINT16(script + cursor));
			cursor += 2;
			if (count < 0) return true;
			if (!count || uint(count) * 2 > capacity - used) return false;
			int16 steps[3];
			for (int channel = 0; channel < 3; ++channel) {
				uint16 target = (color << (channel * 4)) & 0x700;
				steps[channel] = int16(target - channels[channel]) / count;
			}
			for (int n = 0; n < count; ++n) {
				for (int channel = 0; channel < 3; ++channel)
					channels[channel] = uint16(channels[channel] + steps[channel]);
				uint16 packed = (channels[0] & 0xff00) |
					((channels[1] >> 8) << 4) | (channels[2] >> 8);
				WRITE_BE_UINT16(output + used, packed);
				used += 2;
			}
		}
		return false;
	}
};

class DestructImage {
public:
	enum { kFieldBytes = 2308, kPatternBytes = 222, kImageBytes = 19840 };

	DestructImage(byte *field, byte *pattern, uint16 seed, uint16 factor,
		uint16 midpoint) : _field(field), _pattern(pattern), _seed(seed),
		_factor(factor), _midpoint(midpoint), _column(0) {}

	void begin() {
		memset(_field + 2052, 0, 128);
		uint16 initial = READ_BE_UINT16(_pattern);
		_pattern[14] = _pattern[110] = _pattern[206] = byte(initial);
		_pattern[106] = _pattern[114] = byte(initial >> 1);
		refine(_pattern + 2, 5, 2, 4, 96, 1);
		WRITE_BE_UINT32(_pattern + 2, 0);
		_column = 0;
	}

	bool drawColumn(byte *output) {
		if (_column == 20) return false;
		memcpy(_field + 4, _field + 2052, 128);
		uint16 phase = READ_BE_UINT16(_pattern + 4);
		WRITE_BE_UINT16(_pattern + 4, phase + 1);
		byte sample = 0;
		for (int n = 0; n < 9; ++n) {
			sample = _pattern[6 + int16(phase + 24 * n)];
			_field[2180 - 16 * n] = sample;
		}
		for (int n = 0; n < 16; ++n)
			_field[2052 - 128 * n] = sample;
		WRITE_BE_UINT16(_field + 2048, 8);
		for (int dx = 16, shift = 1; dx != 1; dx >>= 1, ++shift) {
			vertical(_field + 2052, 1, _field + 2048, dx, 0, shift);
			WRITE_BE_UINT16(_field + 2048, READ_BE_UINT16(_field + 2048) << 1);
		}
		uint16 shift = refine(_field, 8, 1, 16, 2048, 1);
		uint16 planes[4] = {1, _midpoint, shift, 0xffff};
		byte bias = 0;
		for (int row = 0; row < 124; ++row) {
			planes[0] = 1;
			for (int column = 0; column < 16; ++column) {
				bias ^= 8;
				sample = byte(_field[8 + row + 128 * column] + bias);
				for (int plane = 0; plane < 4; ++plane)
					planes[plane] = uint16((planes[plane] << 1) | ((sample >> (4 + plane)) & 1));
			}
			for (int plane = 0; plane < 4; ++plane)
				WRITE_BE_UINT16(output + _column * 8 + row * 160 + plane * 2, planes[plane]);
			bias ^= 8;
		}
		// Packing leaves D1 as plane one for the next refinement pass.
		_midpoint = planes[1];
		++_column;
		return true;
	}

	uint16 seed() const { return _seed; }

private:
	void cell(byte *start, uint16 stride, uint shift, bool clearHigh) {
		uint16 average = (start[0] + start[int16(stride)]) >> 1;
		_midpoint = (clearHigh ? 0 : _midpoint & 0xff00) | average;
		_seed = uint16(_seed * uint32(_factor) + 1);
		_midpoint = uint16(_midpoint + (int8(_seed) >> shift));
		if (_midpoint & 0x8000) _midpoint = 0;
		if (_midpoint > 255) _midpoint = 0xffff;
		start[stride >> 1] = byte(_midpoint);
	}

	void vertical(byte *base, uint rows, const byte *counts, uint16 dx,
		uint16 dy, uint shift) {
		uint columns = READ_BE_UINT16(counts);
		if (!columns) columns = 65536;
		for (uint row = 0; row < rows; ++row)
			for (uint column = 0; column < columns; ++column)
				cell(base + int(row) * int16(dy) + int(column) * int16(dx), dx, shift, true);
	}

	void horizontal(byte *base, uint columns, const byte *counts, uint16 dx,
		uint16 dy, uint shift) {
		uint rows = READ_BE_UINT16(counts + 2);
		if (!rows) rows = 65536;
		for (uint column = 0; column < columns; ++column)
			for (uint row = 0; row < rows; ++row)
				cell(base + int(column) * int16(dx) + int(row) * int16(dy), dy, shift, false);
	}

	uint16 refine(byte *counts, uint16 columns, uint16 rows,
		uint16 dx, uint16 dy, uint16 shift) {
		WRITE_BE_UINT16(counts, columns);
		WRITE_BE_UINT16(counts + 2, rows);
		do {
			horizontal(counts + 4, READ_BE_UINT16(counts), counts, dx, dy, shift);
			dy >>= 1;
			WRITE_BE_UINT16(counts + 2, READ_BE_UINT16(counts + 2) << 1);
			vertical(counts + 4 + int16(dy), uint16(READ_BE_UINT16(counts + 2) - 2) + 1,
				counts, dx, dy, shift);
			dx >>= 1;
			WRITE_BE_UINT16(counts, READ_BE_UINT16(counts) << 1);
			++shift;
		} while (dx != 1);
		return shift;
	}

	byte *_field;
	byte *_pattern;
	uint16 _seed;
	uint16 _factor;
	uint16 _midpoint;
	uint _column;
};

} // namespace CBlood

#endif
