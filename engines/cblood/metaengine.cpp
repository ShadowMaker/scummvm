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

#if PLUGIN_ENABLED_DYNAMIC(CBLOOD)
REGISTER_PLUGIN_DYNAMIC(CBLOOD, PLUGIN_TYPE_ENGINE, CBloodMetaEngine);
#else
REGISTER_PLUGIN_STATIC(CBLOOD, PLUGIN_TYPE_ENGINE, CBloodMetaEngine);
#endif
