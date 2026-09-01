// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WuwaBossHealthWidget.h"

#include "AbilitySystemComponent.h"
#include "GameAbilities/WuWa_AttributeSetBase.h"
#include "DataAsset/EnemyDataAsset.h"
#include "GameplayTags/WuwaGameplayTags.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UWuwaBossHealthWidget::SetAbilitySystemComponent(UAbilitySystemComponent* InASC)
{
	// Same target: just refresh the values and bail.
	if (BoundASC == InASC)
	{
		PushHealth();
		PushGroggy();
		return;
	}

	UnbindFromCurrentASC();

	BoundASC = InASC;
	if (!BoundASC)
	{
		return;
	}

	HpChangedHandle = BoundASC->GetGameplayAttributeValueChangeDelegate(
		UWuWa_AttributeSetBase::GetHpAttribute()).AddUObject(this, &UWuwaBossHealthWidget::HandleHealthChanged);
	MaxHpChangedHandle = BoundASC->GetGameplayAttributeValueChangeDelegate(
		UWuWa_AttributeSetBase::GetMaxHpAttribute()).AddUObject(this, &UWuwaBossHealthWidget::HandleMaxHealthChanged);
	GroggyChangedHandle = BoundASC->GetGameplayAttributeValueChangeDelegate(
		UWuWa_AttributeSetBase::GetGroggyAttribute()).AddUObject(this, &UWuwaBossHealthWidget::HandleGroggyChanged);
	MaxGroggyChangedHandle = BoundASC->GetGameplayAttributeValueChangeDelegate(
		UWuWa_AttributeSetBase::GetMaxGroggyAttribute()).AddUObject(this, &UWuwaBossHealthWidget::HandleMaxGroggyChanged);

	// Tag event tells us when the enemy enters/leaves the groggy (recovery) state.
	GroggyStateHandle = BoundASC->RegisterGameplayTagEvent(
		StateTags::Enemy_State_Groggy, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UWuwaBossHealthWidget::HandleGroggyStateChanged);

	// Delegates only fire on *future* changes, so seed the UI with the current state + values now.
	SetRecovering(BoundASC->HasMatchingGameplayTag(StateTags::Enemy_State_Groggy));
	PushHealth();
	PushGroggy();
}

void UWuwaBossHealthWidget::SetBossInfo(const UEnemyDataAsset* Data)
{
	if (!Data)
	{
		return;
	}

	if (BossNameText)
	{
		BossNameText->SetText(Data->DisplayName);
	}

	if (BossLevelText)
	{
		// "Lv. {n}" - C++ owns the whole string, so don't type a prefix into the WBP (it gets overwritten).
		BossLevelText->SetText(FText::Format(FText::FromString(TEXT("Lv. {0}")), Data->Level));
	}
}

void UWuwaBossHealthWidget::NativeDestruct()
{
	UnbindFromCurrentASC();
	Super::NativeDestruct();
}

void UWuwaBossHealthWidget::HandleHealthChanged(const FOnAttributeChangeData& /*Data*/)
{
	PushHealth();
}

void UWuwaBossHealthWidget::HandleMaxHealthChanged(const FOnAttributeChangeData& /*Data*/)
{
	PushHealth();
}

void UWuwaBossHealthWidget::HandleGroggyChanged(const FOnAttributeChangeData& /*Data*/)
{
	PushGroggy();
}

void UWuwaBossHealthWidget::HandleMaxGroggyChanged(const FOnAttributeChangeData& /*Data*/)
{
	PushGroggy();
}

void UWuwaBossHealthWidget::HandleGroggyStateChanged(const FGameplayTag /*Tag*/, int32 NewCount)
{
	SetRecovering(NewCount > 0);
}

void UWuwaBossHealthWidget::SetRecovering(bool bRecovering)
{
	// Same bar, different color: white while draining, yellow while the enemy recovers from a groggy break.
	if (GroggyBar)
	{
		GroggyBar->SetFillColorAndOpacity(bRecovering ? GroggyRecoverColor : GroggyBarColor);
	}

	PushGroggy();
}

void UWuwaBossHealthWidget::PushHealth()
{
	if (!BoundASC || !HealthBar)
	{
		return;
	}

	const float Hp = BoundASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetHpAttribute());
	const float MaxHp = BoundASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetMaxHpAttribute());
	HealthBar->SetPercent(MaxHp > 0.f ? Hp / MaxHp : 0.f);
}

void UWuwaBossHealthWidget::PushGroggy()
{
	if (!BoundASC || !GroggyBar)
	{
		return;
	}

	const float Groggy = BoundASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetGroggyAttribute());
	const float MaxGroggy = BoundASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetMaxGroggyAttribute());
	GroggyBar->SetPercent(MaxGroggy > 0.f ? Groggy / MaxGroggy : 0.f);
}

void UWuwaBossHealthWidget::UnbindFromCurrentASC()
{
	if (BoundASC)
	{
		BoundASC->GetGameplayAttributeValueChangeDelegate(UWuWa_AttributeSetBase::GetHpAttribute()).Remove(HpChangedHandle);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UWuWa_AttributeSetBase::GetMaxHpAttribute()).Remove(MaxHpChangedHandle);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UWuWa_AttributeSetBase::GetGroggyAttribute()).Remove(GroggyChangedHandle);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UWuWa_AttributeSetBase::GetMaxGroggyAttribute()).Remove(MaxGroggyChangedHandle);
		BoundASC->RegisterGameplayTagEvent(StateTags::Enemy_State_Groggy, EGameplayTagEventType::NewOrRemoved).Remove(GroggyStateHandle);
	}

	HpChangedHandle.Reset();
	MaxHpChangedHandle.Reset();
	GroggyChangedHandle.Reset();
	MaxGroggyChangedHandle.Reset();
	GroggyStateHandle.Reset();
	BoundASC = nullptr;
}
