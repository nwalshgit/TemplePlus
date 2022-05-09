from toee import *

def OnBeginSpellCast(spell):
    print("Devour Magic OnBeginSpellCast")
    print("spell.target_list=", spell.target_list)
    print("spell.caster=", spell.caster, " caster.level= ", spell.caster_level)

def OnSpellEffect(spell):
    print("Devour Magic OnSpellEffect")

    spell.duration = 0
    storedTempHp = 0

    spellTarget = spell.target_list[0]
    if spellTarget.obj.type == obj_t_pc or spellTarget.obj.type == obj_t_npc:
        spellTarget.partsys_id = game.particles("sp-Dispel Magic - Targeted", spellTarget.obj)
        spellTarget.obj.condition_add_with_args("sp-Devour Magic", spell.id, spell.duration, storedTempHp, 0)
    elif spellTarget.obj.type == obj_t_portal or spellTarget.obj.type == obj_t_container:
        if spellTarget.obj.portal_flags_get() & OPF_MAGICALLY_HELD:
            spellTarget.partsys_id = game.particles("sp-Dispel Magic - Targeted", spellTarget.obj)
            spellTarget.obj.portal_flag_unset(OPF_MAGICALLY_HELD)
            spellTarget.target_list.remove_target(spellTarget.obj)
        else:
            spellTarget.obj.float_mesfile_line("mes\\spell.mes", 30000)
            game.particles("Fizzle", spellTarget.obj)
            spellTarget.target_list.remove_target(spellTarget.obj)
    else:
        spellTarget.target_list.remove_target(spellTarget.obj)

    spell.spell_end(spell.id)

def OnBeginRound(spell):
    print("Devour Magic OnBeginRound")

def OnEndSpellCast(spell):
    print("Devour Magic OnEndSpellCast")

