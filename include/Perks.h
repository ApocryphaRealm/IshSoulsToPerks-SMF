#pragma once

// The mod's own core mechanic - a native re-implementation of "spend dragon souls, gain a perk
// point", clean-room reverse-engineered from Ish's Souls to Perks (Nexus Skyrim Special Edition
// mod 1955, by ishmaeltheforsaken - closed-source ESP+BSA, no public repository, no Papyrus
// source shipped). The archive itself was inspected directly rather than only trusted from its
// own readme, per CLAUDE.md rule 30 ("ask the object, don't infer") - see PORT-NOTES.md for the
// full inspection trail:
//
//   - The BSA's one compiled script (scripts/ishsoultoperktrigger.pex) was extracted and its
//     string table read directly. It references the native functions
//     "Game.GetPlayer().ModActorValue(\"DragonSouls\", ...)" and "AddPerkPoints(...)", and the
//     globals "ishPerkCost" / "ishPerkCost5" / "ishPerkCost10".
//   - The ESP's own records were parsed directly: GLOB "ishPerkCost" = 10.0 (the real, shipped
//     default), "ishPerkCost5" = 50.0 and "ishPerkCost10" = 100.0 (5x/10x the per-point cost,
//     not independent prices), a MESG "ishSoulsToPerksMenu" whose DESC reads "You may purchase
//     a perk point for %.0f dragon souls. How many perk points would you like to purchase?"
//     with buttons "1"/"5"/"10" (each button's CTDA condition referencing the matching cost
//     global, consistent with gating a button's visibility on affordability), and an ACTI
//     "ishsoultoperktriggerstone" (FULL name "Dragonstone", model
//     Clutter\Ruins\DragonStone\RuinsDragonStone01.nif) carrying the trigger script.
//
// So the real mechanic is: dragon souls are tracked as RE::ActorValue::kDragonSouls (confirmed
// against this project's own vendored CommonLibSSE-NG headers, RE/A/ActorValues.h), spent via
// ModActorValue with a negative delta, and a perk point is granted per point purchased. No
// native CommonLibSSE-NG convenience function wraps "add a perk point" (Papyrus's
// AddPerkPoints binds directly to the engine field) - PerkReallocation-SMF, this project's
// other from-scratch perk-manipulation mod, already researched and confirmed the correct native
// equivalent: RE::PlayerCharacter::GetPlayerRuntimeData().perkCount, a signed 8-bit field
// (0-127) - the same field skse64's own PapyrusGame.cpp ModPerkPoints binding and
// covey-j/ActorCopyLib (MIT) both use. Re-used here rather than re-derived, per CLAUDE.md
// rule 24 (search the project for an existing solution before inventing one).
//
// What changed in the port, and why (mirrored 1:1 where the original had a real setting; only
// the *interaction surface* changed, per SMF-CONVERSION-PLAYBOOK.md's "mirror the existing
// settings, only change how they're configured" guidance):
//   - The one real setting - dragon souls per perk point, default 10 - is mirrored exactly as
//     settings::perks::dragonSoulsPerPerkPoint (see Settings.h), replacing the original's MCM
//     slider and "set ishperkcost to X" console fallback with a real SMF slider.
//   - The three purchase quantities (1/5/10) are mirrored exactly as three buttons on the
//     settings page, replacing the original's in-world "Dragonstone" object at the Guardian
//     Stones (which would need new ESP/quest/world-placement work well outside a script-free
//     native plugin's scope) - the same design choice this project already made for Perk
//     Reallocation-SMF, which replaced Ish's Respec Mod's "drink a potion" trigger with a
//     settings-page button rather than adding a new alchemy item. A settings-page button is a
//     legitimate SMF-native equivalent of "walk up to an object and activate it", not a
//     redesign of what the mod actually does.
namespace Perks
{
	enum class PurchaseResult
	{
		kSuccess,
		kInsufficientSouls,
		kInvalidCount,
		kNullPlayer,
		kNullActorValueOwner,
		kPerkCountAtMaximum
	};

	// The player's current dragon soul count, read live from RE::ActorValue::kDragonSouls.
	// Returns 0.0 (with a logged error) if a_player or its ActorValueOwner interface is null,
	// which is a safe fallback for a display-only read (it just shows "0 souls", never crashes
	// a render).
	float GetCurrentDragonSouls(RE::PlayerCharacter* a_player);

	// Total cost, in dragon souls, to buy a_count perk points at the current
	// settings::perks::dragonSoulsPerPerkPoint rate - the same ishPerkCost * count arithmetic
	// the original's own "ishPerkCost5"/"ishPerkCost10" globals encoded as precomputed totals.
	float GetCost(int a_count);

	// True if a_player currently has enough dragon souls to afford a_count perk points at the
	// current rate - mirrors the affordability gating the original's MESG button CTDA
	// conditions applied to the "5" and "10" options.
	bool CanAfford(RE::PlayerCharacter* a_player, int a_count);

	// Attempts to buy a_count perk point(s) for a_player (RE::PlayerCharacter*, matching this
	// project's existing perk-manipulation code in PerkReallocation-SMF's Respec.cpp - perkCount
	// lives behind RE::PlayerCharacter::GetPlayerRuntimeData(), not the generic RE::Actor
	// interface): checks affordability, then (only if affordable) deducts
	// a_count * settings::perks::dragonSoulsPerPerkPoint dragon souls and grants a_count perk
	// point(s), clamped so perkCount (a signed 8-bit field) never overflows. Nothing is touched
	// at all unless the whole purchase can be completed - no partial deduction, matching the
	// original's own message-menu behaviour of the "5"/"10" buttons only ever being selectable
	// when affordable in full.
	PurchaseResult TryPurchase(RE::PlayerCharacter* a_player, int a_count);

	// Human-readable text for a PurchaseResult, for the settings page's status line.
	const char* DescribeResult(PurchaseResult a_result);
}
