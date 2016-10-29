#pragma once

#include "common.h"
#include "idxtables.h"
#include "tig\tig_mes.h"
#include "spell_structs.h"
#include "tio/tio.h"
#include <map>

//#include "ui/ui_picker.h"

struct GameSystemConf;
class TioOutputStream;
struct PickerArgs;

#define MAX_SPELLS_KNOWN 384

#pragma region Spell Structs

struct SpellEntryLevelSpec
{
	uint32_t spellClass;
	uint32_t slotLevel;
	SpellEntryLevelSpec() { spellClass = 0; slotLevel = 0; }
};

struct SpellMultiOption
{
	int value;
	bool isProto = false;
};

enum SpellRangeType : uint32_t;
enum Domain;

struct SpellEntry{
	uint32_t spellEnum;
	uint32_t spellSchoolEnum;
	uint32_t spellSubSchoolEnum;
	uint32_t spellDescriptorBitmask;
	uint32_t spellComponentBitmask;
	uint32_t costXP;
	uint32_t costGP;
	uint32_t castingTimeType;
	SpellRangeType spellRangeType;
	uint32_t spellRange;
	uint32_t savingThrowType;
	uint32_t spellResistanceCode;
	SpellEntryLevelSpec spellLvls[10];
	uint32_t spellLvlsNum;
	uint32_t projectileFlag;
	uint64_t flagsTargetBitmask;
	uint64_t incFlagsTargetBitmask;
	uint64_t excFlagsTargetBitmask;
	uint64_t modeTargetSemiBitmask; // UiPickerType
	uint32_t minTarget;
	uint32_t maxTarget;
	int radiusTarget; //note:	if it's negative, then its absolute value is used as SpellRangeType for mode_target personal; if it's positive, it's a specified number(in feet ? )
	int degreesTarget;
	uint32_t aiTypeBitmask;
	uint32_t pad;

	//UiPickerType GetModeTarget() const;
	SpellEntry();
	explicit SpellEntry(uint32_t spellEnum);
	bool IsBaseModeTarget(UiPickerType type);
	int SpellLevelForSpellClass(int spellClass); // returns -1 if none
};

struct SpellEntryExt {
	std::vector<SpellEntryLevelSpec> levelSpecs;
};

const uint32_t TestSizeOfSpellEntry = sizeof(SpellEntry); // should be 0xC0  ( 192 )

struct SpellPacketBody{
	uint32_t spellEnum;
	uint32_t spellEnumOriginal; // used for spontaneous casting in order to debit the "original" spell
	uint32_t flagSthg; // 8 used in GoalStateFunc70
	void * pSthg;
	objHndl caster;
	uint32_t casterPartsysId;
	uint32_t spellClass; // aka spellClass
	uint32_t spellKnownSlotLevel; // aka spellLevel
	uint32_t casterLevel;
	uint32_t dc;
	int numSpellObjs;
	objHndl aoeObj;
	struct SpellObj
	{
		objHndl obj;
		int partySysId;
		int field_C;
	}	spellObjs[128];
	uint32_t orgTargetCount;
	uint32_t targetCount;
	objHndl targetListHandles[32];
	uint32_t targetListPartsysIds[32];
	uint32_t projectileCount;
	uint32_t field_9C4;
	objHndl projectiles[5];
	LocFull aoeCenter;
	uint32_t field_A04;
	SpellEntry spellEntry;
	int duration;
	int durationRemaining;
	uint32_t spellRange;
	uint32_t savingThrowResult;
	uint32_t invIdx; // inventory index, used for casting spells from items e.g. scrolls
	uint32_t metaMagicData;
	uint32_t spellId;
	uint32_t field_AE4;
	SpellPacketBody();
	SpellPacketBody(uint32_t spellId);
	/*
	// updates the spell in the SpellsCast registry *if it is still active*
	*/
	bool UpdateSpellsCastRegistry() const;
	bool FindObj(objHndl obj, int* idx) const;
	bool InsertToPartsysList(uint32_t idx, int partsysId);
	bool InsertToTargetList(uint32_t idx, objHndl tgt);
	// fetches from the SpellsCastRegistry. If it fails, the spellId will be 0 (as in the Reset function)
	bool AddTarget(objHndl tgt, int partsysId, int replaceExisting); // will add target (or replace its partsys if it already exists)
	bool SavingThrow(objHndl target, D20SavingThrowFlag flags);
	const char* GetName(); // get the spell name
	bool IsVancian();
	bool IsDivine();
	void Debit(); // debit from the caster's memorized / daily casted spells
	void MemorizedUseUp(SpellStoreData &spellData); // mark memorized spell as used up
	void Reset();
};

const uint32_t TestSizeOfSpellPacketBody = sizeof(SpellPacketBody); // should be 0xAE8  (2792)

struct SpellPacket
{
	uint32_t key;
	uint32_t isActive;
	SpellPacketBody spellPktBody;
};

const uint32_t TestSizeOfSpellPacket = sizeof(SpellPacket); // should be 0xAF0  (2800)

struct SpellMapTransferInfo
{
	int spellId;
	int field4;
	ObjectId casterObjId;
	ObjectId aoeObjId;
	ObjectId spellObjs[128];
	ObjectId targets[32];
	ObjectId projectiles[5];
	std::string targetlistPartsys[32];
	std::string spellObjPartsys[128];
	std::string casterPartsys;
	int field1234;
	SpellMapTransferInfo();
};

#pragma endregion

struct CondStruct;

struct LegacySpellSystem : temple::AddressTable
{
	friend class D20ClassSystem;
	friend class SpellFuncReplacements;

	IdxTable<SpellPacket> * spellCastIdxTable;
	std::map<int, SpellEntryExt> mSpellEntryExt;
	
	MesHandle * spellEnumMesHandle;
	MesHandle spellEnumsExt;
	MesHandle * spellMes;
	MesHandle spellMesExt;
	MesHandle * spellsRadialMenuOptionsMes;
	std::vector<SpellMapTransferInfo> spellMapTransInfo;
	void Init(const GameSystemConf& conf);
	

	int GetNewSpellId();
	BOOL RegisterSpell(SpellPacketBody& spellPkt, int spellId);

	uint32_t spellRegistryCopy(uint32_t spellEnum, SpellEntry* spellEntry);
	int CopyLearnableSpells(objHndl & handle, int spellClass, std::vector<SpellEntry> & entries);
	uint32_t ConfigSpellTargetting(PickerArgs* pickerArgs, SpellPacketBody* spellPacketBody);
	int GetMaxSpellLevel(objHndl objHnd, Stat classCode, int characterLvl = 0); // if characterLvl is 0 it will fetch the actual level; it also takes into account spell list extension by PrC's and such
	int GetNumSpellsPerDay(objHndl handle, Stat classCode, int spellLvl); // including from spell list extension
	int ParseSpellSpecString(SpellStoreData* spell, char* spellString);

	const char* GetSpellMesline(uint32_t line) const;
	bool CheckAbilityScoreReqForSpell(objHndl handle, uint32_t spellEnum, int statBeingRaised) const;
	int GetSpellClass(int classEnum, bool isDomain = false);
	static const char* GetSpellEnumTAG(uint32_t spellEnum);
	const char* GetSpellName(uint32_t spellEnum) const;
	
	void SetSpontaneousCastingAltNode(objHndl obj, int nodeIdx, SpellStoreData* spellData);
	

	uint32_t getBaseSpellCountByClassLvl(uint32_t classCode, uint32_t classLvl, uint32_t slotLvl, uint32_t unknown1);
	uint32_t getWizSchool(objHndl objHnd);
	bool IsForbiddenSchool(objHndl handle, int spSchool);
	uint32_t getStatModBonusSpellCount(objHndl objHnd, uint32_t classCode, uint32_t slotLvl);
	void spellPacketBodyReset(SpellPacketBody * spellPktBody);
	void SpellPacketSetCasterLevel(SpellPacketBody * spellPktBody) const;
	uint32_t getSpellEnum(const char* spellName);
	uint32_t GetSpellEnumFromSpellId(uint32_t spellId);
	uint32_t GetSpellPacketBody(uint32_t spellId, SpellPacketBody* spellPktBodyOut);
	void UpdateSpellPacket(const SpellPacketBody &spellPktBody);
	uint32_t spellKnownQueryGetData(objHndl objHnd, uint32_t spellEnum, uint32_t* classCodesOut, uint32_t* slotLevelsOut, uint32_t* count);
	bool SpellKnownQueryGetData(objHndl objHnd, uint32_t spellEnum, std::vector<int> & classCodesOut, std::vector<int> & spellLevels); // returns the information of spell known by the caster; retruns false if unknown
	bool IsSpellKnown(objHndl handle, int spEnum, int spClass);
	uint32_t spellCanCast(objHndl objHnd, uint32_t spellEnum, uint32_t spellClassCode, uint32_t spellLevel);
	int NumSpellsInLevel(objHndl handle, obj_f spellField, int spellClass, int spellLvl);
	uint32_t spellMemorizedQueryGetData(objHndl objHnd, uint32_t spellEnum, uint32_t* classCodesOut, uint32_t* slotLevelsOut, uint32_t* count);
	bool numSpellsKnownTooHigh(objHndl objHnd);
	bool numSpellsMemorizedTooHigh(objHndl objHnd);

	// SpellClass 
	bool isDomainSpell(uint32_t spellClassCode);
	Stat GetCastingClass(uint32_t spellClassCode);
	bool IsArcaneSpellClass(uint32_t spellClass);

	static bool IsSpellLike(int spellEnum); // checks if the spell is in the Spell Like Ability range
	static bool IsLabel(int spellEnum); // check if it is a hardcoded "label" enum (used in the GUI etc)
	static bool IsNewSlotDesignator(int spellEnum); // check if it is a  hardcoded "new slot" designator (used for sorting)  enums 1605-1614
	int GetSpellLevelBySpellClass(int spellEnum, int spellClass, objHndl handle = objHndl::null); // returns -1 if not available for spell class
	bool SpellHasMultiSelection(int spellEnum);
	bool GetMultiSelectOptions(int spellEnum, std::vector<SpellMultiOption>& multiOptions);

	uint32_t pickerArgsFromSpellEntry(SpellEntry * spellEntry, PickerArgs * pickArgs, objHndl objHnd, uint32_t casterLevel);
	uint32_t GetSpellRangeExact(SpellRangeType spellRangeType, uint32_t casterLevel, objHndl caster);
	uint32_t GetSpellRange(SpellEntry* spellEntry, uint32_t baseCasterLevel, objHndl caster);
	const char* GetSpellEnumNameFromEnum(int spellEnum);
	bool GetSpellTargets(objHndl obj, objHndl tgt, SpellPacketBody* spellPkt, unsigned spellEnum);
	BOOL SpellHasAiType(unsigned spellEnum, AiSpellType aiSpellType);
	BOOL IsSpellHarmful(int spellEnum, const objHndl& caster, const objHndl& tgt);

	/*
		does a d20 roll for dispelling, and logs to history (outputting a history ID)
	*/
	int DispelRoll(objHndl obj,	BonusList* bonlist, int rollMod, int dispelDC, char* historyText, int* rollHistId);
	/*
		Plays the Fizzle particles and does a sound
	*/
	BOOL PlayFizzle(objHndl handle);
	int CheckSpellResistance(SpellPacketBody* spellPkt, objHndl obj);

	void SpellBeginRound(objHndl); // plays the OnBeginRound python script
	objHndl mSpellBeginRoundObj = objHndl::null; // supplemental info for the OnBeginRound invocation to identify whose round is beginning...
	int SpellEnd(int spellId, int endDespiteTargetList) const; // endDespiteTargetList will end the spell even if the target list isn't empty
	
	void (__cdecl *SpellRemove)(int);
	
	void SpellSave(); // packs the spells cast registry to SpellMapTransferInfo data structs
	int SpellSave(TioOutputStream&); // saves the spells cast registry to file
		void SpellSavePruneInactive() const;
		
	SpellMapTransferInfo SaveSpellForTeleport(const SpellPacket& data);
		
	void GetSpellsFromTransferInfo();
		bool GetSpellPacketFromTransferInfo(unsigned& spellId, SpellPacket& spellPkt, SpellMapTransferInfo& it);

	bool LoadActiveSpellElement(TioFile* file, uint32_t& spellId, SpellPacket& pkt);
	static void SpellsCastRegistryPut(int spellId, SpellPacket&);
	bool IsSpellActive(int spellid);

	CondStruct *GetCondFromSpellIdx(int id);
	uint32_t(__cdecl * spellRemoveFromStorage)(objHndl objHnd, obj_f fieldIdx, SpellStoreData * spellData, int unknown);
	uint32_t (__cdecl * spellsPendingToMemorized)(objHndl objHnd);
	void SpellsPendingToMemorizedByClass(objHndl handle, Stat classEnum);
	void SpellsCastReset(objHndl handle, Stat classEnum = (Stat)-1);
	int (__cdecl *SpellKnownAdd)(objHndl ObjHnd, int nSpellIdx, int nSpellClassCode, int nSpellCasterLevel, int nSpellStoreData, int nMetamagicData);
	int (__cdecl *SpellMemorizedAdd)(objHndl ObjHnd, int nSpellIdx, int nSpellClassCode, int nSpellCasterLevel, int nSpellStoreData, int nMetamagicData);
	void ForgetMemorized(objHndl handle);
	LegacySpellSystem()
	{
		rebase(spellCastIdxTable, 0x10AAF218);
		rebase(spellEnumMesHandle,0x10AAF210); 
		rebase(spellMes, 0x10AAF438);
		rebase(spellsRadialMenuOptionsMes, 0x10BD0238);

		//rebase(SpellEnd, 0x10079980);
		rebase(SpellRemove, 0x10079A20);
		rebase(_getSpellCountByClassLvl, 0x100F4D10);
		rebase(_getStatModBonusSpellCount, 0x100F4C30);
		rebase(spellRemoveFromStorage, 0x100758A0);
		rebase(spellsPendingToMemorized, 0x100757D0);
		rebase(_spellPacketBodyReset,0x1008A350); 
		rebase(_spellPacketSetCasterLevel,0x10079B70); 
		rebase(SpellKnownAdd, 0x10079EE0);
		rebase(SpellMemorizedAdd, 0x10075A10);
		rebase(_pickerArgsFromSpellEntry,0x100772A0); 
		//rebase(spellMTI, 0x10AB59E8);
	}
private:

	BOOL SpellEntriesInit(const char* spellRulesFodler);
		bool SpellEntryFileParse(SpellEntry & spEntry, TioFile *tf);

	std::map<int, std::vector<SpellMultiOption>> mMultiOptions;
	void GetSpellMultiOptionsFromFile(int spellEnum);
	void GetSpellEntryExtFromClassSpec(std::map<int, int>& mapping, int classEnum);

	uint32_t(__cdecl * _getSpellCountByClassLvl)();
	uint32_t(__cdecl* _getStatModBonusSpellCount)();
	void(__cdecl * _spellPacketBodyReset)(SpellPacketBody * spellPktBody);
	void(__cdecl * _spellPacketSetCasterLevel)(SpellPacketBody * spellPktBody);
	uint32_t(__cdecl * _pickerArgsFromSpellEntry)(SpellEntry * spellEntry, PickerArgs * pickArgs, objHndl objHnd, uint32_t casterLevel);
	//uint32_t(__cdecl * Spell_Range_Sthg_From_SpellEntry_sub_100772A0)(SpellEntry *spellEntry, );
};

extern LegacySpellSystem spellSys;

extern IdxTableWrapper<SpellEntry> spellEntryRegistry;
extern IdxTableWrapper<SpellPacket> spellsCastRegistry;


struct SpontCastSpellLists : temple::AddressTable
{
public:
	uint32_t spontCastSpellsDruid[11];
	uint32_t spontCastSpellsEvilCleric[11];
	uint32_t spontCastSpellsGoodCleric[11];
	uint32_t spontCastSpellsDruidSummons[11];
	SpontCastSpellLists()
	{
		int _spontCastSpellsDruid[] = { -1, 476, 477, 478, 479, 480, 481, 482, 483, 484, 4000 };
		int _spontCastSpellsEvilCleric[] = { 248, 247, 249, 250, 246, 61, 581, 582, 583, 583, 0 };
		int _spontCastSpellsGoodCleric[] = { 91, 90, 92, 93, 89, 221, 577, 578, 579, 579, 0 };
		int _spontCastSpellsDruidSummons[] = { -1, 2000, 2100, 2200, 2300, 2400, 2500, 2600, 2700, 2800, 0 };
		memcpy(spontCastSpellsDruid, _spontCastSpellsDruid, 11 * sizeof(int));
		memcpy(spontCastSpellsEvilCleric, _spontCastSpellsEvilCleric, 11 * sizeof(int));
		memcpy(spontCastSpellsGoodCleric, _spontCastSpellsGoodCleric, 11 * sizeof(int));
		memcpy(spontCastSpellsDruidSummons, _spontCastSpellsDruidSummons, 11 * sizeof(int));
	}
};

extern SpontCastSpellLists spontCastSpellLists;



const uint32_t TestSizeOfSpellStoreType = sizeof(SpellStoreType);
const uint32_t TestSizeOfSpellStoreState = sizeof(SpellStoreState);

//const uint32_t bbb = sizeof(int32_t);


uint32_t _getWizSchool(objHndl objHnd);
uint32_t _getSpellEnum(const char * spellName);
uint32_t _spellKnownQueryGetData(objHndl objHnd, uint32_t spellEnum, uint32_t * classCodesOut, uint32_t *slotLevelsOut, uint32_t * count);
uint32_t _spellMemorizedQueryGetData(objHndl objHnd, uint32_t spellEnum, uint32_t * classCodesOut, uint32_t *slotLevelsOut, uint32_t * count);
uint32_t _spellCanCast(objHndl objHnd, uint32_t spellEnum, uint32_t spellClassCode, uint32_t spellLevel);
uint32_t __cdecl _spellRegistryCopy(uint32_t spellEnum, SpellEntry * spellEntry);
uint32_t _GetSpellEnumFromSpellId(uint32_t spellId);
uint32_t _GetSpellPacketBody(uint32_t spellId, SpellPacketBody * spellPktBodyOut);
void _SetSpontaneousCastingAltNode(objHndl obj, int nodeIdx, SpellStoreData* spellData);





enum SpellRangeType : uint32_t
{
	SRT_Specified = 0,
		SRT_Personal = 1,
		SRT_Touch,
		SRT_Close,
		SRT_Medium,
		SRT_Long,
		SRT_Unlimited,
		SRT_Special_Inivibility_Purge
};




