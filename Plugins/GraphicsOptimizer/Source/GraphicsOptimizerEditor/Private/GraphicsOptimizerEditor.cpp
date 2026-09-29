#include "Modules/ModuleManager.h"
#include "Framework/Docking/TabManager.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "SGraphicsOptimizerPanel.h"

static const FName GraphicsOptimizerTabName(TEXT("GraphicsOptimizerTab"));

class FGraphicsOptimizerEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        FGlobalTabmanager::Get()->RegisterNomadTabSpawner(GraphicsOptimizerTabName,
            FOnSpawnTab::CreateRaw(this, &FGraphicsOptimizerEditorModule::SpawnTab))
            .SetDisplayName(FText::FromString(TEXT("Graphics Optimizer")))
            .SetMenuType(ETabSpawnerMenuType::Hidden);
        UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FGraphicsOptimizerEditorModule::RegisterMenus));
    }

    virtual void ShutdownModule() override
    {
        if (UToolMenus::IsToolMenuUIEnabled()) UToolMenus::UnRegisterStartupCallback(this);
        UToolMenus::UnregisterOwner(this);
        FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(GraphicsOptimizerTabName);
    }

private:
    TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs&)
    {
        return SNew(SDockTab).TabRole(ETabRole::NomadTab)
            [ SNew(SGraphicsOptimizerPanel) ];
    }

    void RegisterMenus()
    {
        FToolMenuOwnerScoped Owner(this);
        UToolMenu* Menu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools"));
        FToolMenuSection& Section = Menu->FindOrAddSection(TEXT("GraphicsOptimizer"));
        Section.AddMenuEntry(
            TEXT("OpenGraphicsOptimizer"),
            FText::FromString(TEXT("Graphics Optimizer")),
            FText::FromString(TEXT("Open the graphics benchmark panel.")),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateLambda([]()
            {
                FGlobalTabmanager::Get()->TryInvokeTab(GraphicsOptimizerTabName);
            })));
    }
};

IMPLEMENT_MODULE(FGraphicsOptimizerEditorModule, GraphicsOptimizerEditor)
