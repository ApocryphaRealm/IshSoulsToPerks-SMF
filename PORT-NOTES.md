# Port notes - Ish's Souls to Perks -> SMF Settings

Source: `current project mod\Ish's Souls to Perks-1955-1-2.zip` (Nexus Skyrim Special Edition
mod 1955, "Ish's Souls to Perks" by ishmaeltheforsaken, version 1.2SE). Closed-source ESP+BSA,
no public repository, no Papyrus source shipped in the archive.

## What was actually inspected (rule 30 - ask the object, don't infer)

The archive's own readme was re-verified rather than trusted from the earlier triage summary
in `PROGRESS.md`, and the ESP/BSA themselves were inspected directly:

- **`Ish's Souls to Perks.txt`** (the mod's own readme): confirms the mechanic ("buy perk points
  with dragon souls"), the default price (10 dragon souls per perk point), the console fallback
  (`set ishperkcost to X`), and the changelog line that originally suggested MCM support existed
  (`6/8/2012, 1.1 - removed SKSE requirement; added MCM support`).
- **`Ish's Souls to Perks.bsa`** was parsed with `bethesda-structs` (pip-installed for this task)
  and contains exactly one compiled script, `scripts/ishsoultoperktrigger.pex`. Its string table
  was read directly with a small custom PEX-header parser (magic/version/string-table only, not
  a full disassembly) and names the real native calls and identifiers the original used:
  `Game.GetPlayer()`, `ModActorValue`, `AddPerkPoints`, the actor value `DragonSouls`, and the
  globals `ishPerkCost` / `ishPerkCost5` / `ishPerkCost10`.
- **`Ish's Souls to Perks.esp`** was parsed with a small custom TES5 record walker (group/record
  headers, subrecord walk, zlib-decompress compressed records) and gave the exact, real values:
  - `GLOB ishPerkCost` = **10.0** (the real shipped default - confirms the readme).
  - `GLOB ishPerkCost5` = **50.0**, `GLOB ishPerkCost10` = **100.0** - both exactly 5x/10x
    `ishPerkCost`, confirming these are *precomputed totals* for buying 5 or 10 points at once,
    not independent prices.
  - `ACTI ishsoultoperktriggerstone` (FULL name "Dragonstone", model
    `Clutter\Ruins\DragonStone\RuinsDragonStone01.nif`) - the in-world object at the Guardian
    Stones, carrying the trigger script with properties bound to `ishPerkCost`,
    `ishSoulsToPerksMenu`, `ishPerkCost5`, `ishPerkCost10`.
  - `MESG ishSoulsToPerksMenu` - `DESC` reads *"You may purchase a perk point for %.0f dragon
    souls. How many perk points would you like to purchase?"*, with buttons **"1", "5", "10"**
    (plus a "None"/cancel entry), each button carrying a `CTDA` condition referencing the
    matching cost global - consistent with each option only being selectable when affordable.
  - `QUST ishstpmcmmenuquest` - the MCM registration quest. Its own script
    (`ishstpmcmscript`) was **not** found anywhere in the BSA, so the MCM half of the original
    (added in its 1.1 changelog, years before the SE port in this archive) appears to be broken
    or incomplete in this specific SE release - moot for this port either way, since SMF replaces
    the MCM page entirely.
  - A second, unused activator (`ishSoulsToPerksTriggerNOTUSED`) exists in the ESP and was
    ignored - its own `EDID` says what it is.
- A full bytecode disassembly of the `.pex` (not just its string table) was **not** attempted -
  the ESP's own records already gave every concrete number and identifier the mechanic needs
  (the price, the three purchase quantities, the actor value, the native call names), so a full
  disassembly would only have confirmed control flow already implied unambiguously by the data
  above (deduct souls, grant point(s), gated by affordability). Building Orvid/Champollion
  (LGPL-3.0, `github.com/Orvid/Champollion`) from source was attempted as the more rigorous
  route and is left in the scratchpad partially built if a future session wants to finish it and
  cross-check this port's assumptions directly against the decompiled `.psc`.

## Design decisions

- **The one real setting - dragon souls per perk point (`ishPerkCost`, default 10) - is mirrored
  1:1** as `settings::perks::dragonSoulsPerPerkPoint`, replacing the original's MCM slider (and
  its `set ishperkcost to X` console fallback) with a real SMF slider. Per
  `SMF-CONVERSION-PLAYBOOK.md` Part 1 step 4: "mirror the existing settings 1:1 at first - a port
  is not the place to redesign what the mod configures."
- **The three purchase quantities (1/5/10) are mirrored 1:1** as three buttons on the settings
  page (`Perks.h`/`Perks.cpp`, `UI.cpp`'s `RenderPurchaseSection`), each showing its live total
  cost and whether the player can currently afford it.
- **The in-world "Dragonstone" object and its Guardian Stones placement are NOT reproduced.**
  This is the one real behavior change, and it is deliberate: adding a new placed `ACTI`
  reference at a specific world location is ESP/world-editing work, outside what a script-free
  native SKSE plugin can do, and outside this task's scope (a settings page, not a new quest or
  world object). The purchase is instead exposed directly as buttons on the SMF settings page -
  the same design choice this project already made for **Perk Reallocation - SMF Settings**,
  which replaced Ish's Respec Mod's "drink a potion" trigger with a settings-page button rather
  than adding a new alchemy item. This is flagged explicitly per the playbook's Case-A-style
  "no disk source" guidance (Part 1 step 3) about old in-game interaction surfaces needing an
  explicit keep-or-remove decision, not a silent one.
- **No keybind was added** (`CLAUDE.md` rule 28 only applies to a mod with a bindable in-game
  action). There is nothing to bind here - opening the settings page uses SMF's own menu key,
  and the purchase itself is a button on that page, the same reasoning already applied to
  Carry Weight Per Level - SMF Settings and Perk Reallocation - SMF Settings, neither of which
  added a keybind either.
- **Dragon souls are read/spent via `RE::ActorValue::kDragonSouls`** (confirmed present in this
  project's own vendored CommonLibSSE-NG headers, `include/RE/A/ActorValues.h`), through
  `RE::ActorValueOwner::ModActorValue`/`GetActorValue` - the same actor-value-owner pattern
  Carry Weight Per Level-SMF's `Leveling.cpp` already uses for `kCarryWeight`.
- **Perk points are granted via `RE::PlayerCharacter::GetPlayerRuntimeData().perkCount`**
  (signed 8-bit, clamped 0-127) rather than any Papyrus-only `AddPerkPoints` binding - this
  project's own **Perk Reallocation - SMF Settings** (`source/Respec.cpp`) already researched
  and confirmed this is the correct native equivalent (the same field skse64's own
  `papyrusGame::ModPerkPoints` and the public MIT-licensed `covey-j/ActorCopyLib` both use),
  reused directly per `CLAUDE.md` rule 24 rather than re-derived from scratch.
- **A refused purchase touches nothing.** `Perks::TryPurchase` checks affordability (and the
  127-point ceiling) before deducting anything, and only ever grants a full purchase or none -
  no partial deduction - matching the original's own behavior of an unaffordable button simply
  not being selectable.

## Known caveats for review

- **`UI::RenderPurchaseSection()` reads `RE::PlayerCharacter::GetSingleton()`'s dragon-soul
  actor value directly on the render thread**, for the live "Dragon souls: N" display and the
  per-button affordability check - every *write* in this mod (the actual purchase) is deferred
  to the main thread via `SKSE::GetTaskInterface()`, matching this project's established
  pattern, but this one read is not. It is null-safe (`GetSingleton()` returns null outside a
  running game; `Perks::GetCurrentDragonSouls()` null-checks the `ActorValueOwner` interface),
  and a single aligned-float field read without mutation is a common, generally-accepted
  pattern in other SKSE plugins' UI code - but none of this project's *other* SMF pages read
  live engine state (as opposed to their own plugin-owned settings variables) directly during
  `Render()`, so there is no established precedent here to lean on. Worth a second look, and
  worth watching for in an in-game test.
- **A full Champollion (Orvid/Champollion, LGPL-3.0) bytecode disassembly of the original's
  `.pex` was not completed** - see "What was actually inspected" above for what was verified
  instead (the BSA's string table plus every relevant ESP record) and why that was judged
  sufficient. A partial build attempt is left in the scratchpad if a future session wants to
  finish it and cross-check this port's control-flow assumptions (deduct-then-grant, gated by
  affordability, no partial purchase) directly against the decompiled source.

## Licensing

Clean-room reimplementation: no Papyrus code, ESP records, or BSA assets from the original mod
are reused anywhere in this repo - only the general mechanic ("spend dragon souls to buy a
perk point"), which is a game mechanic and not copyrightable, the same reasoning already applied
to this project's other from-scratch mods (Carry Weight Per Level, Perk Reallocation). New code
here is MIT-licensed, matching those two siblings.
