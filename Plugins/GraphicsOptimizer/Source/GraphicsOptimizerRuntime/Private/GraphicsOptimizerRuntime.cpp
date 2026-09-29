#include "GraphicsOptimizerRuntime.h"

DEFINE_LOG_CATEGORY(LogGraphicsOptimizer);

void FGraphicsOptimizerRuntimeModule::StartupModule()
{
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Plugin] Runtime initialized."));
}

void FGraphicsOptimizerRuntimeModule::ShutdownModule()
{
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Plugin] Runtime shut down."));
}

IMPLEMENT_MODULE(FGraphicsOptimizerRuntimeModule, GraphicsOptimizerRuntime)
