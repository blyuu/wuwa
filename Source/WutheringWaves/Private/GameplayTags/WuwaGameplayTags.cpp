// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayTags/WuwaGameplayTags.h"


namespace WeaponTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Type_Sword, "Weapon.Type.Sword", "직검 무기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Type_Pistol, "Weapon.Type.Pistol", "권총 무기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Type_Rectifier, "Weapon.Type.Rectifier", "증폭기 무기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Type_Gauntlet, "Weapon.Type.Gauntlet", "권갑 무기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Type_Broadblade, "Weapon.Type.Broadblade", "대검 무기");
}

namespace ElementTags
{
	//Character Attribute
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_Element_Fusion, "Character.Element.Fusion",  "융용");		//융용
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_Element_Glacio, "Character.Element.Glacio", "응결");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_Element_Aero, "Character.Element.Aero",  "기류");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_Element_Electro, "Character.Element.Electro",  "전도");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_Element_Spectro, "Character.Element.Spectro",  "회절");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_Element_Havoc, "Character.Element.Havoc",  "인멸");
	
}

namespace AbilityTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Type_BaseAttack, "Ability.Type.BaseAttack", "일반공격");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Type_ResonanceSkill, "Ability.Type.ResonanceSkill", "공명 스킬");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Type_Liberation, "Ability.Type.Liberation", "공명 해방");
}
