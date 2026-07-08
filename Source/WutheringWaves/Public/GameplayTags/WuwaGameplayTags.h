// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace WeaponTags
{
	//Weapon Type
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Type_Sword);		//직검
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Type_Pistol);		//권총
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Type_Rectifier)	//증폭기
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Type_Gauntlet)	//권갑
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Type_Broadblade)	//대검
	
}

namespace ElementTags
{
	//Character Attribute
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Element_Fusion);		//융용
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Element_Glacio);		//응결
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Element_Aero);			//기류
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Element_Electro);		//전도
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Element_Spectro);		//회절
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Element_Havoc);		//인멸
	
}


namespace AbilityTags
{
	//Character Skills
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type_BaseAttack);			//일반 공격
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type_ResonanceSkill);		//공명 스킬
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type_Liberation);		//공명 해방
	
}
