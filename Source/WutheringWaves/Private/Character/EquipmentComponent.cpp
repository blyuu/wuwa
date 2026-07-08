// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/EquipmentComponent.h"

UEquipmentComponent::UEquipmentComponent()
{
	
	PrimaryComponentTick.bCanEverTick = true;
	
}



void UEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}



void UEquipmentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

