from toee import *

def OnBeginSpellCast(spell):
    print "Eldritch Cone OnBeginSpellCast"
    print "spell.target_list=", spell.target_list
    print "spell.caster=", spell.caster, " caster.level= ", spell.caster_level
    print "spell.spell_level=", spell.spell_level
    print "spell.dc=", spell.dc

def OnSpellEffect(spell):
    print "Eldritch Cone OnSpellEffect"

    targetsToRemove = []
    spell.duration = 0
    saveType = D20_Save_Reduction_Half
    spellDamageDice = dice_new("1d6")
    spellDamageDice.number = int((spell.caster_level + 1) / 2) #capped at cl 20 (9d6)
    damageType = spell.caster.d20_query("PQ_Eldritch_Blast_Damage_Type")
    if not damageType:
        damageType = D20DT_MAGIC
    saveDescriptor = D20STD_F_NONE

    #game.particles("sp-Ray of Enfeeblement", spell.target_loc)

    for spellTarget in spell.target_list:
        if spellTarget.obj.reflex_save_and_damage(spell.caster, spell.dc, saveType, saveDescriptor, spellDamageDice, damageType, D20DAP_UNSPECIFIED, D20A_CAST_SPELL, spell.id):
            spellTarget.obj.float_mesfile_line('mes\\spell.mes', 30001)
        else:
            spellTarget.obj.float_mesfile_line('mes\\spell.mes', 30002)
        if not spellTarget.obj.d20_query("PQ_Eldritch_Blast_Has_Secondary_Effect"):
            targetsToRemove.append(spellTarget.obj)
    if targetsToRemove:
        spell.target_list.remove_list(targetsToRemove)
    spell.spell_end(spell.id)

def OnBeginRound(spell):
    print "Eldritch Cone OnBeginRound"

def OnEndSpellCast(spell):
    print "Eldritch Cone OnEndSpellCast"

