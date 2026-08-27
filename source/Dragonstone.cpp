#include "Dragonstone.h"

#include "Perks.h"
#include "Settings.h"
#include "utils/Logger.h"

#include <format>
#include <string>

namespace Dragonstone
{
	namespace
	{
		// The contract with the plugin. Both halves are fixed: the ESL declares the activator at
		// this local FormID, and this lookup finds it by plugin filename, so neither may drift
		// without the other. See Dragonstone.h.
		constexpr const char* kPluginFileName = "IshSoulsToPerks.esl";
		constexpr RE::FormID kActivatorLocalFormID = 0x800;

		// Resolved once, at kDataLoaded, and then never re-looked-up. Null means "the plugin is
		// not installed", which is a supported configuration rather than a fault.
		RE::TESForm* g_activatorForm = nullptr;
		bool         g_sinkRegistered = false;
		bool         g_reportedMissing = false;

		// Fires on every activation in the game, so it must be cheap and must reject anything
		// that is not ours as early as possible - the first comparison below is a pointer test
		// against the one form we care about.
		class ActivateSink : public RE::BSTEventSink<RE::TESActivateEvent>
		{
		public:
			static ActivateSink* GetSingleton()
			{
				static ActivateSink singleton;

				return &singleton;
			}

			RE::BSEventNotifyControl ProcessEvent(
				const RE::TESActivateEvent*                a_event,
				RE::BSTEventSource<RE::TESActivateEvent>*  a_source) override
			{
				(void)a_source;

				if (!a_event || !g_activatorForm)
				{
					return RE::BSEventNotifyControl::kContinue;
				}

				RE::TESObjectREFR* activated = a_event->objectActivated.get();
				RE::TESObjectREFR* activator = a_event->actionRef.get();

				if (!activated || !activator)
				{
					return RE::BSEventNotifyControl::kContinue;
				}

				// The event carries the placed reference; what identifies it as ours is the base
				// object it was placed from, not the reference's own FormID. Comparing base forms
				// also means a second copy of the stone (a future version, or a user placing one
				// by console) would work without any change here.
				RE::TESBoundObject* base = activated->GetBaseObject();

				if (!base || base->GetFormID() != g_activatorForm->GetFormID())
				{
					return RE::BSEventNotifyControl::kContinue;
				}

				// Only the player buys perk points. An NPC or a script activating the stone is
				// ignored rather than silently granting the player something.
				RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();

				if (!player || activator != static_cast<RE::TESObjectREFR*>(player))
				{
					logger::debug("Dragonstone activated by a non-player reference; ignoring");

					return RE::BSEventNotifyControl::kContinue;
				}

				HandlePlayerActivation(player);

				return RE::BSEventNotifyControl::kContinue;
			}

		private:
			static void HandlePlayerActivation(RE::PlayerCharacter* a_player)
			{
				const float cost = Perks::GetCost(1);
				const float souls = Perks::GetCurrentDragonSouls(a_player);

				logger::info(
					"Dragonstone activated by the player: {:.0f} dragon souls held, {:.0f} needed for one perk point",
					souls,
					cost);

				const Perks::PurchaseResult result = Perks::TryPurchase(a_player, 1);

				logger::info("Dragonstone purchase result: {}", Perks::DescribeResult(result));

				// The stone's whole point is that it works without opening a menu, so the outcome
				// has to be visible in the world. A corner notification is the vanilla-consistent
				// way to say so, and it is what the game itself uses for this class of event.
				if (result == Perks::PurchaseResult::kSuccess)
				{
					const std::string message =
						std::format("Perk point purchased for {:.0f} dragon souls.", cost);

					RE::DebugNotification(message.c_str());
				}
				else
				{
					RE::DebugNotification(Perks::DescribeResult(result));
				}
			}
		};
	}

	void Init(bool a_lastAttempt)
	{
		if (g_sinkRegistered)
		{
			return;
		}

		RE::TESDataHandler* dataHandler = RE::TESDataHandler::GetSingleton();

		if (!dataHandler)
		{
			// Not fatal and not final - kDataLoaded can be reached with the handler still
			// settling, and rule 17 says a late-binding lookup is retried, not concluded.
			logger::debug("Dragonstone::Init: no TESDataHandler yet");

			return;
		}

		RE::TESForm* form = dataHandler->LookupForm(kActivatorLocalFormID, kPluginFileName);

		if (!form)
		{
			// The supported "plugin not installed" path. Reported once as info on the final
			// attempt, then never again - this is a configuration, not a failure, and the mod is
			// fully usable from its settings page without it.
			if (a_lastAttempt && !g_reportedMissing)
			{
				g_reportedMissing = true;

				logger::info(
					"{} is not present in the load order, so the in-world Dragonstone is disabled. "
					"The settings page's purchase buttons are unaffected.",
					kPluginFileName);
			}
			else
			{
				logger::debug("Dragonstone::Init: {} not found yet", kPluginFileName);
			}

			return;
		}

		g_activatorForm = form;

		RE::ScriptEventSourceHolder* holder = RE::ScriptEventSourceHolder::GetSingleton();

		if (!holder)
		{
			// Resolved the form but cannot listen for activations - leave g_sinkRegistered false
			// so a later retry can still complete the job.
			logger::warn("Dragonstone::Init: no ScriptEventSourceHolder; activation sink not registered");

			return;
		}

		holder->AddEventSink<RE::TESActivateEvent>(ActivateSink::GetSingleton());
		g_sinkRegistered = true;

		logger::info(
			"Dragonstone activator resolved from {} at 0x{:08X}; activation sink registered",
			kPluginFileName,
			form->GetFormID());
	}

	bool IsActive()
	{
		return g_sinkRegistered && g_activatorForm != nullptr;
	}
}
