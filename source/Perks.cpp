#include "Perks.h"

#include "Diagnostics.h"
#include "Settings.h"
#include "utils/Logger.h"

#include <algorithm>

namespace Perks
{
	float GetCurrentDragonSouls(RE::PlayerCharacter* a_player)
	{
		if (!a_player)
		{
			logger::error("GetCurrentDragonSouls: null player");

			return 0.0F;
		}

		RE::ActorValueOwner* avOwner = a_player->AsActorValueOwner();

		if (!avOwner)
		{
			logger::error("GetCurrentDragonSouls: player's ActorValueOwner interface was null");

			return 0.0F;
		}

		return avOwner->GetActorValue(RE::ActorValue::kDragonSouls);
	}

	float GetCost(int a_count)
	{
		if (a_count <= 0)
		{
			return 0.0F;
		}

		return settings::perks::dragonSoulsPerPerkPoint * static_cast<float>(a_count);
	}

	bool CanAfford(RE::PlayerCharacter* a_player, int a_count)
	{
		if (!a_player || a_count <= 0)
		{
			return false;
		}

		return GetCurrentDragonSouls(a_player) >= GetCost(a_count);
	}

	PurchaseResult TryPurchase(RE::PlayerCharacter* a_player, int a_count)
	{
		if (!a_player)
		{
			logger::error("TryPurchase: null player (count {})", a_count);

			return PurchaseResult::kNullPlayer;
		}

		if (a_count <= 0)
		{
			logger::debug("TryPurchase: refused - invalid count {}", a_count);

			return PurchaseResult::kInvalidCount;
		}

		RE::ActorValueOwner* avOwner = a_player->AsActorValueOwner();

		if (!avOwner)
		{
			logger::error("TryPurchase: player's ActorValueOwner interface was null (count {})", a_count);

			return PurchaseResult::kNullActorValueOwner;
		}

		const float cost = GetCost(a_count);
		const float currentSouls = avOwner->GetActorValue(RE::ActorValue::kDragonSouls);

		logger::debug("TryPurchase: count {}, cost {:.1f} dragon souls, player has {:.1f}",
			a_count, cost, currentSouls);

		if (currentSouls < cost)
		{
			logger::debug("TryPurchase: refused - not enough dragon souls (has {:.1f}, needs {:.1f})",
				currentSouls, cost);

			diagnostics::RecordPurchaseRefused(a_count, cost, currentSouls);

			return PurchaseResult::kInsufficientSouls;
		}

		auto& runtimeData = a_player->GetPlayerRuntimeData();
		const int currentPerkCount = static_cast<int>(runtimeData.perkCount);
		const int desiredPerkCount = currentPerkCount + a_count;
		const int newPerkCount = std::clamp(desiredPerkCount, 0, 127);

		if (newPerkCount != desiredPerkCount)
		{
			logger::warn("TryPurchase: perkCount would have exceeded the field's 127 maximum "
						 "(current {}, requested +{}); refusing rather than granting a partial amount",
				currentPerkCount, a_count);

			diagnostics::RecordPurchaseRefused(a_count, cost, currentSouls);

			return PurchaseResult::kPerkCountAtMaximum;
		}

		// Deduct souls first, then grant the points - if anything above already refused, nothing
		// has been touched yet, matching the original's own message-menu behaviour of a refused
		// purchase leaving the player untouched.
		avOwner->ModActorValue(RE::ActorValue::kDragonSouls, -cost);
		runtimeData.perkCount = static_cast<std::int8_t>(newPerkCount);

		logger::info("TryPurchase: bought {} perk point(s) for {:.1f} dragon souls "
					 "(perkCount {} -> {})",
			a_count, cost, currentPerkCount, newPerkCount);

		diagnostics::RecordPurchaseSucceeded(a_count, cost);

		return PurchaseResult::kSuccess;
	}

	const char* DescribeResult(PurchaseResult a_result)
	{
		switch (a_result)
		{
		case PurchaseResult::kSuccess:
			return "Purchase successful.";
		case PurchaseResult::kInsufficientSouls:
			return "Not enough dragon souls.";
		case PurchaseResult::kInvalidCount:
			return "Invalid purchase quantity.";
		case PurchaseResult::kNullPlayer:
			return "Could not find the player.";
		case PurchaseResult::kNullActorValueOwner:
			return "Could not access the player's actor values.";
		case PurchaseResult::kPerkCountAtMaximum:
			return "You already have the maximum possible number of perk points.";
		}

		return "Unknown result.";
	}
}
