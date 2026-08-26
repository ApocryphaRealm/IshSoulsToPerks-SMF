#include "UI.h"

#include "SKSEMenuFramework.h"

#include "Perks.h"
#include "Settings.h"

#include "utils/Logger.h"

#include <algorithm>

namespace UI
{
	namespace
	{
		std::string statusMessage;

		// The slider the arrow keys currently drive. Set by clicking one.
		std::string selectedSlider;

		constexpr const char* kLogLevelNames[] = { "Trace", "Debug", "Info", "Warning", "Error", "Critical", "Off" };
		constexpr int kLogLevelCount = 7;

		// The framework renders from the renderer's present hook, which is not the thread the
		// game's own systems expect to be talked to from - anything beyond touching this
		// plugin's own settings variables has to be handed to the main thread first.
		void OnMainThread(std::function<void()> a_task)
		{
			if (auto* taskInterface = SKSE::GetTaskInterface())
			{
				taskInterface->AddTask(std::move(a_task));
			}
		}

		// Older SMF builds do not export every cimgui function a page needs, and calling
		// through a null function pointer crashes on the first draw rather than failing to
		// register - so every export this page's widgets resolve at runtime is probed here
		// first, by its *resolved* name (varargs widgets resolve to a "...V"-suffixed export).
		// See CarryWeightPerLevel-SMF/source/UI.cpp's identical check (CLAUDE.md rule 24).
		bool HasRequiredExports()
		{
			constexpr const char* required[] = {
				"AddSectionItem",
				"igTextV",
				"igTextDisabledV",
				"igTextWrappedV",
				"igSetTooltipV",
				"igSeparatorText",
				"igCombo_Str_arr",
				"igSliderFloat",
				"igIsItemHovered",
				"igButton",
				"igSameLine",
				"igSpacing",
				"igPushItemWidth",
				"igPopItemWidth",
				// Needed by NudgeableSlider's arrow-key nudge (ported from Dragon's Eye
				// Minimap's UI.cpp - CLAUDE.md rule 24).
				"igIsKeyPressed_Bool",
				"igIsItemClicked",
				"igIsItemActive"
			};

			for (const char* name : required)
			{
				if (!GetMenuFrameworkFunction<void*>(name))
				{
					logger::warn("SKSE Menu Framework does not export \"{}\"", name);

					return false;
				}
			}

			return true;
		}

		// A slider that the arrow keys can also nudge, once it has been clicked. Dragging is
		// hopeless for the last decimal place, and the framework does not turn on ImGui's own
		// keyboard navigation, so this tracks the selection itself rather than changing a
		// setting shared with every other mod's page. Ported verbatim from Dragon's Eye
		// Minimap's UI.cpp, which already had this working - see CLAUDE.md rule 24.
		bool NudgeableSlider(const char* a_label, float* a_value, float a_min, float a_max,
							 const char* a_format, float a_step)
		{
			bool changed = ImGuiMCP::SliderFloat(a_label, a_value, a_min, a_max, a_format);

			if (ImGuiMCP::IsItemClicked() || ImGuiMCP::IsItemActive())
			{
				selectedSlider = a_label;
			}

			if (selectedSlider == a_label)
			{
				float nudge = 0.0F;

				if (ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_LeftArrow) || ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_DownArrow))
				{
					nudge -= a_step;
				}
				if (ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_RightArrow) || ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_UpArrow))
				{
					nudge += a_step;
				}

				if (nudge != 0.0F)
				{
					*a_value = std::clamp(*a_value + nudge, a_min, a_max);
					changed = true;
				}

				ImGuiMCP::SameLine();
				ImGuiMCP::TextDisabled("<-->");
			}

			return changed;
		}

		void HelpMarker(const char* a_description)
		{
			ImGuiMCP::SameLine();
			ImGuiMCP::TextDisabled("(?)");

			if (ImGuiMCP::IsItemHovered())
			{
				ImGuiMCP::SetTooltip("%s", a_description);
			}
		}

		// Fires a purchase attempt on the main thread and turns the result into the status
		// line's next message. a_count matches one of the original mod's own three message-menu
		// options (1/5/10 - see Perks.h's header comment for where those numbers came from).
		void RequestPurchase(int a_count)
		{
			OnMainThread([a_count]() {
				RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
				const Perks::PurchaseResult result = Perks::TryPurchase(player, a_count);

				if (result == Perks::PurchaseResult::kSuccess)
				{
					statusMessage = std::format("Bought {} perk point(s) for {:.0f} dragon souls.",
						a_count, Perks::GetCost(a_count));
				}
				else
				{
					statusMessage = Perks::DescribeResult(result);
				}
			});
		}

		void RenderCostSection()
		{
			using namespace settings::perks;

			ImGuiMCP::SeparatorText("Price");

			NudgeableSlider("Dragon souls per perk point", &dragonSoulsPerPerkPoint, 1.0F, 100.0F, "%.0f", 1.0F);
			HelpMarker("How many dragon souls one perk point costs. The original mod's own default was 10, changeable in game via \"set ishperkcost to X\" in the console or its MCM page - this replaces both with a slider. Applies live, the next time you buy.");
		}

		void RenderPurchaseSection()
		{
			ImGuiMCP::SeparatorText("Buy Perk Points");

			// Read-only, off the main thread (the render hook this page draws from) - unlike
			// every write in this file, which is deferred to OnMainThread(). A single pointer
			// chase down to one already-aligned float field is the same category of read this
			// project's other pages already treat as safe inline (e.g. reading this plugin's
			// own settings variables directly during Render); GetSingleton() is null-safe on
			// its own (returns null outside a running game, e.g. the main menu) and
			// GetCurrentDragonSouls() null-checks the ActorValueOwner interface too. Flagged
			// explicitly here since it is this page's only *engine* (not plugin-owned) read
			// done this way - worth a second look before this port goes further, per this
			// mod's PORT-NOTES.md.
			RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
			const float currentSouls = Perks::GetCurrentDragonSouls(player);

			ImGuiMCP::TextDisabled("Dragon souls: %.0f", currentSouls);

			// Mirrors the original's own message-menu quantities exactly (see Perks.h's header
			// comment - the ESP's own MESG record for "ishSoulsToPerksMenu" offered exactly
			// these three options, each gated on affordability). The original hid an
			// unaffordable option entirely; this project's other SMF pages never disable/hide a
			// button (see PerkReallocation-SMF's "Perform Respec"), so all three always show and
			// TryPurchase itself reports why a purchase was refused.
			constexpr int kCounts[] = { 1, 5, 10 };

			for (int count : kCounts)
			{
				const float cost = Perks::GetCost(count);
				const bool affordable = currentSouls >= cost;

				if (ImGuiMCP::Button(std::format("Buy {}##buy{}", count, count).c_str()))
				{
					RequestPurchase(count);
				}

				ImGuiMCP::SameLine();
				ImGuiMCP::TextDisabled("%.0f souls%s", cost, affordable ? "" : " (not enough)");

				HelpMarker(count == 1
					? "Buy 1 perk point for the price above."
					: std::format("Buy {} perk points at once, for {} times the price above.", count, count).c_str());
			}
		}

		void RenderDebugSection()
		{
			using namespace settings;

			ImGuiMCP::SeparatorText("Debug");

			int level = static_cast<int>(debug::logLevel);
			if (ImGuiMCP::Combo("Log level", &level, kLogLevelNames, kLogLevelCount))
			{
				debug::logLevel = static_cast<logger::level>(level);

				OnMainThread([]() { logger::set_level(settings::debug::logLevel, settings::debug::logLevel); });
			}
			HelpMarker("Applies to the log immediately. Ships at Trace by default - see CLAUDE.md rule 31.");
		}

		void RenderButtons()
		{
			if (ImGuiMCP::Button("Save"))
			{
				OnMainThread([]() {
					statusMessage = settings::Save() ? "Settings saved." : "Could not save the INI. See the log for why.";
				});
			}
			HelpMarker("Writes the price above back to the INI. Comments and unrelated keys are left alone.");

			ImGuiMCP::SameLine();

			if (ImGuiMCP::Button("Reload from INI"))
			{
				OnMainThread([]() {
					statusMessage = settings::Reload()
										 ? "Settings reloaded from the INI."
										 : "Could not read the INI. See the log for why.";
				});
			}
			HelpMarker("Throws away any change made here since the last save and re-reads the INI from disk. Also picks up edits made to the file by hand.");

			ImGuiMCP::SameLine();

			if (ImGuiMCP::Button("Restore defaults"))
			{
				OnMainThread([]() { settings::RestoreDefaults(); });

				statusMessage = "Defaults restored. Press Save to keep them.";
			}
			HelpMarker("Puts the price back to 10 dragon souls per perk point, the original mod's own default. Nothing is written until you press Save. Never affects perk points or dragon souls you already have.");

			if (!statusMessage.empty())
			{
				ImGuiMCP::TextWrapped("%s", statusMessage.c_str());
			}

			ImGuiMCP::Spacing();
			ImGuiMCP::TextDisabled("%s", settings::GetIniPath().c_str());
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled())
		{
			logger::info("SKSE Menu Framework is not installed; settings will be read from the INI only");

			return;
		}

		if (!HasRequiredExports())
		{
			logger::warn("The installed SKSE Menu Framework is older than this plugin's settings "
						 "menu needs. Update it to a newer version to configure Ish's Souls to Perks in game.");

			return;
		}

		SKSEMenuFramework::SetSection("Ish's Souls to Perks");
		SKSEMenuFramework::AddSectionItem("Settings", SettingsPanel::Render);

		logger::info("Registered the settings page with SKSE Menu Framework");
	}

	void __stdcall SettingsPanel::Render()
	{
		ImGuiMCP::TextWrapped("Buy perk points with dragon souls. The price applies as soon as you change it; press Save to keep it for next time.");
		ImGuiMCP::Spacing();

		ImGuiMCP::PushItemWidth(260.0F);

		RenderCostSection();
		ImGuiMCP::Spacing();

		RenderPurchaseSection();
		ImGuiMCP::Spacing();

		RenderDebugSection();
		ImGuiMCP::Spacing();

		ImGuiMCP::PopItemWidth();

		RenderButtons();
	}
}
