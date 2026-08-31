// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayTags/WuwaGameplayTags.h"


namespace WeaponTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Type_Sword, "Weapon.Type.Sword", "Sword weapon");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Type_Pistol, "Weapon.Type.Pistol", "Pistol weapon");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Type_Rectifier, "Weapon.Type.Rectifier", "Rectifier weapon");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Type_Gauntlet, "Weapon.Type.Gauntlet", "Gauntlet weapon");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Type_Broadblade, "Weapon.Type.Broadblade", "Broadblade weapon");
}

namespace ElementTags
{
	//Character Attribute
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_Element_Fusion, "Character.Element.Fusion",  "Fusion");		//Fusion
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_Element_Glacio, "Character.Element.Glacio", "Glacio");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_Element_Aero, "Character.Element.Aero",  "Aero");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_Element_Electro, "Character.Element.Electro",  "Electro");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_Element_Spectro, "Character.Element.Spectro",  "Spectro");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_Element_Havoc, "Character.Element.Havoc",  "Havoc");

}

namespace AbilityTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Type_BaseAttack, "Ability.Type.BaseAttack", "Normal attack");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Type_ResonanceSkill, "Ability.Type.ResonanceSkill", "Resonance skill");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Type_Liberation, "Ability.Type.Liberation", "Resonance liberation");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Type_Dodge, "Ability.Type.Dodge", "Dodge / dash");
}

namespace StateTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Invulnerable, "State.Invulnerable", "Immune to damage (dodge i-frame)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_State_Attacking, "Enemy.State.Attacking", "Enemy is inside an attack danger window");
}

namespace EventTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_BaseAttack_Hit, "Event.BaseAttack.Hit", "Normal attack hit");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_EnemyAttack_Hit, "Event.EnemyAttack.Hit", "Enemy attack hit");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Combo_WindowOpen, "Event.Combo.WindowOpen", "Normal attack combo event");
}

namespace RangeTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_Range_Melee, "Character.Range.Melee", "Melee character");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_Range_Ranged, "Character.Range.Ranged", "Ranged character");
}

namespace EnemyTypeTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_Type_Calamity, "Enemy.Type.Calamity", "Calamity class");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_Type_Common, "Enemy.Type.Common", "Common class");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_Type_Elite, "Enemy.Type.Elite", "Elite class");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_Type_Overload, "Enemy.Type.Overload", "Overload class");

}

namespace EnemyAbilityTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_Ability_Attack, "Enemy.Ability.Attack", "Enemy basic attack");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_Ability_Heavy, "Enemy.Ability.Heavy", "Heavy attack pattern");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_Ability_Charge, "Enemy.Ability.Charge", "Charge pattern");
}
