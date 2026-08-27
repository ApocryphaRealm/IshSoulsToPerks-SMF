#pragma once

// The in-world "Dragonstone" activator - the original mod's own interaction surface, restored.
//
// Background, because this reverses an earlier decision and the reasoning matters: the first
// port (1.0.0) deliberately dropped the original's placed activator at the Guardian Stones and
// exposed the purchase as buttons on the SMF settings page instead, on the grounds that adding a
// new placed reference is ESP work rather than something a script-free native plugin can do.
// That reasoning was correct and still is - neither Base Object Swapper nor SkyPatcher can add a
// placement (BOS swaps what is already there; SkyPatcher edits record values). It is precisely
// the narrow case CLAUDE.md rule 1 still reserves an ESP for: "adding a genuinely new reference
// that none of the three can express."
//
// So the object comes back carried by a minimal ESL - IshSoulsToPerks.esl - holding exactly two
// records and nothing else: the ACTI at local FormID 0x800, and the REFR placing it at 0x801.
// The plugin is a placement carrier only. It hosts no script, no quest, no globals and no
// message record; every behaviour below lives here, in the DLL.
//
// Two properties of this design are deliberate and should not be quietly changed:
//
//   1. THE ESL IS OPTIONAL AT RUNTIME. If it is missing, this module logs the fact once and does
//      nothing further - it never fails a load and never throws. The SMF settings page keeps
//      working exactly as it did in 1.0.0, so a user without the plugin still has the full
//      mechanic. That is also why the settings-page buttons were kept rather than replaced: the
//      Dragonstone is an ADDITIONAL trigger, not the only one, and losing SMF or the plugin must
//      never lose the mod's actual function.
//
//   2. THE FORM IDS ARE A CONTRACT with the plugin. 0x800 is looked up by name below, so the
//      plugin cannot renumber it. Light plugins address their own records in the 0x800-0xFFF
//      range, which is where these come from - see PLUGIN-NOTES.md for how the plugin is built
//      and how to rebuild it.
//
// Note also that a placed reference bakes its FormID into any save made while the plugin is
// active. Renaming or removing IshSoulsToPerks.esl after the fact leaves that reference dangling
// in those saves, so the filename above is effectively permanent once shipped.

namespace Dragonstone
{
	// Resolves the activator form from IshSoulsToPerks.esl and registers the activation sink.
	//
	// Safe to call repeatedly - it is a cheap no-op once resolution has succeeded, which is what
	// makes it usable as a rule-17 late-binding retry (never treat a one-shot lookup of something
	// that may not be ready yet as terminal). Call it from kDataLoaded, when the load order is
	// available; a_lastAttempt only controls whether an unresolved lookup is reported as a
	// definitive "not installed" rather than a quiet "not yet".
	void Init(bool a_lastAttempt = false);

	// True once the activator form has been found and the sink is live. False means the plugin
	// is not installed, which is a supported configuration, not an error.
	bool IsActive();
}
