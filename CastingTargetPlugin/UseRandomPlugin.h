#pragma once

#include <ToolboxUIPlugin.h>

class UseRandomPlugin : public ToolboxPlugin {
public:
    UseRandomPlugin() = default;
    ~UseRandomPlugin() override = default;

    const char* Name() const override { return "Use Random Item Plugin"; }

    void Initialize(ImGuiContext* ctx, ImGuiAllocFns allocator_fns, HMODULE toolbox_dll) override;
    void SignalTerminate() override;
    bool CanTerminate() override;
};
