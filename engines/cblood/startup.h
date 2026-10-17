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

#ifndef CBLOOD_STARTUP_H
#define CBLOOD_STARTUP_H

#include "common/scummsys.h"
#include "common/endian.h"

namespace CBlood {
namespace Startup {

struct Intro {
	enum Phase { kHold, kStartFade, kFade } phase;
	byte selector;
	int16 hold;
	uint16 toggle;
	int level, step;
	bool outgoing;
	Intro() : phase(kHold), selector(1), hold(0), toggle(0), level(0), step(1), outgoing(false) {}
	void request(byte image) { selector = image; hold = 0; }
	// Bits: dispatch deferred work, select image, write palette.
	int tick() {
		if (phase == kHold) {
			hold = int16(uint16(hold) - 1);
			if (hold >= 0) return 1;
			hold = 1500;
			selector ^= 1;
			phase = kStartFade;
			return 3;
		}
		if (phase == kStartFade) {
			level = 0; step = 1; outgoing = true; phase = kFade;
			return 0;
		}
		toggle ^= 1;
		return toggle ? 4 : 0;
	}
	bool advanceFade() {
		level += step;
		if (level >= 0 && level <= 7) return false;
		if (outgoing) {
			outgoing = false; level = 7; step = -1;
			return true;
		}
		phase = kHold;
		return false;
	}
};

inline uint16 fadeColor(uint16 color, int level) {
	uint16 result = 0;
	for (int shift = 0; shift <= 8; shift += 4) {
		int channel = ((color >> shift) & 7) - level;
		if (channel > 0) result |= channel << shift;
	}
	return result;
}

template<class Writer>
void visitRing(int size, Writer writer) {
	int x = 0, y = size - 1;
	if (!y) return;
	do {
		int distance = size * size - y * y;
		do {
			if (x <= 125 && y <= 160) {
				for (int swap = 0; swap < (y <= 125 ? 2 : 1); ++swap) {
					int horizontal = swap ? x : y;
					int vertical = (swap ? y : x) >> 1;
					writer(160 + horizontal, 81 + vertical);
					writer(159 - horizontal, 81 + vertical);
					writer(159 - horizontal, 81 - vertical);
					writer(160 + horizontal, 81 - vertical);
				}
			}
			++x;
		} while (x * x < distance);
		--x; --y;
	} while (y > x);
}

inline void ring(byte *screen, const byte *backup, int size, bool restore) {
	visitRing(size, [=](int x, int y) {
		int offset = y * 160 + (x / 16) * 8;
		if (restore) memcpy(screen + offset, backup + offset, 8);
		else for (int plane = 0; plane < 4; ++plane) {
			byte *word = screen + offset + plane * 2;
			WRITE_BE_UINT16(word, READ_BE_UINT16(word) | (0x8000 >> (x & 15)));
		}
	});
}

template<class Publisher>
void buttons(uint16 oldMask, uint16 requested, Publisher publish) {
	uint16 changed = oldMask ^ requested;
	for (int pass = 0; pass < 2; ++pass) {
		uint16 mask = changed & (pass ? requested : oldMask);
		for (int index = 9; index >= 0; --index) {
			if (!(mask & (1 << index))) continue;
			for (int step = 0; step < (pass ? 7 : 5); ++step)
				publish(index, bool((step & 1) == pass));
		}
	}
}

enum Language { kNoLanguage = -1, kFrench, kEnglish, kGerman };

inline Language hitLanguage(int x, int y) {
	const int hitX = (x + 4) >> 1;
	if (y < 0x55 || y > 0x78)
		return kNoLanguage;
	if (hitX >= 0x28 && hitX <= 0x41)
		return kFrench;
	if (hitX >= 0x44 && hitX <= 0x5c)
		return kEnglish;
	if (hitX >= 0x5f && hitX <= 0x78)
		return kGerman;
	return kNoLanguage;
}

inline void selectLanguage(byte *dialogue, Language language) {
	if (language == kFrench)
		memcpy(dialogue + 0xf30, dialogue + 0x790, 1920);
	else if (language == kGerman)
		memcpy(dialogue + 0xf30, dialogue, 1920);
	WRITE_BE_UINT32(dialogue + 0x16c0, 0xc02f0000);
}

inline void drawFlags(byte *screen, const byte *source) {
	for (int row = 0; row < 32; ++row) {
		byte *dst = screen + 0x3548 + row * 160;
		for (int group = 0; group < 10; ++group, dst += 8, source += 6) {
			memcpy(dst, source, 6);
			WRITE_BE_UINT16(dst + 6, 0);
		}
	}
}

inline void restoreHand(byte *screen, const byte *saved, int destination, int16 height) {
	int16 counter = int16(uint16(height) + 1);
	if (counter < 0) return;
	for (int row = 0; row <= counter; ++row)
		memcpy(screen + destination + row * 160, saved + row * 24, 24);
}

inline void revealOorxx(byte *screen, const byte *source, uint16 &frame) {
	++frame;
	if (int16(62 - frame) < 0) return;
	int rows = (frame < 57 ? frame : 57) + 1;
	for (int row = 0; row < rows; ++row) {
		const byte *sprite = source + (62 - frame + row) * 16;
		uint32 keep = ~READ_BE_UINT32(sprite) | READ_BE_UINT32(source + 0x3f0 + row * 4);
		uint32 replace = ~keep;
		const byte *background = source + 0x4e2 + row * 16;
		byte *dest = screen + 0x24a8 + row * 160;
		for (int group = 0; group < 2; ++group) {
			int shift = group ? 0 : 16;
			for (int plane = 0; plane < 3; ++plane) {
				uint16 word = (READ_BE_UINT16(background + group * 8 + plane * 2) & (keep >> shift)) |
					((READ_BE_UINT32(sprite + 4 + plane * 4) >> shift) & (replace >> shift));
				WRITE_BE_UINT16(dest + group * 8 + plane * 2, word);
			}
			WRITE_BE_UINT16(dest + group * 8 + 6,
				READ_BE_UINT16(background + group * 8 + 6) | (replace >> shift));
		}
	}
}

// Screen buffers need 32768 bytes because the hand extends below the visible rows.
inline void mergeRows(byte *screen, byte *saved, const byte *source,
		int destination, int rows, int shift) {
	for (int row = 0; row < rows; ++row, destination += 160, saved += 24) {
		memcpy(saved, screen + destination, 24);
		const byte *src = source + row * 16;
		const uint32 mask = READ_BE_UINT32(src);
		const uint16 masks[3] = { uint16((mask >> shift) >> 16), uint16(mask >> shift),
			uint16(mask << (16 - shift)) };
		for (int group = 0; group < 3; ++group) {
			byte *dst = screen + destination + group * 8;
			WRITE_BE_UINT16(dst + 6, READ_BE_UINT16(dst + 6) | masks[group]);
			for (int plane = 0; plane < 3; ++plane) {
				const uint32 bits = READ_BE_UINT32(src + 4 + plane * 4);
				const uint16 color = group == 0 ? uint16((bits >> shift) >> 16) :
					group == 1 ? uint16(bits >> shift) : uint16(bits << (16 - shift));
				byte *word = dst + plane * 2;
				WRITE_BE_UINT16(word, (READ_BE_UINT16(word) & ~masks[group]) | color);
			}
		}
	}
}

inline void mergeHand(byte *screen, byte *saved, const byte *bloodM,
		int destination, int height, int shift, int topOffset) {
	int topRows = height <= 15 ? height + 1 : 16;
	mergeRows(screen, saved, bloodM + topOffset, destination, topRows, shift);
	if (height > 15)
		mergeRows(screen, saved + 16 * 24, bloodM + 0x500,
			destination + 16 * 160, height - 14, shift);
}

inline void moveHand(int &x, int &y, int maxY, byte &held, byte &latch,
		int dx, int dy, byte buttons) {
	if (held != buttons) {
		held = buttons;
		latch |= buttons;
	}
	x += dx;
	y += dy;
	if (x < 0) x = 0;
	if (x > 288) x = 288;
	if (y < 0) y = 0;
	if (y > maxY) y = maxY;
}

} // End of namespace Startup
} // End of namespace CBlood

#endif
