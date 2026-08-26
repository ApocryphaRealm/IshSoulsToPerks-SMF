#include "Diagnostics.h"

#include "DevBench/DevBenchAPI.h"
#include "Settings.h"
#include "utils/Logger.h"

#include <mutex>

namespace diagnostics
{
	namespace
	{
		using clock = std::chrono::steady_clock;

		std::mutex mtx;

		struct State
		{
			std::uint64_t purchasesSucceeded = 0;
			std::optional<clock::time_point> lastPurchaseSucceeded;
			int lastPurchaseCount = 0;
			float lastPurchaseCost = 0.0F;

			std::uint64_t purchasesRefused = 0;
			std::optional<clock::time_point> lastPurchaseRefused;
			int lastRefusedCount = 0;
			float lastRefusedCost = 0.0F;
			float lastRefusedSouls = 0.0F;
		};

		State state;

		// Renders "field": null or "field": <seconds ago>, so a query can tell "never
		// happened" apart from "happened a long time ago" instead of both looking like a
		// missing/zero field.
		std::string SecondsAgoField(const char* a_name, const std::optional<clock::time_point>& a_when)
		{
			if (!a_when)
			{
				return std::format("\"{}SecondsAgo\": null", a_name);
			}

			const double seconds = std::chrono::duration<double>(clock::now() - *a_when).count();

			return std::format("\"{}SecondsAgo\": {:.1f}", a_name, seconds);
		}

		void StatusTool(void*, const char*, void* a_sink, DevBenchAPI::WriteFn a_write)
		{
			std::string json;

			{
				std::scoped_lock lock(mtx);

				json = std::format(
					"{{"
					"\"settings\":{{"
					"\"dragonSoulsPerPerkPoint\":{:.1f}"
					"}},"
					"\"purchasesSucceeded\":{{"
					"\"count\":{},"
					"\"lastCount\":{},"
					"\"lastCost\":{:.1f},"
					"{}"
					"}},"
					"\"purchasesRefused\":{{"
					"\"count\":{},"
					"\"lastCount\":{},"
					"\"lastCost\":{:.1f},"
					"\"lastPlayerSouls\":{:.1f},"
					"{}"
					"}}"
					"}}",
					settings::perks::dragonSoulsPerPerkPoint,
					state.purchasesSucceeded,
					state.lastPurchaseCount,
					state.lastPurchaseCost,
					SecondsAgoField("last", state.lastPurchaseSucceeded),
					state.purchasesRefused,
					state.lastRefusedCount,
					state.lastRefusedCost,
					state.lastRefusedSouls,
					SecondsAgoField("last", state.lastPurchaseRefused));
			}

			a_write(a_sink, json.c_str());
		}
	}

	void Init(bool a_lastAttempt)
	{
		static bool registered = false;

		if (registered)
		{
			return;
		}

		DevBenchAPI::IDevBenchInterface001* devBench = DevBenchAPI::GetDevBenchInterface001();

		if (!devBench)
		{
			if (a_lastAttempt)
			{
				logger::info("DevBench not detected; skipping the \"ishsoulstoperks.status\" live-diagnostics "
							 "tool (logging alone still covers this session - see CLAUDE.md rule 31)");
			}
			else
			{
				logger::debug("DevBench not detected yet; will retry at the next message");
			}

			return;
		}

		constexpr const char* descriptor =
			"{"
			"\"description\":\"Live Ish's Souls to Perks state: the current dragon-soul-per-"
			"perk-point price, and the last successful and refused purchase attempts.\","
			"\"inputSchema\":{\"type\":\"object\",\"properties\":{}},"
			"\"readOnly\":true"
			"}";

		if (devBench->RegisterTool("ishsoulstoperks.status", descriptor, &StatusTool, nullptr))
		{
			logger::info("Registered \"ishsoulstoperks.status\" with DevBench (build {})", devBench->GetBuildNumber());
		}
		else
		{
			logger::warn("DevBench reported \"ishsoulstoperks.status\" replaced an existing tool of the same name");
		}

		registered = true;
	}

	void RecordPurchaseSucceeded(int a_count, float a_soulsCost)
	{
		std::scoped_lock lock(mtx);

		++state.purchasesSucceeded;
		state.lastPurchaseCount = a_count;
		state.lastPurchaseCost = a_soulsCost;
		state.lastPurchaseSucceeded = clock::now();
	}

	void RecordPurchaseRefused(int a_count, float a_soulsCost, float a_currentSouls)
	{
		std::scoped_lock lock(mtx);

		++state.purchasesRefused;
		state.lastRefusedCount = a_count;
		state.lastRefusedCost = a_soulsCost;
		state.lastRefusedSouls = a_currentSouls;
		state.lastPurchaseRefused = clock::now();
	}
}
