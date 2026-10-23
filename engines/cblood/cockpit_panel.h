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
#ifndef CBLOOD_COCKPIT_PANEL_H
#define CBLOOD_COCKPIT_PANEL_H
#include "cblood/screen.h"
namespace CBlood {
class CockpitPanel {
public:
	CockpitPanel(ScreenController &host, GameState &state, Resources &resources, Renderer &renderer);
	void initData();
	void resetClock();
	void tickClock();
	void pollGameClock();
	void handleEvent(const Common::Event &event);
	void updateButtons(uint16 requested);
	void restoreHand();
	void updateHand();
	void drawHand();
	void draw(DrawJob job, int button);
	bool handActive;
	uint16 buttonMask;
	int cursorX, cursorY, cursorYClamp;
	byte pointerButton, pointerButtonLatch;
	uint16 revealFrame;
	byte digits[36];
private:
	ScreenController &_host;
	GameState &_state;
	Resources &_resources;
	Renderer &_renderer;
	uint16 handAnimation;
	int oldHandHeight, oldHandDest, _handTop;
	int _mouseYFraction = 0;
	byte savedHand[2664], clockDigits[10];
	uint16 shownClock, clockReply;
	bool clockReplyReady;
	int clockPollTicks;
};
} // namespace CBlood
#endif
