# Port notes - Ish's Souls to Perks -> SMF Settings

Source: `5. current project mod\Ish's Souls to Perks-1955-1-2.zip` (Nexus Skyrim Special Edition
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
- A full bytecode disassembly of the `.pex` (not just its string table) was **not** completed -
  the ESP's own records already gave every concrete number and identifier the mechanic needs
  (the price, the three purchase quantities, the actor value, the native call names), so a full
  disassembly would only have confirmed control flow already implied unambiguously by the data
  above (deduct souls, grant point(s), gated by affordability). Building Orvid/Champollion
  (LGPL-3.0, `github.com/Orvid/Champollion` - a real, buildable CMake+vcpkg CLI Papyrus
  decompiler, not previously in `code library`) from source was attempted as the more rigorous
  route and was still running (stuck on vcpkg's `boost-program-options` dependency) when this
  port was otherwise finished. It was building in this session's own temp scratchpad, which does
  **not** persist between sessions - so if a future session wants to cross-check this port's
  assumptions against a real decompile, it needs to re-clone and re-build
  `github.com/Orvid/Champollion` (`cmake -S . -B build -G Ninja
  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake` against the BSA's
  extracted `scripts/ishsoultoperktrigger.pex`), not look for anything left behind by this one.

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
  - **A real consequence of this choice, worth being explicit about**: because the purchase
    action lives *only* on the SMF page, this mod's entire core mechanic - not just its
    configurability - is unavailable if SMF is missing or too old. `HasRequiredExports()`
    still degrades gracefully (no crash, a clear log line, the INI is still read), but unlike
    a mod whose underlying feature keeps working headlessly and only loses its *settings UI*
    without SMF (e.g. Carry Weight Per Level's level-up bonus, which applies regardless of
    whether its page ever registers), there is no equivalent fallback here - correctly so,
    since the original mod's own purchase trigger (activating the Dragonstone) has no INI/
    console equivalent either, so there was nothing headless to preserve. Worth noting on the
    Nexus page's requirements section once this mod is finalized, so SMF reads as a hard
    requirement, not merely recommended.
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

---

## 2026-08-26 — Re-examined: do SkyPatcher and Base Object Swapper remove the ESP need here?

Asked directly by the author after both tools were adopted into rule 1's parallel set. **Answer: no,
not for this mod's specific gap — but the ESP is still avoidable, by a different route than
either of them.**

### Why neither new tool applies

The thing this port dropped is the **"Dragonstone" `ACTI` placed at the Guardian Stones**. That
is *adding a new reference to the world*, and it is the one thing both new tools explicitly
cannot do:

- **Base Object Swapper swaps what is already there.** Its own documentation, quoted in
  `CodeLibrary.md`: it covers *"replace this with that"*, not *"put something new here."* There
  is no vanilla reference at that site that is a plausible stand-in to swap, and swapping some
  arbitrary nearby object into an activator would be a contrivance that breaks whatever it
  replaced.
- **SkyPatcher edits values on records.** It can change what a form *is like*; it cannot create a
  placement. It could add a keyword to the existing Guardian Stones, which matters below, but it
  cannot put a new object beside them.

So this is precisely the narrow case rule 1 still reserves an ESP for: *"adding a genuinely new
reference that none of the three can express."* The tools did not make that case disappear —
they made it **rarer**, which is a different and still valuable thing.

### But the ESP is optional, because the mod already works without it

The shipped 1.0.0 build exposes the purchase as **buttons on the SMF settings page**, mirroring
the original's 1/5/10 quantities with live affordability. The mechanic is complete. The world
object is *flavour and discoverability*, not function — so the real question is not "how do we
restore the object" but "is the object worth an ESP."

### The three honest routes, cheapest first

1. **Do nothing.** Keep the settings-page buttons. Costs a little immersion and the "stumble
   across it at the Guardian Stones" discovery moment; costs no plugin, no save-baked reference,
   no load-order position to defend. This is what already ships.
2. **Hook the EXISTING Guardian Stones.** Our own plugin sinks activation events and opens the
   purchase prompt when the player activates a Guardian Stone. **No new reference, no ESP, no
   BOS** — pure route 1, the top of the ladder. It restores the location association almost
   exactly, since the original's stone sat at that same site. **The caveat is real and should be
   settled before building it:** the Guardian Stones already do something on activation (granting
   their blessing), so this either has to present both choices or find a non-conflicting gesture.
   That is a design decision, not a technical blocker. SkyPatcher becomes genuinely useful here
   as the *data-driven* half — mark which references trigger the menu with a keyword, so the
   trigger set is configurable rather than a hardcoded FormID list.
3. **A tiny ESL-flagged plugin carrying only the placement.** All behaviour stays in the DLL; the
   plugin holds nothing but the activator record and its reference. This is the original's own
   design and is what category 6 doctrine prescribes for a world reference. It is the last
   resort, and it is legitimately available — but it should be chosen deliberately, and only if
   route 2's activation-conflict question turns out badly.

### Recommendation

**Route 2, with route 1 as the fallback.** It restores what was actually lost — the association
with the Guardian Stones — without an ESP, without a save-baked reference, and without either new
tool being stretched past what it does. Route 3 stays on the table but should not be reached for
first now that a runtime path exists.

**This does not block the current test run.** The 1.0.0 build is functionally complete and should
be tested as-is; this is a follow-up enhancement, not a defect.

### DECIDED 2026-08-26: route 3 - a small ESL

the author: *"let's go with option three and use a small ESL."*

So the Dragonstone placement comes back, carried by a **minimal ESL-flagged plugin that holds
nothing but the activator record and its placed reference at the Guardian Stones**. All behaviour
stays in the DLL - the plugin is a placement carrier, not a script host. This is the original
mod's own design and is what category 6 doctrine prescribes for a world reference, and it is the
narrow case rule 1 still reserves an ESP for: adding a genuinely new reference that neither the
runtime plugin, Base Object Swapper nor SkyPatcher can express.

**Consequences to honour when building it:**

- **ESL-flagged**, so it costs no full plugin slot.
- **The plugin becomes a hard requirement** and ships with the mod - a category 6 mod's world
  reference is never separated from its plugin.
- **The SMF page buttons stay.** They are the mod's accessibility path and already work; the
  Dragonstone is an additional trigger, not a replacement. Losing SMF should not lose the mechanic.
- **The reference will bake into saves.** Once placed, its FormID persists in any save made with
  the plugin active, so the plugin must not be casually renamed or removed later.
- Build it with the Creation Kit (CKPE is installed on the SME instance) or an equivalent plugin
  authoring route, and verify the result with `.MD\scripts\Get-PluginRecordTypes.ps1` - it should
  report `CELL`/`WRLD`/`REFR` and nothing surprising.

**Not part of the current test run.** The shipped 1.0.0 is functionally complete via the settings
page and is what is being tested now; the ESL is the next version's work.
