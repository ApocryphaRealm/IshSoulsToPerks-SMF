#pragma once

// Backs the "ishsoulstoperks.status" DevBench tool - see CLAUDE.md rule 31 (every mod's first
// version ships with live-queryable state, not just logs reconstructed after the fact).
//
// Every Record* function here is called from the main thread, at the exact point a decision is
// made, and only ever writes a mutex-guarded snapshot. The DevBench tool handler runs on
// devbench's own listener thread and only ever reads that snapshot - it never reaches back into
// game state itself.
namespace diagnostics
{
	// Looks up the DevBench interface (present only if the DevBench plugin is installed) and
	// registers "ishsoulstoperks.status". Safe to call repeatedly - a rule-17 retry, not a
	// one-shot lookup: call again at kPostPostLoad and kDataLoaded too. Every call after the
	// first successful one is a cheap no-op; only the final call (a_lastAttempt = true) logs
	// that DevBench was never found, so the "not installed" conclusion is not reported before
	// every retry is exhausted.
	void Init(bool a_lastAttempt = false);

	// A perk-point purchase succeeded: a_count point(s) bought for a_soulsCost dragon souls.
	void RecordPurchaseSucceeded(int a_count, float a_soulsCost);

	// A perk-point purchase was refused (not enough souls, or perkCount at its maximum).
	// a_currentSouls is the player's dragon soul balance at the time of refusal.
	void RecordPurchaseRefused(int a_count, float a_soulsCost, float a_currentSouls);
}
