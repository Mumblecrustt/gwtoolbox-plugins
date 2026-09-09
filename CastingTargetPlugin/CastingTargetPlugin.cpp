#include "CastingTargetPlugin.h"

#include <GWCA/Constants/Constants.h>
#include <GWCA/GameEntities/Agent.h>
#include <GWCA/Managers/AgentMgr.h>
#include <GWCA/Managers/GameThreadMgr.h>
#include <GWCA/Utilities/Hook.h>

#include <Utils/GuiUtils.h>
#include <Timer.h>

namespace {
    bool show_distance_sorted = true;

    // Keep one entry per enemy that stops casting.
    // This lets a button stay visible for a short time after the cast ends.
    struct CastingEnemy {
        GW::AgentID agent_id = 0;
        clock_t last_seen_casting = 0;
    };

    std::vector<CastingEnemy> tracked_enemies;
    constexpr float kButtonLingerSeconds = 1.5f;
}

DLLAPI ToolboxPlugin* ToolboxPluginInstance()
{
    static CastingTargetPlugin instance;
    return &instance;
}

void CastingTargetPlugin::DrawSettings()
{
    if (!toolbox_handle) {
        return;
    }
    ImGui::Checkbox("Sort buttons by distance", &show_distance_sorted);
}

void CastingTargetPlugin::LoadSettings(const wchar_t* folder)
{
    ToolboxPlugin::LoadSettings(folder);
    LoadSetting("show_distance_sorted", show_distance_sorted);
}

void CastingTargetPlugin::SaveSettings(const wchar_t* folder)
{
    SaveSetting("show_distance_sorted", show_distance_sorted);
    ToolboxPlugin::SaveSettings(folder);
}

void CastingTargetPlugin::Initialize(ImGuiContext* ctx, const ImGuiAllocFns allocator_fns, const HMODULE toolbox_dll)
{
    ToolboxPlugin::Initialize(ctx, allocator_fns, toolbox_dll);
}

void CastingTargetPlugin::SignalTerminate()
{
    ToolboxPlugin::SignalTerminate();
    tracked_enemies.clear();
}

bool CastingTargetPlugin::CanTerminate()
{
    return true;
}

void CastingTargetPlugin::Draw(IDirect3DDevice9*)
{
    const GW::AgentArray* agents = GW::Agents::GetAgentArray();
    const GW::Agent* player = agents ? GW::Agents::GetObservingAgent() : nullptr;

    if (player && agents) {
        for (auto* agent : *agents) {
            auto* living = agent ? agent->GetAsAgentLiving() : nullptr;
            if (!living || living->allegiance != GW::Constants::Allegiance::Enemy || !living->GetIsAlive()) {
                continue;
            }

            const bool is_casting = living->skill != static_cast<uint16_t>(GW::Constants::SkillID::No_Skill);
            if (!is_casting) {
                continue;
            }

            const auto found = std::ranges::find_if(tracked_enemies, [living](const CastingEnemy& e) {
                return e.agent_id == living->agent_id;
            });

            if (found == tracked_enemies.end()) {
                tracked_enemies.push_back({living->agent_id, TIMER_INIT()});
            }
            else {
                found->last_seen_casting = TIMER_INIT();
            }
        }

        // Drop any tracked enemy that stopped casting past the linger time.
        std::erase_if(tracked_enemies, [](const CastingEnemy& e) {
            return TIMER_DIFF(e.last_seen_casting) / static_cast<float>(CLOCKS_PER_SEC) > kButtonLingerSeconds;
        });

        if (show_distance_sorted) {
            std::ranges::sort(tracked_enemies, [player](const CastingEnemy& a, const CastingEnemy& b) {
                const auto* la = GW::Agents::GetAgentByID(a.agent_id);
                const auto* lb = GW::Agents::GetAgentByID(b.agent_id);
                if (!la || !lb) {
                    return false;
                }
                return GW::GetSquareDistance(player->pos, la->pos) < GW::GetSquareDistance(player->pos, lb->pos);
            });
        }
    }
    else {
        tracked_enemies.clear();
    }

    ImGui::SetNextWindowSize(ImVec2(220, 0), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(Name(), GetVisiblePtr())) {
        if (tracked_enemies.empty()) {
            ImGui::TextDisabled("No enemy casts a spell right now.");
        }
        for (const auto& entry : tracked_enemies) {
            const auto* agent = GW::Agents::GetAgentByID(entry.agent_id);
            const auto* living = agent ? agent->GetAsAgentLiving() : nullptr;
            if (!living) {
                continue;
            }

            ImGui::PushID(entry.agent_id);
            if (ImGui::Button("Target", ImVec2(-1, 0))) {
                const GW::AgentID target_id = entry.agent_id;
                GW::GameThread::Enqueue([target_id] {
                    auto* fresh_agent = GW::Agents::GetAgentByID(target_id);
                    auto* fresh_living = fresh_agent ? fresh_agent->GetAsAgentLiving() : nullptr;
                    if (fresh_living) {
                        GW::Agents::ChangeTarget(fresh_living);
                    }
                });
            }
            ImGui::PopID();
        }
    }
    ImGui::End();
}
