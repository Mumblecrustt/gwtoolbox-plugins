#pragma once

#include <ToolboxUIPlugin.h>

// This plugin lists enemies that cast a spell right now.
// Each enemy gets one button. Press the button to target that enemy.
class CastingTargetPlugin : public ToolboxPlugin {
public:
    CastingTargetPlugin() = default;
    ~CastingTargetPlugin() override = default;

    const char* Name() const override { return "Casting Target Plugin"; }

    [[nodiscard]] bool HasSettings() const override { return true; }
    void DrawSettings() override;
    void LoadSettings(const wchar_t* folder) override;
    void SaveSettings(const wchar_t* folder) override;
    void Initialize(ImGuiContext* ctx, ImGuiAllocFns allocator_fns, HMODULE toolbox_dll) override;
    void SignalTerminate() override;
    bool CanTerminate() override;

    // Draw the button list. The Toolbox core calls this every frame.
    void Draw(IDirect3DDevice9* pDevice) override;
};
