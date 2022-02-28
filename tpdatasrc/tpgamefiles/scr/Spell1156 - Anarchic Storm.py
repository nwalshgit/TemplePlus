from toee import *

def OnBeginSpellCast(spell):
    print "Anarchic Storm OnBeginSpellCast"
    print "spell.target_list=", spell.target_list
    print "spell.caster=", spell.caster, " caster.level= ", spell.caster_level

def OnSpellEffect(spell):
    print "Anarchic Storm OnSpellEffect"

    targetsToRemove = []
    spell.duration = 1 * spell.caster_level # 1 round/caster_level

    cloudObject = game.obj_create(OBJECT_SPELL_GENERIC, spell.target_loc)

    casterInitiative = spell.caster.get_initiative()
    cloudObject.d20_status_init()
    cloudObject.set_initiative(casterInitiative)

    cloudRadius = 20.0
    cloudEventId = cloudObject.object_event_append(OLC_CRITTERS, cloudRadius)

    for spellTarget in spell.target_list:
         targetsToRemove.append(spellTarget.obj)
    spell.target_list.remove_list(targetsToRemove)

    cloudObject.condition_add_with_args("sp-Anarchic Storm", spell.id, spell.duration, 0, cloudEventId, spell.dc, 0)

    spell.target_list.remove_list(targetsToRemove)

def OnBeginRound(spell):
    print "Anarchic Storm OnBeginRound"

def OnEndSpellCast(spell):
    print "Anarchic Storm OnEndSpellCast"

