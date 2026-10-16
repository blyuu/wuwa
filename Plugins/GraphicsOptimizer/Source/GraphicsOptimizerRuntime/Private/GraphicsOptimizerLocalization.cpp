#include "GraphicsOptimizerLocalization.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CString.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Interfaces/IPluginManager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
TMap<FString, FString>& GetTranslationMap()
{
    static TMap<FString, FString> Map;
    return Map;
}

bool bGTranslationsLoaded = false;

// Loads Resources/Localization/ko.json from this plugin's own directory, a flat
// { "English": "Korean", ... } object. Missing/invalid file just leaves the map
// empty, so Translate() falls back to returning the English text unchanged.
void EnsureTranslationsLoaded()
{
    if (bGTranslationsLoaded) return;
    bGTranslationsLoaded = true;

    const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("GraphicsOptimizer"));
    if (!Plugin.IsValid()) return;

    const FString JsonPath = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources"), TEXT("Localization"), TEXT("ko.json"));
    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *JsonPath)) return;

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return;

    TMap<FString, FString>& Map = GetTranslationMap();
    for (const auto& Field : Root->Values)
    {
        FString Value;
        if (Field.Value.IsValid() && Field.Value->TryGetString(Value))
        {
            Map.Add(Field.Key, Value);
        }
    }
}

EGraphicsOptimizerLanguage GCurrentLanguage = EGraphicsOptimizerLanguage::English;
bool bGLanguageLoaded = false;

void EnsureLanguageLoaded()
{
    if (bGLanguageLoaded) return;
    bGLanguageLoaded = true;
#if WITH_EDITOR
    FString Saved;
    if (GConfig && GConfig->GetString(TEXT("GraphicsOptimizer"), TEXT("Language"), Saved, GEditorPerProjectIni))
    {
        GCurrentLanguage = (Saved == TEXT("Korean")) ? EGraphicsOptimizerLanguage::Korean : EGraphicsOptimizerLanguage::English;
    }
#endif
}
}

EGraphicsOptimizerLanguage FGraphicsOptimizerLocalization::GetLanguage()
{
    EnsureLanguageLoaded();
    return GCurrentLanguage;
}

void FGraphicsOptimizerLocalization::SetLanguage(EGraphicsOptimizerLanguage NewLanguage)
{
    EnsureLanguageLoaded();
    GCurrentLanguage = NewLanguage;
#if WITH_EDITOR
    if (GConfig)
    {
        GConfig->SetString(TEXT("GraphicsOptimizer"), TEXT("Language"),
            NewLanguage == EGraphicsOptimizerLanguage::Korean ? TEXT("Korean") : TEXT("English"), GEditorPerProjectIni);
        GConfig->Flush(false, GEditorPerProjectIni);
    }
#endif
}

void FGraphicsOptimizerLocalization::ToggleLanguage()
{
    SetLanguage(GetLanguage() == EGraphicsOptimizerLanguage::English
        ? EGraphicsOptimizerLanguage::Korean
        : EGraphicsOptimizerLanguage::English);
}

FString FGraphicsOptimizerLocalization::Translate(const TCHAR* EnglishText)
{
    if (GetLanguage() == EGraphicsOptimizerLanguage::Korean)
    {
        EnsureTranslationsLoaded();
        if (const FString* Found = GetTranslationMap().Find(FString(EnglishText)))
        {
            return *Found;
        }
    }
    return FString(EnglishText);
}

FString VARARGS FGraphicsOptimizerLocalization::Printf(const TCHAR* EnglishFormat, ...)
{
    const FString TranslatedFormat = Translate(EnglishFormat);

    int32 BufferSize = 512;
    for (;;)
    {
        TArray<TCHAR> Buffer;
        Buffer.SetNumUninitialized(BufferSize);
        const TCHAR* FmtPtr = *TranslatedFormat;
        va_list ArgPtr;
        va_start(ArgPtr, EnglishFormat);
        const int32 Result = FCString::GetVarArgs(Buffer.GetData(), BufferSize, FmtPtr, ArgPtr);
        va_end(ArgPtr);
        if (Result >= 0 && Result < BufferSize)
        {
            return FString::ConstructFromPtrSize(Buffer.GetData(), Result);
        }
        BufferSize *= 2;
    }
}
