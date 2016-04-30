#include "stdafx.h"
#include "dispatcher.h"
#include "d20.h"
#include "temple_functions.h"
#include "obj.h"
#include "condition.h"
#include "bonus.h"
#include "action_sequence.h"
#include "turn_based.h"
#include "damage.h"
#include "util/fixes.h"
#include "gamesystems/objects/objsystem.h"
#include "gamesystems/gamesystems.h"
#include "ui/ui_party.h"

// Dispatcher System Function Replacements
class DispatcherReplacements : public TempleFix {
public:

	void apply() override {
		logger->info("Replacing basic Dispatcher functions");
		
		replaceFunction(0x1004D700, _DispIoCheckIoType1);
		replaceFunction(0x1004D720, _DispIoCheckIoType2);
		replaceFunction(0x1004D760, _DispIoCheckIoType4);
		replaceFunction(0x1004D780, _DispIoCheckIoType5);
		replaceFunction(0x1004D7A0, _DispIoCheckIoType6);
		replaceFunction(0x1004D7C0, _DispIoCheckIoType7);
		replaceFunction(0x1004D7E0, _DispIoCheckIoType8);
		replaceFunction(0x1004D800, _DispIoCheckIoType9);
		replaceFunction(0x1004D820, _DispIoCheckIoType10);
		replaceFunction(0x1004D840, _DispIoCheckIoType11);
		replaceFunction(0x1004D860, _DispIoCheckIoType12);
		replaceFunction(0x1004D8A0, _DispIoCheckIoType14);
		replaceFunction(0x1004F780, _PackDispatcherIntoObjFields);
		
		replaceFunction(0x100E1E30, _DispatcherRemoveSubDispNodes);
		replaceFunction(0x100E2400, _DispatcherClearField);
		replaceFunction(0x100E2720, _DispatcherClearPermanentMods);
		replaceFunction(0x100E2740, _DispatcherClearItemConds);
		replaceFunction(0x100E2760, _DispatcherClearConds);
		replaceFunction(0x100E2120, _DispatcherProcessor);
		replaceFunction(0x100E1F10, _DispatcherInit);
		replaceFunction(0x1004DBA0, DispIOType21Init);
		replaceFunction(0x1004D3A0, _Dispatch62);
		replaceFunction(0x1004D440, _Dispatch63);
		replaceFunction(0x1004DEC0, _DispatchAttackBonus);
		replaceFunction(0x1004E040, _DispatchDamage);
		replaceFunction(0x1004E790, _dispatchTurnBasedStatusInit); 

		replaceFunction(0x1004ED70, _dispatch1ESkillLevel); 

		replaceFunction<int(__cdecl)(D20Actn*, TurnBasedStatus*, enum_disp_type)>(0x1004E0D0, [](D20Actn* d20a, TurnBasedStatus* tbStat, enum_disp_type dispType)
		{
			return dispatch.DispatchD20ActionCheck(d20a, tbStat, dispType);
		});
		
 
	}
} dispatcherReplacements;

struct DispatcherSystemAddresses : temple::AddressTable
{
	int(__cdecl *DispatchAttackBonus)(objHndl attacker, objHndl victim, DispIoAttackBonus *dispIoIn, enum_disp_type, int key);
	DispatcherSystemAddresses()
	{
		rebase(DispatchAttackBonus, 0x1004DEC0);
	}
} addresses;

#pragma region Dispatcher System Implementation
DispatcherSystem dispatch;

void DispatcherSystem::DispatcherProcessor(Dispatcher* dispatcher, enum_disp_type dispType, uint32_t dispKey, DispIO* dispIO)
{
	_DispatcherProcessor(dispatcher, dispType, dispKey, dispIO);
}

Dispatcher * DispatcherSystem::DispatcherInit(objHndl objHnd)
{
	return _DispatcherInit(objHnd);
}

bool DispatcherSystem::dispatcherValid(Dispatcher* dispatcher)
{
	return (dispatcher != nullptr && dispatcher != (Dispatcher*)-1);
}

void  DispatcherSystem::DispatcherClearField(Dispatcher * dispatcher, CondNode ** dispCondList)
{
	_DispatcherClearField(dispatcher, dispCondList);
}

void  DispatcherSystem::DispatcherClearPermanentMods(Dispatcher * dispatcher)
{
	_DispatcherClearField(dispatcher, &dispatcher->permanentMods);
}

void  DispatcherSystem::DispatcherClearItemConds(Dispatcher * dispatcher)
{
	_DispatcherClearField(dispatcher, &dispatcher->itemConds);
}

void  DispatcherSystem::DispatcherClearConds(Dispatcher *dispatcher)
{
	_DispatcherClearConds(dispatcher);
}

int32_t DispatcherSystem::dispatch1ESkillLevel(objHndl objHnd, SkillEnum skill, BonusList* bonOut, objHndl objHnd2, int32_t flag)
{
	DispIoBonusAndObj dispIO;
	Dispatcher * dispatcher = objects.GetDispatcher(objHnd);
	if (!dispatcherValid(dispatcher)) return 0;

	dispIO.dispIOType = dispIOTypeSkillLevel;
	dispIO.returnVal = flag;
	dispIO.bonOut = bonOut;
	dispIO.obj = objHnd2;
	if (!bonOut)
	{
		dispIO.bonOut = &dispIO.bonlist;
		bonusSys.initBonusList(&dispIO.bonlist);
	}
	DispatcherProcessor(dispatcher, dispTypeSkillLevel, skill + 20, &dispIO);
	return bonusSys.getOverallBonus(dispIO.bonOut);
	
}

float DispatcherSystem::Dispatch29hGetMoveSpeed(objHndl objHnd, void* iO) // including modifiers like armor restirction
{
	float result = 30.0;
	uint32_t objHndLo = (uint32_t)(objHnd & 0xffffFFFF);
	uint32_t objHndHi = (uint32_t)((objHnd >>32) & 0xffffFFFF);
	macAsmProl;
	__asm{
		mov ecx, this;
		mov esi, [ecx]._Dispatch29hMovementSthg;
		mov eax, iO;
		push eax;
		mov eax, objHndHi;
		push eax;
		mov eax, objHndLo;
		push eax;
		call esi;
		add esp, 12;
		fstp result;
	}
	macAsmEpil
	return result;
}

void DispatcherSystem::dispIOTurnBasedStatusInit(DispIOTurnBasedStatus* dispIOtbStat)
{
	dispIOtbStat->dispIOType = dispIOTypeTurnBasedStatus;
	dispIOtbStat->tbStatus = nullptr;
}


void DispatcherSystem::dispatchTurnBasedStatusInit(objHndl objHnd, DispIOTurnBasedStatus* dispIOtB)
{
	Dispatcher * dispatcher = objects.GetDispatcher(objHnd);
	if (dispatcherValid(dispatcher))
	{
		DispatcherProcessor(dispatcher, dispTypeTurnBasedStatusInit, 0, dispIOtB);
		if (dispIOtB->tbStatus)
		{
			if (dispIOtB->tbStatus->hourglassState > 0)
			{
				d20Sys.d20SendSignal(objHnd, DK_SIG_BeginTurn, 0, 0);
			}
		}
	}
}


DispIoCondStruct* DispatcherSystem::DispIoCheckIoType1(DispIoCondStruct* dispIo)
{
	if (dispIo->dispIOType != dispIoTypeCondStruct) return nullptr;
	return dispIo;
}

DispIoBonusList* DispatcherSystem::DispIoCheckIoType2(DispIoBonusList* dispIo)
{
	if (dispIo->dispIOType != dispIOTypeBonusList) return nullptr;
	return dispIo;
}

DispIoBonusList* DispatcherSystem::DispIoCheckIoType2(DispIO* dispIoBonusList)
{
	return DispIoCheckIoType2( (DispIoBonusList*) dispIoBonusList);
}

DispIoSavingThrow* DispatcherSystem::DispIoCheckIoType3(DispIoSavingThrow* dispIo)
{
	if (dispIo->dispIOType != dispIOTypeSavingThrow) return nullptr;
	return dispIo;
}

DispIoSavingThrow* DispatcherSystem::DispIoCheckIoType3(DispIO* dispIo)
{
	return DispIoCheckIoType3((DispIoSavingThrow*)dispIo);
}

DispIoDamage* DispatcherSystem::DispIoCheckIoType4(DispIoDamage* dispIo)
{
	if (dispIo->dispIOType != dispIOTypeDamage) return nullptr;
	return dispIo;
}

DispIoDamage* DispatcherSystem::DispIoCheckIoType4(DispIO* dispIo)
{
	return DispIoCheckIoType4((DispIoDamage*)dispIo);
}

DispIoAttackBonus* DispatcherSystem::DispIoCheckIoType5(DispIoAttackBonus* dispIo)
{
	if (dispIo->dispIOType != dispIOTypeAttackBonus) return nullptr;
	return dispIo;
}

DispIoAttackBonus* DispatcherSystem::DispIoCheckIoType5(DispIO* dispIo)
{
	return DispIoCheckIoType5((DispIoAttackBonus*)dispIo);
}

DispIoD20Signal* DispatcherSystem::DispIoCheckIoType6(DispIoD20Signal* dispIo)
{
	if (dispIo->dispIOType != dispIoTypeSendSignal) return nullptr;
	return dispIo;
}

DispIoD20Signal* DispatcherSystem::DispIoCheckIoType6(DispIO* dispIo)
{
	return DispIoCheckIoType6((DispIoD20Signal*)dispIo);
}

DispIoD20Query* DispatcherSystem::DispIoCheckIoType7(DispIoD20Query* dispIo)
{
	if (dispIo->dispIOType != dispIOTypeQuery) return nullptr;
	return dispIo;
}

DispIoD20Query* DispatcherSystem::DispIoCheckIoType7(DispIO* dispIo)
{
	return DispIoCheckIoType7((DispIoD20Query*)dispIo);
}

DispIOTurnBasedStatus* DispatcherSystem::DispIoCheckIoType8(DispIOTurnBasedStatus* dispIo)
{
	if (dispIo->dispIOType != dispIOTypeTurnBasedStatus) return nullptr;
	return dispIo;
}

DispIOTurnBasedStatus* DispatcherSystem::DispIoCheckIoType8(DispIO* dispIo)
{
	return DispIoCheckIoType8((DispIOTurnBasedStatus*)dispIo);
}

DispIoTooltip* DispatcherSystem::DispIoCheckIoType9(DispIoTooltip* dispIo)
{
	if (dispIo->dispIOType != dispIoTypeTooltip) return nullptr;
	return dispIo;
}

DispIoTooltip* DispatcherSystem::DispIoCheckIoType9(DispIO* dispIo)
{
	if (dispIo->dispIOType != dispIoTypeTooltip) return nullptr;
	return static_cast<DispIoTooltip*>(dispIo);
}

DispIoBonusAndObj* DispatcherSystem::DispIoCheckIoType10(DispIoBonusAndObj* dispIo)
{
	if (dispIo->dispIOType != dispIOTypeSkillLevel) return nullptr;
	return dispIo;
}

DispIoBonusAndObj* DispatcherSystem::DispIoCheckIoType10(DispIO* dispIo)
{
	if (dispIo->dispIOType != dispIOTypeSkillLevel) return nullptr;
	return static_cast<DispIoBonusAndObj*>(dispIo);
}

DispIoDispelCheck* DispatcherSystem::DispIOCheckIoType11(DispIoDispelCheck* dispIo)
{
	if (dispIo->dispIOType != dispIOTypeDispelCheck) return nullptr;
	return dispIo;
}

DispIoD20ActionTurnBased* DispatcherSystem::DispIoCheckIoType12(DispIoD20ActionTurnBased* dispIo)
{
	if (dispIo->dispIOType != dispIOTypeD20ActionTurnBased) return nullptr;
	return dispIo;
}

DispIoD20ActionTurnBased* DispatcherSystem::DispIoCheckIoType12(DispIO* dispIo)
{
	return DispIoCheckIoType12((DispIoD20ActionTurnBased*)dispIo);
}

DispIoMoveSpeed * DispatcherSystem::DispIOCheckIoType13(DispIoMoveSpeed* dispIo)
{
	if (dispIo->dispIOType != dispIOTypeMoveSpeed) return nullptr;
	return dispIo;
}

DispIoMoveSpeed* DispatcherSystem::DispIOCheckIoType13(DispIO* dispIo)
{
	return DispIOCheckIoType13((DispIoMoveSpeed*)dispIo);
}

void DispatcherSystem::Dispatch48BeginRound(objHndl obj, int numRounds) const
{
	auto dispatcher = gameSystems->GetObj().GetObject(obj)->GetDispatcher();
	if (dispatcher->IsValid()) {
		DispIoD20Signal dispIo;
		dispIo.data1 = 1; // num rounds
		dispatch.DispatcherProcessor(dispatcher, dispTypeBeginRound, DK_NONE, &dispIo);
		static void(*onBeginRoundSpell)(objHndl) = temple::GetRef<void(__cdecl)(objHndl)>(0x100766E0);
		onBeginRoundSpell(obj);
	}
}

bool DispatcherSystem::Dispatch64ImmunityCheck(objHndl handle, DispIoImmunity* dispIo)
{
	auto dispatcher = gameSystems->GetObj().GetObject(handle)->GetDispatcher();
	if (dispatcher->IsValid())
	{
		DispatcherProcessor(dispatcher, dispTypeSpellImmunityCheck, 0, dispIo);
		return dispIo->returnVal;
	}
	
	return 0;
}

DispIoObjEvent* DispatcherSystem::DispIoCheckIoType17(DispIO* dispIo)
{
	if (dispIo->dispIOType != dispIoTypeObjEvent)
		return nullptr;
	return static_cast<DispIoObjEvent*>(dispIo);
}

DispIoImmunity* DispatcherSystem::DispIoCheckIoType23(DispIoImmunity* dispIo)
{
	if (dispIo->dispIOType != dispIOTypeImmunityHandler) return nullptr;
	return dispIo;
}

DispIoImmunity* DispatcherSystem::DispIoCheckIoType23(DispIO* dispIo)
{
	return DispIoCheckIoType23((DispIoImmunity*)dispIo);
}

DispIoEffectTooltip* DispatcherSystem::DispIoCheckIoType24(DispIoEffectTooltip* dispIo)
{
	if (dispIo->dispIOType != dispIOTypeEffectTooltip) return nullptr;
	return dispIo;
}

DispIoEffectTooltip* DispatcherSystem::DispIoCheckIoType24(DispIO* dispIo)
{
	return DispIoCheckIoType24((DispIoEffectTooltip*)dispIo);
}

DispIOBonusListAndSpellEntry* DispatcherSystem::DispIOCheckIoType14(DispIOBonusListAndSpellEntry* dispIo)
{
	if (dispIo->dispIOType != dispIoTypeBonusListAndSpellEntry) return nullptr;
	return dispIo;
}

void DispatcherSystem::PackDispatcherIntoObjFields(objHndl objHnd, Dispatcher* dispatcher)
{
	int numArgs;
	int k;
	int hashkey;
	int numConds;
	int condArgs[64] = {0,};
	const char* name = description.getDisplayName(objHnd);

	auto obj = objSystem->GetObject(objHnd);

	k = 0;
	d20Sys.d20SendSignal(objHnd, DK_SIG_Pack, 0, 0);
	obj->ResetField(obj_f_conditions);
	obj->ResetField(obj_f_condition_arg0);
	obj->ClearArray(obj_f_permanent_mods);
	obj->ClearArray(obj_f_permanent_mod_data);
	numConds = conds.GetActiveCondsNum(dispatcher);
	for (int i = 0; i < numConds; i++)
	{
		numArgs = conds.ConditionsExtractInfo(dispatcher, i, &hashkey, condArgs);
		obj->SetInt32(obj_f_conditions, i, hashkey);
		for (int j = 0; j < numArgs; ++k) {
			obj->SetInt32(obj_f_condition_arg0, k, condArgs[j++]);
		}
			
	}
	k = 0;
	numConds = conds.GetPermanentModsAndItemCondCount(dispatcher);
	for (int i = 0; i < numConds; i++)
	{
		numArgs = conds.PermanentAndItemModsExtractInfo(dispatcher, i, &hashkey, condArgs);
		if (hashkey)
		{
			obj->SetInt32(obj_f_permanent_mods, i, hashkey);
			for (int j = 0; j < numArgs; ++k) {
				obj->SetInt32(obj_f_permanent_mod_data, k, condArgs[j++]);
			}
				
		}
	}
}

int DispatcherSystem::DispatchAttackBonus(objHndl objHnd, objHndl victim, DispIoAttackBonus* dispIo, enum_disp_type dispType, int key)
{
	Dispatcher * dispatcher = objects.GetDispatcher(objHnd);
	if (!dispatcherValid(dispatcher))
		return 0;
	DispIoAttackBonus dispIoLocal;
	DispIoAttackBonus * dispIoToUse = dispIo;
	if (!dispIo)
	{
		dispIoLocal.attackPacket.victim = victim;
		dispIoLocal.attackPacket.attacker = objHnd;
		dispIoLocal.attackPacket.d20ActnType = (D20ActionType)dispType; //  that's the original code, jones...
		dispIoLocal.attackPacket.ammoItem = 0i64;
		dispIoLocal.attackPacket.weaponUsed = 0i64;
		dispIoToUse = &dispIoLocal;
	}
	DispatcherProcessor(dispatcher, dispType, key, dispIoToUse);
	return bonusSys.getOverallBonus(&dispIoToUse->bonlist);

	// return addresses.DispatchAttackBonus(objHnd, victim, dispIo, dispType, key);
}

int DispatcherSystem::DispatchToHitBonusBase(objHndl objHndCaller, DispIoAttackBonus* dispIo)
{
	int key = 0;
	if (dispIo)
	{
		key = dispIo->attackPacket.dispKey;
	}
	return DispatchAttackBonus(objHndCaller, 0i64, dispIo, dispTypeToHitBonusBase, key);
}

int DispatcherSystem::DispatchGetSizeCategory(objHndl obj)
{
	Dispatcher * dispatcher = objects.GetDispatcher(obj);
	DispIoD20Query dispIo; 

	if (dispatcherValid(dispatcher))
	{
		dispIo.dispIOType = dispIOTypeQuery;
		dispIo.return_val = objects.getInt32(obj, obj_f_size);
		dispIo.data1 = 0;
		dispIo.data2 = 0;
		DispatcherProcessor(
			(Dispatcher *)dispatcher, dispTypeGetSizeCategory,	0, &dispIo);
		return dispIo.return_val;
	}
	return 0;
}

void DispatcherSystem::DispatchConditionRemove(Dispatcher* dispatcher, CondNode* cond)
{
	for (auto subDispNode = dispatcher->subDispNodes[dispTypeConditionRemove]; subDispNode; subDispNode = subDispNode->next)
	{
		if (subDispNode->subDispDef->dispKey == 0)
		{
			DispatcherCallbackArgs dca;
			dca.dispIO = nullptr;
			dca.dispType = dispTypeConditionRemove;
			dca.dispKey = 0;
			dca.objHndCaller = dispatcher->objHnd;
			dca.subDispNode = subDispNode;
			subDispNode->subDispDef->dispCallback(dca.subDispNode, dca.objHndCaller, dca.dispType, dca.dispKey, dca.dispIO);
		}
	}

	for (auto subDispNode = dispatcher->subDispNodes[dispTypeConditionRemove2]; subDispNode; subDispNode = subDispNode->next)
	{
		if (subDispNode->subDispDef->dispKey == 0)
		{
			DispatcherCallbackArgs dca;
			dca.dispIO = nullptr;
			dca.dispType = dispTypeConditionRemove2;
			dca.dispKey = 0;
			dca.objHndCaller = dispatcher->objHnd;
			dca.subDispNode = subDispNode;
			subDispNode->subDispDef->dispCallback(dca.subDispNode, dca.objHndCaller, dca.dispType, dca.dispKey, dca.dispIO);
		}
	}
	cond->flags |= 1;
}

unsigned DispatcherSystem::Dispatch35BaseCasterLevelModify(objHndl obj, SpellPacketBody* spellPkt)
{
	auto _dispatcher = objects.GetDispatcher(obj);
	if (!dispatch.dispatcherValid(_dispatcher))
		return 0;
	DispIoD20Query dispIo;
	dispIo.return_val = spellPkt->baseCasterLevel;
	dispIo.data1 = reinterpret_cast<uint32_t>(spellPkt);
	dispIo.data2 = 0;
	DispatcherProcessor(_dispatcher, dispTypeBaseCasterLevelMod, 0, &dispIo);
	spellPkt->baseCasterLevel = dispIo.return_val;
	return dispIo.return_val;
}

int DispatcherSystem::Dispatch45SpellResistanceMod(objHndl handle, DispIOBonusListAndSpellEntry* dispIo)
{
	auto dispatcher = gameSystems->GetObj().GetObject(handle)->GetDispatcher();
	if (dispatcher->IsValid())
	{
		BonusList bonlist;
		dispIo->bonList = &bonlist;
		DispatcherProcessor(dispatcher, dispTypeSpellResistanceMod, 0, dispIo);
		auto bonus = bonlist.GetEffectiveBonusSum();
		return bonus;
	}
	else
		return 0;
}

void DispatcherSystem::DispIoDamageInit(DispIoDamage* dispIoDamage)
{
	dispIoDamage->dispIOType = dispIOTypeDamage;
	damage.DamagePacketInit(&dispIoDamage->damage);
	dispIoDamage->attackPacket.attacker=0i64;
	dispIoDamage->attackPacket.victim = 0i64;
	dispIoDamage->attackPacket.dispKey = 0;
	*(int*)&dispIoDamage->attackPacket.flags = 0;
	dispIoDamage->attackPacket.weaponUsed = 0i64;
	dispIoDamage->attackPacket.ammoItem = 0i64;
	dispIoDamage->attackPacket.d20ActnType= D20A_NONE;

}

int32_t DispatcherSystem::DispatchDamage(objHndl objHnd, DispIoDamage* dispIoDamage, enum_disp_type dispType, D20DispatcherKey key)
{
	Dispatcher * dispatcher = objects.GetDispatcher(objHnd);
	if (!dispatch.dispatcherValid(dispatcher)) return 0;
	DispIoDamage * dispIo = dispIoDamage;
	DispIoDamage dispIoLocal;
	if (!dispIoDamage)
	{
		dispatch.DispIoDamageInit(&dispIoLocal);
		dispIo = &dispIoLocal;
	}
	logger->info("Dispatching damage event for {} - type {}, key {}, victim {}", 
		description.getDisplayName(objHnd), dispType, key, description.getDisplayName( dispIoDamage->attackPacket.victim) );
	dispatch.DispatcherProcessor(dispatcher, dispType, key, dispIo);
	return 1;
}

int DispatcherSystem::DispatchD20ActionCheck(D20Actn* d20a, TurnBasedStatus* turnBasedStatus, enum_disp_type dispType)
{
	auto dispatcher = objects.GetDispatcher(d20a->d20APerformer);
	if (dispatch.dispatcherValid(dispatcher))
	{
		DispIoD20ActionTurnBased dispIo(d20a);
		dispIo.tbStatus = turnBasedStatus;
		if (dispType == dispTypeGetBonusAttacks) {
			BonusList bonlist;
			dispIo.bonlist = &bonlist;
			dispatch.DispatcherProcessor(dispatcher, dispType, d20a->d20ActType + 75, &dispIo);
			auto bonval = bonlist.GetEffectiveBonusSum();
			return bonval + dispIo.returnVal;
		}
		else {
			dispatch.DispatcherProcessor(dispatcher, dispType, d20a->d20ActType + 75, &dispIo);
			return dispIo.returnVal;
		}
	}
	return 0;
}

int DispatcherSystem::Dispatch60GetAttackDice(objHndl obj, DispIoAttackDice* dispIo)
{
	BonusList localBonlist;
	if (!dispIo->bonlist)
		dispIo->bonlist = &localBonlist;

	Dispatcher * dispatcher = objects.GetDispatcher(obj);
	if (!dispatcherValid(dispatcher))
		return 0;
	if (dispIo->weapon)
	{
		int weaponDice = objects.getInt32(dispIo->weapon, obj_f_weapon_damage_dice);
		dispIo->dicePacked = weaponDice;
		dispIo->attackDamageType = (DamageType)objects.getInt32(dispIo->weapon, obj_f_weapon_attacktype);
	}
	DispatcherProcessor(dispatcher, dispTypeGetAttackDice, 0, dispIo);
	int overallBonus = bonusSys.getOverallBonus(dispIo->bonlist);
	Dice diceNew ;
	diceNew = diceNew.FromPacked(dispIo->dicePacked);
	int bonus = diceNew.GetModifier() + overallBonus;
	int diceType = diceNew.GetSides();
	int diceNum = diceNew.GetCount();
	Dice diceNew2(diceNum, diceType, bonus);
	return diceNew.ToPacked();


}

int DispatcherSystem::DispatchGetBonus(objHndl critter, DispIoBonusList* eventObj, enum_disp_type dispType, D20DispatcherKey key) {

	DispIoBonusList dispIo;
	if (!eventObj) {
		dispIo.dispIOType = dispIOTypeBonusList;
		dispIo.flags = 0;
		eventObj = &dispIo;
	}

	auto obj = objSystem->GetObject(critter);
	if (!obj->IsCritter()) {
		return 0;
	}

	auto dispatcher = obj->GetDispatcher();
	if (!dispatcher) {
		return 0;
	}

	DispatcherProcessor(dispatcher, dispType, key, eventObj);

	return eventObj->bonlist.GetEffectiveBonusSum();

}

void DispIO::AssertType(enum_dispIO_type eventObjType) const
{
	assert(dispIOType == eventObjType);
}
#pragma endregion





#pragma region Dispatcher Functions

void __cdecl _DispatcherRemoveSubDispNodes(Dispatcher * dispatcher, CondNode * cond)
{
	for (uint32_t i = 0; i < dispTypeCount; i++)
	{
		SubDispNode ** ppSubDispNode = &dispatcher->subDispNodes[i];
		while (*ppSubDispNode != nullptr)
		{
			if ((*ppSubDispNode)->condNode == cond)
			{
				SubDispNode * savedNext = (*ppSubDispNode)->next;
				free(*ppSubDispNode);
				*ppSubDispNode = savedNext;

			}
			else
			{
				ppSubDispNode = &((*ppSubDispNode)->next);
			}

		}
	}

	};


void __cdecl _DispatcherClearField(Dispatcher *dispatcher, CondNode ** dispCondList)
{
	CondNode * cond = *dispCondList;
	objHndl obj = dispatcher->objHnd;
	while (cond != nullptr)
	{
		SubDispNode * subDispNode_TypeRemoveCond = dispatcher->subDispNodes[2];
		CondNode * nextCond = cond->nextCondNode;

		while (subDispNode_TypeRemoveCond != nullptr)
		{

			SubDispDef * sdd = subDispNode_TypeRemoveCond->subDispDef;
			if (sdd->dispKey == 0 && (subDispNode_TypeRemoveCond->condNode->flags & 1) == 0
				&& subDispNode_TypeRemoveCond->condNode == cond)
			{
				sdd->dispCallback(subDispNode_TypeRemoveCond, obj, dispTypeConditionRemove, 0, nullptr);
			}
			subDispNode_TypeRemoveCond = subDispNode_TypeRemoveCond->next;
		}
		_DispatcherRemoveSubDispNodes(dispatcher, cond);
		free(cond);
		cond = nextCond;

	}
	*dispCondList = nullptr;
};

void __cdecl _DispatcherClearPermanentMods(Dispatcher *dispatcher)
{
	_DispatcherClearField(dispatcher, &dispatcher->permanentMods);
};

void __cdecl _DispatcherClearItemConds(Dispatcher *dispatcher)
{
	_DispatcherClearField(dispatcher, &dispatcher->itemConds);
};

void __cdecl _DispatcherClearConds(Dispatcher *dispatcher)
{
	_DispatcherClearField(dispatcher, &dispatcher->conditions);
};

DispIoCondStruct * _DispIoCheckIoType1(DispIoCondStruct * dispIo)
{
	return dispatch.DispIoCheckIoType1(dispIo);
}

DispIoBonusList* _DispIoCheckIoType2(DispIoBonusList* dispIo)
{
	return dispatch.DispIoCheckIoType2(dispIo);
}

DispIoSavingThrow* _DispIOCheckIoType3(DispIoSavingThrow* dispIo)
{
	return dispatch.DispIoCheckIoType3(dispIo);
}

DispIoDamage* _DispIoCheckIoType4(DispIoDamage* dispIo)
{
	return dispatch.DispIoCheckIoType4(dispIo);
}

DispIoAttackBonus* _DispIoCheckIoType5(DispIoAttackBonus* dispIo)
{
	return dispatch.DispIoCheckIoType5(dispIo);
}

DispIoD20Signal* _DispIoCheckIoType6(DispIoD20Signal* dispIo)
{
	return dispatch.DispIoCheckIoType6(dispIo);
}

DispIoD20Query* _DispIoCheckIoType7(DispIoD20Query* dispIo)
{
	return dispatch.DispIoCheckIoType7(dispIo);
}

DispIOTurnBasedStatus* _DispIoCheckIoType8(DispIOTurnBasedStatus* dispIo)
{
	return dispatch.DispIoCheckIoType8(dispIo);
}

DispIoTooltip* _DispIoCheckIoType9(DispIoTooltip* dispIo)
{
	return dispatch.DispIoCheckIoType9(dispIo);
}

DispIoBonusAndObj* _DispIoCheckIoType10(DispIoBonusAndObj* dispIo)
{
	return dispatch.DispIoCheckIoType10(dispIo);
}

DispIoDispelCheck* _DispIoCheckIoType11(DispIoDispelCheck* dispIo)
{
	return dispatch.DispIOCheckIoType11(dispIo);
}

DispIoD20ActionTurnBased* _DispIoCheckIoType12(DispIoD20ActionTurnBased* dispIo)
{
	return dispatch.DispIoCheckIoType12(dispIo);
};

DispIOBonusListAndSpellEntry* _DispIoCheckIoType14(DispIOBonusListAndSpellEntry* dispIO)
{
	return dispatch.DispIOCheckIoType14(dispIO);
}

void _PackDispatcherIntoObjFields(objHndl objHnd, Dispatcher* dispatcher)
{
	dispatch.PackDispatcherIntoObjFields(objHnd, dispatcher);
};

void _DispatcherProcessor(Dispatcher* dispatcher, enum_disp_type dispType, uint32_t dispKey, DispIO* dispIO) {
	static uint32_t dispCounter = 0;
	if (dispCounter > DISPATCHER_MAX) {
		logger->info("Dispatcher maximum recursion reached!");
		return;
	}
	dispCounter++;
	/*
	if (dispKey == DK_D20A_TOUCH_ATTACK || dispKey == DK_SIG_TouchAttack)
	{
		int asd = 1;
	}
	*/
	SubDispNode* subDispNode = dispatcher->subDispNodes[dispType];

	while (subDispNode != nullptr) {

		if ((subDispNode->subDispDef->dispKey == dispKey || subDispNode->subDispDef->dispKey == 0) && ((subDispNode->condNode->flags & 1) == 0)) {

			DispIoTypeImmunityTrigger dispIoImmunity;
			DispIOType21Init((DispIoTypeImmunityTrigger*)&dispIoImmunity);
			dispIoImmunity.condNode = (CondNode *)subDispNode->condNode;

			if (dispKey != DK_IMMUNITY_SPELL || dispType != dispTypeImmunityTrigger) {
				_Dispatch62(dispatcher->objHnd, (DispIO*)&dispIoImmunity, 10);
			}
			
			if (dispIoImmunity.interrupt == 1 && dispType != dispType63) {
				dispIoImmunity.interrupt = 0;
				dispIoImmunity.val2 = 10;
				_Dispatch63(dispatcher->objHnd, (DispIO*)&dispIoImmunity);
				if (dispIoImmunity.interrupt == 0) {
					subDispNode->subDispDef->dispCallback(subDispNode, dispatcher->objHnd, dispType, dispKey, (DispIO*)dispIO);
				}
			}
			else {
				subDispNode->subDispDef->dispCallback(subDispNode, dispatcher->objHnd, dispType, dispKey, (DispIO*)dispIO);
			}


		}

		subDispNode = subDispNode->next;
	}

	dispCounter--;
	/*
	if (dispCounter == 0)
	{
		int breakpointDummy = 1;
	}
	*/
	return;

}

int32_t _DispatchDamage(objHndl objHnd, DispIoDamage* dispIo, enum_disp_type dispType, D20DispatcherKey key)
{
	return dispatch.DispatchDamage(objHnd, dispIo, dispType, key);
}

int32_t _dispatch1ESkillLevel(objHndl objHnd, SkillEnum skill, BonusList* bonOut, objHndl objHnd2, int32_t flag)
{
	return dispatch.dispatch1ESkillLevel(objHnd, skill, bonOut, objHnd2, flag);
}


void DispIoEffectTooltip::Append(int effectTypeId, uint32_t spellEnum, const char* text) const
{
	BuffDebuffSub * bdbSub;
	if (effectTypeId >= 91)
	{
		if (effectTypeId >= 168)
		{
			/*
				Status effects (icons inside the portrait)
			*/
			if (bdb->innerCount >= 6)
				return;
			bdbSub = &bdb->innerStatuses[bdb->innerCount++];

		} 
		/*
			Debuffs (icons below the portrait)
		*/
		else
		{
			if (bdb->debuffCount >= 8)
				return;
			bdbSub = &bdb->debuffs[bdb->debuffCount++];
		}

	} 
	/*
		Buffs (icons above the portrait)
	*/
	else
	{
		if (bdb->buffCount >= 8)
			return;
		bdbSub = &bdb->buffs[bdb->buffCount++];
	}

	//copy the text
	bdbSub->effectTypeId = effectTypeId;
	bdbSub->spellEnum = spellEnum;
	if (text){
		bdbSub->text = new char[strlen(text) + 1];
		strcpy( const_cast<char*>(bdbSub->text), text);
	} else
	{
		bdbSub->text = nullptr;
	}
}

bool Dispatcher::IsValid()
{
	return dispatch.dispatcherValid(this);
}

void Dispatcher::Process(enum_disp_type dispType, D20DispatcherKey key, DispIO* dispIo){
	dispatch.DispatcherProcessor(this, dispType, key, dispIo);
}

Dispatcher* _DispatcherInit(objHndl objHnd) {
	Dispatcher* dispatcherNew = (Dispatcher *)malloc(sizeof(Dispatcher));
	memset(&dispatcherNew->subDispNodes, 0, dispTypeCount * sizeof(SubDispNode*));
	CondNode* condNode = *(conds.pCondNodeGlobal);
	while (condNode != nullptr) {
		_CondNodeAddToSubDispNodeArray(dispatcherNew, condNode);
		condNode = condNode->nextCondNode;
	}
	dispatcherNew->objHnd = objHnd;
	dispatcherNew->permanentMods = nullptr;
	dispatcherNew->itemConds = nullptr;
	dispatcherNew->conditions = nullptr;
	return dispatcherNew;
};

void DispIOType21Init(DispIoTypeImmunityTrigger* dispIO) {
	dispIO->dispIOType = dispIOType21;
	dispIO->interrupt = 0;
	dispIO->field_8 = 0;
	dispIO->field_C = 0;
	dispIO->SDDKey1 = 0;
	dispIO->val2 = 0;
	dispIO->okToAdd = 0;
	dispIO->condNode = nullptr;
}

void _dispatchTurnBasedStatusInit(objHndl objHnd, DispIOTurnBasedStatus* dispIOtB)
{
	dispatch.dispatchTurnBasedStatusInit(objHnd, dispIOtB);
}

int _DispatchAttackBonus(objHndl objHnd, objHndl victim, DispIoAttackBonus* dispIo, enum_disp_type dispType, int key)
{
	return dispatch.DispatchAttackBonus(objHnd, victim, dispIo, dispType, key);
};


uint32_t _Dispatch62(objHndl objHnd, DispIO* dispIO, uint32_t dispKey) {
	Dispatcher* dispatcher = (Dispatcher *)objects.GetDispatcher(objHnd);
	if (dispatcher != nullptr && (int32_t)dispatcher != -1) {
		_DispatcherProcessor(dispatcher, dispTypeImmunityTrigger, dispKey, dispIO);
	}
	return 0;
}


uint32_t _Dispatch63(objHndl objHnd, DispIO* dispIO) {
	Dispatcher* dispatcher = (Dispatcher *)objects.GetDispatcher(objHnd);
	if (dispatcher != nullptr && (int32_t)dispatcher != -1) {
		_DispatcherProcessor(dispatcher, dispType63, 0, dispIO);
	}
	return 0;
}


#pragma endregion 
SubDispDefNew::SubDispDefNew(){
	dispType = dispType0;
	dispKey = DK_NONE;
	dispCallback = nullptr;
	data1.sVal = 0;
	data2.sVal = 0;
}

SubDispDefNew::SubDispDefNew(enum_disp_type type, uint32_t key, int(* callback)(DispatcherCallbackArgs), CondStructNew* data1In, uint32_t data2In){
	dispType = type;
	dispKey = key;
	dispCallback = callback;
	data1.condStruct = data1In;
	data2.usVal = data2In;
}

SubDispDefNew::SubDispDefNew(enum_disp_type type, uint32_t key, int(* callback)(DispatcherCallbackArgs), uint32_t data1In, uint32_t data2In){
	dispType = type;
	dispKey = key;
	dispCallback = callback;
	data1.usVal = data1In;
	data2.usVal = data2In;
}

CondStructNew::CondStructNew(){
	numArgs = 0;
	condName = nullptr;
	memset(subDispDefs, 0, sizeof(subDispDefs));
}

CondStructNew::CondStructNew(std::string Name, int NumArgs, bool preventDuplicate) : CondStructNew() {
	condName = strdup( Name.c_str());
	numArgs = NumArgs;
	if (preventDuplicate)
	{
		AddHook(dispTypeConditionAddPre, DK_NONE, ConditionPrevent, this, 0);
	}
	Register();
	
}

void CondStructNew::AddHook(enum_disp_type dispType, D20DispatcherKey dispKey, int(* callback)(DispatcherCallbackArgs)){
	Expects(numHooks < 99);
	subDispDefs[numHooks++] = { dispType, dispKey, callback, static_cast<uint32_t>(0), static_cast<uint32_t>(0) };
}

void CondStructNew::AddHook(enum_disp_type dispType, D20DispatcherKey dispKey, int(* callback)(DispatcherCallbackArgs), uint32_t data1, uint32_t data2){
	Expects(numHooks < 99);
	subDispDefs[numHooks++] = {dispType, dispKey, callback, data1, data2};
}

void CondStructNew::AddHook(enum_disp_type dispType, D20DispatcherKey dispKey, int(* callback)(DispatcherCallbackArgs), CondStructNew* data1, uint32_t data2)
{
	Expects(numHooks < 99);
	subDispDefs[numHooks++] = { dispType, dispKey, callback, data1, data2 };
}

void CondStructNew::Register(){
	conds.hashmethods.CondStructAddToHashtable(reinterpret_cast<CondStruct*>(this));
}

void CondStructNew::AddToFeatDictionary(feat_enums feat, feat_enums featEnumMax, uint32_t condArg2Offset){
	conds.AddToFeatDictionary(this, feat, featEnumMax, condArg2Offset);
}

uint32_t DispatcherCallbackArgs::GetCondArg(int argIdx)
{
	return conds.CondNodeGetArg(subDispNode->condNode, argIdx);
}

void* DispatcherCallbackArgs::GetCondArgPtr(int argIdx){
	return conds.CondNodeGetArgPtr(subDispNode->condNode, argIdx);
}

int DispatcherCallbackArgs::GetData1() const
{
	return subDispNode->subDispDef->data1;
}

int DispatcherCallbackArgs::GetData2() const
{
	return subDispNode->subDispDef->data2;
}

void DispatcherCallbackArgs::SetCondArg(int argIdx, int value)
{
	conds.CondNodeSetArg(subDispNode->condNode, argIdx, value);
}

DispIoAttackBonus::DispIoAttackBonus(){
	dispIOType = dispIOTypeAttackBonus;
	this->attackPacket.dispKey = 1;
}

int DispIoAttackBonus::Dispatch(objHndl obj, objHndl obj2, enum_disp_type dispType, D20DispatcherKey key){
	return dispatch.DispatchAttackBonus(obj, obj2, this, dispType, key);
}

void DispIoTooltip::Append(string& cs)
{
	if (numStrings < 10)
	{
		strncpy(strings[numStrings++], &cs[0], 0x100);
	}
}

DispIoBonusAndObj::DispIoBonusAndObj()
{
	dispIOType = dispIOTypeSkillLevel;
	obj = 0i64;
	pad = 0;
	returnVal = 0;
	bonOut = &bonlist;
}

DispIoD20ActionTurnBased::DispIoD20ActionTurnBased(){
	dispIOType = dispIOTypeD20ActionTurnBased;
	returnVal = 0;
	d20a = nullptr;
	tbStatus = nullptr;
	bonlist = nullptr;
}

DispIoD20ActionTurnBased::DispIoD20ActionTurnBased(D20Actn* D20a):DispIoD20ActionTurnBased(){
	d20a = D20a;
}

void DispIoD20ActionTurnBased::DispatchPerform(D20DispatcherKey key){
	if (!d20a || !d20a->d20APerformer){
		returnVal = AEC_INVALID_ACTION;
		return;
	}
		
	auto dispatcher = objects.GetDispatcher(d20a->d20APerformer);

	if (dispatcher->IsValid() ){
		dispatch.DispatcherProcessor(dispatcher, dispTypeD20ActionPerform, key, this);
	}
}
