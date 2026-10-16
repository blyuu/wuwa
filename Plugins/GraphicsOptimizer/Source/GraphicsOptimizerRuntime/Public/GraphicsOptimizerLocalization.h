#pragma once

#include "CoreMinimal.h"

// Lightweight, plugin-owned EN/KO text switch. Independent of the editor's
// display-language setting so the panel can offer its own toggle regardless
// of how the host project's editor locale is configured.
enum class EGraphicsOptimizerLanguage : uint8
{
    English,
    Korean
};

class GRAPHICSOPTIMIZERRUNTIME_API FGraphicsOptimizerLocalization
{
public:
    static EGraphicsOptimizerLanguage GetLanguage();
    static void SetLanguage(EGraphicsOptimizerLanguage NewLanguage);
    static void ToggleLanguage();

    // Returns the Korean text loaded for EnglishText (from
    // Resources/Localization/ko.json) when the current language is Korean
    // and a translation exists; otherwise returns EnglishText.
    static FString Translate(const TCHAR* EnglishText);

    // FString::Printf requires its format argument to be a compile-time literal
    // (enforced by a static_assert), so it cannot take a runtime-selected
    // (possibly translated) format string. Use this instead of
    // FString::Printf(GOTEXT("..."), ...) wherever the format string itself
    // needs to be localized: EnglishFormat is translated first, then the
    // result is used as the sprintf-style format with the given arguments.
    static FString VARARGS Printf(const TCHAR* EnglishFormat, ...);
};

// Use where a TCHAR* is needed - e.g. one branch of a ternary feeding a %s
// argument, a plain FString assignment/concatenation, or an FText::FromString
// argument. EnglishLiteral is a string literal (TEXT() is added for you). The
// returned pointer is only valid for the rest of the current full
// expression/statement - never store it in a variable or return it from a
// function. Do NOT pass GOTEXT(...) as the format argument of
// FString::Printf/Appendf - that argument must be a compile-time literal;
// use FGraphicsOptimizerLocalization::Printf(TEXT("..."), ...) instead.
#define GOTEXT(EnglishLiteral) (*FGraphicsOptimizerLocalization::Translate(TEXT(EnglishLiteral)))

// Same as GOTEXT but for an expression that is already a const TCHAR*
// (e.g. the result of another helper function), rather than a literal.
#define GOTR(TCharExpr) (*FGraphicsOptimizerLocalization::Translate(TCharExpr))

// Use where an FString is needed, e.g. inside FText::FromString(...) or string concatenation.
#define GOSTR(EnglishLiteral) (FGraphicsOptimizerLocalization::Translate(TEXT(EnglishLiteral)))
