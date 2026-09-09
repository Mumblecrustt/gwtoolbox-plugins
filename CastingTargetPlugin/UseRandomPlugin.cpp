#include "UseRandomPlugin.h"

#include <GWCA/Constants/Constants.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Managers/ItemMgr.h>
#include <GWCA/Managers/ChatMgr.h>
#include <GWCA/GameEntities/Item.h>
#include <GWCA/Utilities/Hook.h>
#include <sstream>
#include <random>

namespace {
    GW::HookEntry UseRandomCmd_HookEntry;
}

DLLAPI ToolboxPlugin* ToolboxPluginInstance()
{
    static UseRandomPlugin instance;
    return &instance;
}

void UseRandomCmd(GW::HookStatus*, const wchar_t*, const int argc, const LPWSTR* argv)
{
    if (argc < 2) {
        GW::Chat::WriteChat(GW::Chat::CHANNEL_GLOBAL, L"Usage: /userandom <item_id1,item_id2,item_id3,...>", L"UseRandom Plugin");
        return;
    }

    // Parse comma-separated item IDs from the command argument
    std::vector<uint32_t> item_ids;
    std::wstringstream ss(argv[1]);
    std::wstring token;
    
    while (std::getline(ss, token, L',')) {
        // Trim whitespace
        token.erase(0, token.find_first_not_of(L" \t\n\r"));
        token.erase(token.find_last_not_of(L" \t\n\r") + 1);
        if (!token.empty()) {
            try {
                uint32_t id = static_cast<uint32_t>(std::stoul(token));
                if (id > 0) {
                    item_ids.push_back(id);
                }
            } catch (...) {
                // Invalid number, skip
            }
        }
    }

    if (item_ids.empty()) {
        GW::Chat::WriteChat(GW::Chat::CHANNEL_GLOBAL, L"No valid item IDs provided", L"UseRandom Plugin");
        return;
    }

    // Pick a random item ID from the list (same logic as HotkeyUseItem::Execute)
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, item_ids.size() - 1);
    uint32_t selected_item_id = item_ids[dist(gen)];

    // Try to use the item (same logic as HotkeyUseItem::Execute)
    bool used = GW::Items::UseItemByModelId(selected_item_id, 1, 4);
    if (!used && GW::Map::GetInstanceType() == GW::Constants::InstanceType::Outpost) {
        used = GW::Items::UseItemByModelId(selected_item_id, 8, 16);
    }

    if (!used) {
        std::wstring msg = L"Item #" + std::to_wstring(selected_item_id) + L" not found!";
        GW::Chat::WriteChat(GW::Chat::CHANNEL_GLOBAL, msg.c_str(), L"UseRandom Plugin");
    } else {
        std::wstring msg = L"Used random item #" + std::to_wstring(selected_item_id);
        GW::Chat::WriteChat(GW::Chat::CHANNEL_GLOBAL, msg.c_str(), L"UseRandom Plugin");
    }
}

void UseRandomPlugin::Initialize(ImGuiContext* ctx, const ImGuiAllocFns allocator_fns, const HMODULE toolbox_dll)
{
    ToolboxPlugin::Initialize(ctx, allocator_fns, toolbox_dll);
    GW::Chat::CreateCommand(&UseRandomCmd_HookEntry, L"userandom", UseRandomCmd);
}

void UseRandomPlugin::SignalTerminate()
{
    ToolboxPlugin::SignalTerminate();
    GW::Chat::DeleteCommand(&UseRandomCmd_HookEntry);
}

bool UseRandomPlugin::CanTerminate()
{
    return true;
}
