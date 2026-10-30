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

#include "cblood/cblood.h"
#include "cblood/detection.h"
#include "cblood/input.h"
#include "backends/keymapper/action.h"
#include "backends/keymapper/keymap.h"
#include "backends/keymapper/standard-actions.h"
#include "backends/keymapper/hardware-input.h"
#include "common/translation.h"

#include "base/plugins.h"
#include "engines/advancedDetector.h"

static const ADExtraGuiOptionsMap gameGuiOptions[] = {
	{
		GAMEOPTION_PROTECTION_FAILURE,
		{
			_s("Simulate copy-protection failure"),
			_s("Reproduce the original failure penalties."),
			"simulate_protection_failure",
			false, 0, 0
		}
	},
	{
		GAMEOPTION_BORDER,
		{
			_s("Show Atari ST border"),
			_s("Show the original screen border."),
			"show_border",
			true, 0, 0
		}
	},
	AD_EXTRA_GUI_OPTIONS_TERMINATOR
};

class CBloodMetaEngine : public AdvancedMetaEngine<ADGameDescription> {
public:
	Common::KeymapArray initKeymaps(const char *target) const override;
	const ADExtraGuiOptionsMap *getAdvancedExtraGuiOptions() const override {
		return gameGuiOptions;
	}
	const char *getName() const override {
		return "cblood";
	}

	Common::Error createInstance(OSystem *syst, Engine **engine, const ADGameDescription *desc) const override {
		*engine = new CBlood::CBloodEngine(syst, desc);
		return Common::kNoError;
	}
};

Common::KeymapArray CBloodMetaEngine::initKeymaps(const char *target) const {
	using namespace Common;
	Keymap *mouse = new Keymap(Keymap::kKeymapTypeGame, "cblood", "Captain Blood");
	Action *action = new Action(kStandardActionLeftClick, _("Left click"));
	action->setLeftClickEvent();
	action->addDefaultInputMapping("MOUSE_LEFT");
	action->addDefaultInputMapping("JOY_A");
	mouse->addAction(action);
	action = new Action(kStandardActionRightClick, _("Right click"));
	action->setRightClickEvent();
	action->addDefaultInputMapping("MOUSE_RIGHT");
	action->addDefaultInputMapping("JOY_B");
	mouse->addAction(action);
	Keymap *intro = new Keymap(Keymap::kKeymapTypeGame, "cblood-intro", _("Intro"));
	intro->setEnabled(false);
	action = new Action("CONTINUE", _("Continue"));
	action->setCustomEngineActionEvent(CBlood::kContinue);
	for (const KeyTableEntry *key = defaultKeys; key->hwId; ++key)
		action->addDefaultInputMapping(key->hwId);
	action->addDefaultInputMapping("JOY_X");
	intro->addAction(action);
	Keymap *map = new Keymap(Keymap::kKeymapTypeGame, "cblood-map", _("Galaxy map"));
	map->setEnabled(false);
	const char *ids[] = {kStandardActionMoveUp, kStandardActionMoveDown, kStandardActionMoveLeft, kStandardActionMoveRight};
	const char *labels[] = {_s("Move up"), _s("Move down"), _s("Move left"), _s("Move right")};
	const char *keys[] = {"UP", "DOWN", "LEFT", "RIGHT"};
	for (uint i = 0; i < 4; ++i) {
		action = new Action(ids[i], _(labels[i]));
		action->setCustomEngineActionEvent(CBlood::kMapUp + i);
		action->addDefaultInputMapping(keys[i]);
		map->addAction(action);
	}
	KeymapArray maps = Keymap::arrayOf(mouse);
	maps.push_back(intro);
	maps.push_back(map);
	return maps;
}

#if PLUGIN_ENABLED_DYNAMIC(CBLOOD)
REGISTER_PLUGIN_DYNAMIC(CBLOOD, PLUGIN_TYPE_ENGINE, CBloodMetaEngine);
#else
REGISTER_PLUGIN_STATIC(CBLOOD, PLUGIN_TYPE_ENGINE, CBloodMetaEngine);
#endif
