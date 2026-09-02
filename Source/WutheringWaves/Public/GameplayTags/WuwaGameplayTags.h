// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace WeaponTags
{
	//Weapon Type
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Type_Sword);		//Sword
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Type_Pistol);		//Pistol
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Type_Rectifier)	//Rectifier
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Type_Gauntlet)	//Gauntlet
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Type_Broadblade)	//Broadblade

}

namespace ElementTags
{
	//Character Attribute
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Element_Fusion);		//Fusion
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Element_Glacio);		//Glacio
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Element_Aero);			//Aero
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Element_Electro);		//Electro
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Element_Spectro);		//Spectro
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Element_Havoc);		//Havoc

}


namespace AbilityTags
{
	//Character Skills
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type_BaseAttack);			//Normal attack
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type_ResonanceSkill);		//Resonance skill
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type_Liberation);		//Resonance liberation
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type_Dodge);				//Dodge / dash
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type_Intro);				//Charged-swap intro (flank appear)

}

namespace StateTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invulnerable);				//i-frame during dodge (blocks damage GE)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_State_Attacking);			//enemy is inside an attack danger window
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_State_Groggy);				//enemy is staggered/groggy (takes extra damage)
}

namespace DataTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage);					//SetByCaller magnitude key: flat damage amount
}

namespace EventTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Attack_Hit);			//Attack hit event (base attack / skill / ultimate)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_EnemyAttack_Hit);			//Enemy attack hit event
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Combo_WindowOpen);			//Normal attack combo event
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Skill_Voice);				//play a random skill voice line
}

namespace RangeTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Range_Melee);				//Melee character
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Range_Ranged);             //Ranged character
}


namespace EnemyTypeTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Type_Calamity);							//Calamity class monster
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Type_Common);								//Common class monster
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Type_Elite)								//Elite class monster
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Type_Overload)								//Overload class monster
}

namespace EnemyAbilityTags
{
	//Enemy Skills (Skills map key = attack pattern)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Ability_Attack);							//Enemy basic attack
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Ability_Heavy);							//Heavy attack pattern
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Enemy_Ability_Charge);							//Charge pattern
}
