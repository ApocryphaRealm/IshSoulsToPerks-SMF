#pragma once

namespace SKSE::log
{
	using level = spdlog::level::level_enum;
}
namespace logger = SKSE::log;

namespace settings
{
	// Reads the INI into the variables below. The values the variables hold when this is
	// called are remembered as the built-in defaults, so RestoreDefaults() can put them back.
	void Init(const std::string& a_iniFileName);

	// Writes every setting below back to the INI that Init() read, leaving the comments and
	// any unrelated keys in that file alone. Returns false if the file could not be written.
	bool Save();

	// Puts every setting back to its built-in default. This only touches the variables;
	// follow it with Save() to persist, and with UI::ApplyLiveSettings() to show it in game.
	void RestoreDefaults();

	// Re-reads the INI that Init() read, discarding any unsaved change made since. Returns
	// false if the file could not be read, leaving the current values alone.
	bool Reload();

	// Full path of the INI Init() read, or an empty string before Init() has run.
	const std::string& GetIniPath();

	namespace debug
	{
		// Ships at trace by default (project standard) so a submitted log carries the detail
		// needed to diagnose a compatibility, timing or stability report without asking the
		// reporter to change anything first - see CLAUDE.md rule 31.
		inline logger::level logLevel = logger::level::trace;
	}

	namespace perks
	{
		// The mod's one real setting: dragon souls required per perk point. The original's
		// GlobalVariable "ishPerkCost" defaulted to 10.0 and was only changeable via
		// "set ishperkcost to X" in the console (or its own MCM page, if that half of the
		// original still worked) - here it is a real slider.
		//
		// DEFAULT 1, design call 2026-08-27: *"I want the mod's default to be 1 not 10."*
		// This DIVERGES from the original's 10 deliberately, so it is not a porting mistake and
		// should not be "corrected" back by a later session. Anyone who wants the original's
		// economy sets the slider to 10.
		inline float dragonSoulsPerPerkPoint = 1.0F;
	}
}
