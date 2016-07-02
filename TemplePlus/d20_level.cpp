#include "stdafx.h"
#include "d20_level.h"
#include "common.h"
#include "obj.h"
#include "util/fixes.h"
#include "gamesystems/gamesystems.h"
#include "gamesystems/objects/objsystem.h"


D20LevelSystem d20LevelSys;

struct D20LevelSystemAddresses : temple::AddressTable
{
	uint32_t * xpReqTable;
	uint32_t (__cdecl *LevelPacketInit)(LevelPacket *lvlPkt);
	uint32_t(__cdecl *LevelPacketDealloc)(LevelPacket* lvlPkt);
	uint32_t(__cdecl *GetLevelPacket)(Stat classEnum, objHndl objHnd, uint32_t levelAdjustSthg, uint32_t classLevel, LevelPacket* lvlPkt);

	D20LevelSystemAddresses()
	{
		rebase(xpReqTable, 0x102AAF00); // xp needed to reach level N, starting from N=0 (0 , 0 , 1000, 3000, ...)

		rebase(LevelPacketDealloc, 0x100F4780);
		rebase(LevelPacketInit, 0x100F5520);
		rebase(GetLevelPacket, 0x100F5140);
	};
} addresses;



class D20LevelHooks : public TempleFix
{
public: 
	
	void apply() override 
	{
		// hooked the address of the wizardSpellsPerLvl
		auto writeVal = reinterpret_cast<int>(d20LevelSys.mWizardSpellsPerLevel);

		write(0x100F4E4C + 3, &writeVal, sizeof(int));

		// AddSkillPoints
		replaceFunction<int(objHndl, int, int)>(0x1007DAA0, [](objHndl obj, int skillEnum, int numAdd)->int{
			return d20LevelSys.AddSkillPoints(obj, skillEnum, numAdd);
		});
	}
} d20LevelHooks;

uint32_t D20LevelSystem::LevelPacketInit(LevelPacket* lvlPkt)
{
	return addresses.LevelPacketInit(lvlPkt);
}

uint32_t D20LevelSystem::LevelPacketDealloc(LevelPacket* lvlPkt)
{
	return addresses.LevelPacketDealloc(lvlPkt);
}

uint32_t D20LevelSystem::GetLevelPacket(Stat classEnum, objHndl objHnd, uint32_t levelAdjustSthg, uint32_t classLevel, LevelPacket* lvlPkt)
{
	return addresses.GetLevelPacket(classEnum, objHnd, levelAdjustSthg, classLevel, lvlPkt);
}

bool D20LevelSystem::CanLevelup(objHndl objHnd)
{
	auto lvl = objects.StatLevelGet(objHnd, stat_level);
	if (d20Sys.d20Query(objHnd, DK_QUE_ExperienceExempt) || lvl >= (int)config.maxLevel)
	{
		return 0;
	}
	return objects.getInt32(objHnd, obj_f_critter_experience) >= xpReqTable[lvl + 1];
		//addresses.xpReqTable[lvl + 1];

}

int D20LevelSystem::AddSkillPoints(objHndl handle, int skillEnum, int numAdd){

	auto obj = gameSystems->GetObj().GetObject(handle);
	auto crLvls = obj->GetInt32Array(obj_f_critter_level_idx);
	
	if (!crLvls.GetSize() ){ // by default use fighter specs
		if (d20ClassSys.IsClassSkill((SkillEnum)skillEnum, stat_level_fighter))
			numAdd *= 2;
	}
	else{
		auto lastClass = (Stat)crLvls[crLvls.GetSize() - 1];
		if (lastClass == stat_level_cleric && temple::GetRef<BOOL(__cdecl)(objHndl, int)>(0x1004C0D0)(handle, skillEnum)
			|| d20ClassSys.IsClassSkill((SkillEnum)skillEnum, lastClass)){
			numAdd *= 2;
		} 
	}

	auto result = numAdd + obj->GetInt32(obj_f_critter_skill_idx, skillEnum);
	if (obj->IsPC()){
		auto expectedSkillpts = 2 * objects.StatLevelGet(handle, stat_level) + 6;
		if (result > expectedSkillpts)
		{
			logger->debug("Warning: the PC {} has more skill points ({}) than they should ({})", handle, result, expectedSkillpts);
		}	
	}
	obj->SetInt32(obj_f_critter_skill_idx, skillEnum, result);

	return result;
}

uint32_t D20LevelSystem::GetXpRequireForLevel(uint32_t level)
{
	if (level < 0 || level >= XP_REQ_TABLE_MAX_LEVEL)
		return 0;
	return xpReqTable[level];

}

int D20LevelSystem::GetSurplusXp(objHndl handle){
	int xpReq = (int)GetXpRequireForLevel(objects.StatLevelGet(handle, stat_level));
	return gameSystems->GetObj().GetObject(handle)->GetInt32(obj_f_critter_experience) - xpReq;
}

int D20LevelSystem::GetSpellsPerLevel(const objHndl handle, Stat classCode, int spellLvl, int casterLvl){
	// todo: extend to non-core
	LevelPacket lvlPkt;
	auto spellLvlCapped = spellLvl;
	if (classCode == stat_level_bard){
		if (spellLvlCapped > 7)
			spellLvlCapped = 7;
	}
	else if (classCode == stat_level_paladin){
		if (spellLvlCapped > 5)
			spellLvlCapped = 5;
	}
	else if (spellLvlCapped > 10){
		spellLvlCapped = 10;
	}

	lvlPkt.GetLevelPacket(classCode, handle, 0, casterLvl);
	auto spPerLvl = lvlPkt.sorcBardSpellCount[spellLvlCapped];

	switch (spPerLvl + 40){
	case 0:
	case 10: 
	case 20:
	case 30:
		spPerLvl = 0;
		break;
	default:
		break;
	}
	return spPerLvl;
}

void D20LevelSystem::GenerateSpellsPerLevelTables()
{

	memset(mWizardSpellsPerLevel, 0,  sizeof(mWizardSpellsPerLevel));

	memcpy(mWizardSpellsPerLevel, temple::GetPointer<int>(0x102EA460), 20 * NUM_SPELL_LEVELS*sizeof(int));

}

LevelPacket::LevelPacket(){
	d20LevelSys.LevelPacketInit(this);
}

LevelPacket::~LevelPacket()
{
	d20LevelSys.LevelPacketDealloc(this);
}

int LevelPacket::GetLevelPacket(Stat classCode, objHndl obj, int lvlAdj, int classLvl)
{
	return d20LevelSys.GetLevelPacket(classCode, obj, lvlAdj, classLvl, this);
}

uint32_t _CanLevelup(objHndl objHnd)
{
	return d20LevelSys.CanLevelup(objHnd);
}
