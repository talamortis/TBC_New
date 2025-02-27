#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "zulaman.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"

enum Spells
{
    SPELL_BERSERK           = 45078,

    // Troll form
    SPELL_BRUTALSWIPE       = 42384,
    SPELL_MANGLE            = 42389,
    SPELL_MANGLEEFFECT      = 44955,
    SPELL_SURGE             = 42402,
    SPELL_BEARFORM          = 42377,

    // Bear form
    SPELL_LACERATINGSLASH   = 42395,
    SPELL_RENDFLESH         = 42397,
    SPELL_DEAFENINGROAR     = 42398
};

// Trash Waves
float NalorakkWay[8][3] =
{
    { 18.569f, 1414.512f, 11.42f}, // waypoint 1
    {-17.264f, 1419.551f, 12.62f},
    {-52.642f, 1419.357f, 27.31f}, // waypoint 2
    {-69.908f, 1419.721f, 27.31f},
    {-79.929f, 1395.958f, 27.31f},
    {-80.072f, 1374.555f, 40.87f}, // waypoint 3
    {-80.072f, 1314.398f, 40.87f},
    {-80.072f, 1295.775f, 48.60f} // waypoint 4
};

#define YELL_NALORAKK_WAVE1     "Get da move on, guards! It be killin' time!"
#define SOUND_NALORAKK_WAVE1    12066
#define YELL_NALORAKK_WAVE2     "Guards, go already! Who you more afraid of, dem... or me?"
#define SOUND_NALORAKK_WAVE2    12067
#define YELL_NALORAKK_WAVE3     "Ride now! Ride out dere and bring me back some heads!"
#define SOUND_NALORAKK_WAVE3    12068
#define YELL_NALORAKK_WAVE4     "I be losin' me patience! Go on: make dem wish dey was never born!"
#define SOUND_NALORAKK_WAVE4    12069

//Unimplemented SoundIDs
/*
#define SOUND_NALORAKK_EVENT1   12078
#define SOUND_NALORAKK_EVENT2   12079
*/

//General defines
#define YELL_AGGRO              "You be dead soon enough!"
#define SOUND_YELL_AGGRO        12070
#define YELL_KILL_ONE           "Mua-ha-ha! Now whatchoo got to say?"
#define SOUND_YELL_KILL_ONE     12075
#define YELL_KILL_TWO           "Da Amani gonna rule again!"
#define SOUND_YELL_KILL_TWO     12076
#define YELL_DEATH              "I... be waitin' on da udda side...."
#define SOUND_YELL_DEATH        12077
#define YELL_BERSERK            "You had your chance, now it be too late!" //Never seen this being used, so just guessing from what I hear.
#define SOUND_YELL_BERSERK      12074
#define YELL_SURGE              "I bring da pain!"
#define SOUND_YELL_SURGE        12071

#define YELL_SHIFTEDTOTROLL     "Make way for Nalorakk!"
#define SOUND_YELL_TOTROLL      12073


#define YELL_SHIFTEDTOBEAR      "You call on da beast, you gonna get more dan you bargain for!"
#define SOUND_YELL_TOBEAR       12072

class boss_nalorakk : public CreatureScript
{
    public:

        boss_nalorakk()
            : CreatureScript("boss_nalorakk")
        {
        }

        struct boss_nalorakkAI : public ScriptedAI
        {
            boss_nalorakkAI(Creature* creature) : ScriptedAI(creature)
            {
                MoveEvent = true;
                MovePhase = 0;
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;

            uint32 BrutalSwipe_Timer;
            uint32 Mangle_Timer;
            uint32 Surge_Timer;

            uint32 LaceratingSlash_Timer;
            uint32 RendFlesh_Timer;
            uint32 DeafeningRoar_Timer;

            uint32 ShapeShift_Timer;
            uint32 Berserk_Timer;

            bool inBearForm;
            bool MoveEvent;
            bool inMove;
            uint32 MovePhase;
            uint32 waitTimer;

            void Reset()
            {
                if (MoveEvent)
                {
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    inMove = false;
                    waitTimer = 0;
                    me->SetSpeedRate(MOVE_RUN, 2);
                    me->SetWalk(false);
                }else
                {
                    (*me).GetMotionMaster()->MovePoint(0, NalorakkWay[7][0], NalorakkWay[7][1], NalorakkWay[7][2]);
                }

                instance->SetData(DATA_NALORAKKEVENT, NOT_STARTED);

                Surge_Timer = urand(15000, 20000);
                BrutalSwipe_Timer = urand(7000, 12000);
                Mangle_Timer = urand(10000, 15000);
                ShapeShift_Timer = urand(45000, 50000);
                Berserk_Timer = 600000;

                inBearForm = false;
                // me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1, 5122);  /// @todo find the correct equipment id
            }

            void SendAttacker(Unit* target)
            {
                std::list<Creature*> templist;
                float x, y, z;
                me->GetPosition(x, y, z);

                {
                    CellCoord pair(Trinity::ComputeCellCoord(x, y));
                    Cell cell(pair);
                    cell.SetNoCreate();

                    Trinity::AllFriendlyCreaturesInGrid check(me);
                    Trinity::CreatureListSearcher<Trinity::AllFriendlyCreaturesInGrid> searcher(me, templist, check);

                    TypeContainerVisitor<Trinity::CreatureListSearcher<Trinity::AllFriendlyCreaturesInGrid>, GridTypeMapContainer> cSearcher(searcher);

                    cell.Visit(pair, cSearcher, *(me->GetMap()), *me, me->GetGridActivationRange());
                }

                if (templist.empty())
                    return;

                for (std::list<Creature*>::const_iterator i = templist.begin(); i != templist.end(); ++i)
                {
                    if ((*i) && me->IsWithinDistInMap((*i), 25))
                    {
                        (*i)->SetNoCallAssistance(true);
                        (*i)->AI()->AttackStart(target);
                    }
                }
            }

            void AttackStart(Unit* who)
            {
                if (!MoveEvent)
                    ScriptedAI::AttackStart(who);
            }

            void MoveInLineOfSight(Unit* who)

            {
                if (!MoveEvent)
                {
                    ScriptedAI::MoveInLineOfSight(who);
                }
                else
                {
                    if (me->IsHostileTo(who))
                    {
                        if (!inMove)
                        {
                            switch (MovePhase)
                            {
                                case 0:
                                    if (me->IsWithinDistInMap(who, 50))
                                    {
                                        // me->MonsterYell(YELL_NALORAKK_WAVE1, LANG_UNIVERSAL, NULL);
                                        DoPlaySoundToSet(me, SOUND_NALORAKK_WAVE1);

                                        (*me).GetMotionMaster()->MovePoint(1, NalorakkWay[1][0], NalorakkWay[1][1], NalorakkWay[1][2]);
                                        MovePhase ++;
                                        inMove = true;

                                        SendAttacker(who);
                                    }
                                    break;
                                case 2:
                                    if (me->IsWithinDistInMap(who, 40))
                                    {
                                        // me->MonsterYell(YELL_NALORAKK_WAVE2, LANG_UNIVERSAL, NULL);
                                        DoPlaySoundToSet(me, SOUND_NALORAKK_WAVE2);

                                        (*me).GetMotionMaster()->MovePoint(3, NalorakkWay[3][0], NalorakkWay[3][1], NalorakkWay[3][2]);
                                        MovePhase ++;
                                        inMove = true;

                                        SendAttacker(who);
                                    }
                                    break;
                                case 5:
                                    if (me->IsWithinDistInMap(who, 40))
                                    {
                                        // me->MonsterYell(YELL_NALORAKK_WAVE3, LANG_UNIVERSAL, NULL);
                                        DoPlaySoundToSet(me, SOUND_NALORAKK_WAVE3);

                                        (*me).GetMotionMaster()->MovePoint(6, NalorakkWay[6][0], NalorakkWay[6][1], NalorakkWay[6][2]);
                                        MovePhase ++;
                                        inMove = true;

                                        SendAttacker(who);
                                    }
                                    break;
                                case 7:
                                    if (me->IsWithinDistInMap(who, 50))
                                    {
                                        SendAttacker(who);

                                        // me->MonsterYell(YELL_NALORAKK_WAVE4, LANG_UNIVERSAL, NULL);
                                        DoPlaySoundToSet(me, SOUND_NALORAKK_WAVE4);

                                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

                                        MoveEvent = false;
                                    }
                                    break;
                            }
                        }
                    }
                }
            }

            void EnterCombat(Unit* /*who*/)
            {
                instance->SetData(DATA_NALORAKKEVENT, IN_PROGRESS);

                // me->MonsterYell(YELL_AGGRO, LANG_UNIVERSAL, NULL);
                DoPlaySoundToSet(me, SOUND_YELL_AGGRO);
                DoZoneInCombat();
            }

            void JustDied(Unit* /*killer*/)
            {
                instance->SetData(DATA_NALORAKKEVENT, DONE);

                // me->MonsterYell(YELL_DEATH, LANG_UNIVERSAL, NULL);
                DoPlaySoundToSet(me, SOUND_YELL_DEATH);
            }

            void KilledUnit(Unit* /*victim*/)
            {
                switch (urand(0, 1))
                {
                    case 0:
                        // me->MonsterYell(YELL_KILL_ONE, LANG_UNIVERSAL, NULL);
                        DoPlaySoundToSet(me, SOUND_YELL_KILL_ONE);
                        break;
                    case 1:
                        // me->MonsterYell(YELL_KILL_TWO, LANG_UNIVERSAL, NULL);
                        DoPlaySoundToSet(me, SOUND_YELL_KILL_TWO);
                        break;
                }
            }

            void MovementInform(uint32 type, uint32 id)
            {
                if (MoveEvent)
                {
                    if (type != POINT_MOTION_TYPE)
                        return;

                    if (!inMove)
                        return;

                    if (MovePhase != id)
                        return;

                    switch (MovePhase)
                    {
                        case 2:
                            me->SetOrientation(3.1415f*2);
                            inMove = false;
                            return;
                        case 1:
                        case 3:
                        case 4:
                        case 6:
                            MovePhase ++;
                            waitTimer = 1;
                            inMove = true;
                            return;
                        case 5:
                            me->SetOrientation(3.1415f*0.5f);
                            inMove = false;
                            return;
                        case 7:
                            me->SetOrientation(3.1415f*0.5f);
                            inMove = false;
                            return;
                    }

                }
            }

            void UpdateAI(uint32 diff)
            {
                if (waitTimer && inMove)
                {
                    if (waitTimer <= diff)
                    {
                        // @todo
                        //(*me).GetMotionMaster()->MovementExpired();
                        (*me).GetMotionMaster()->MovePoint(MovePhase, NalorakkWay[MovePhase][0], NalorakkWay[MovePhase][1], NalorakkWay[MovePhase][2]);
                        waitTimer = 0;
                    } else waitTimer -= diff;
                }

                if (!UpdateVictim())
                    return;

                if (Berserk_Timer <= diff)
                {
                    DoCast(me, SPELL_BERSERK, true);
                    // me->MonsterYell(YELL_BERSERK, LANG_UNIVERSAL, NULL);
                    DoPlaySoundToSet(me, SOUND_YELL_BERSERK);
                    Berserk_Timer = 600000;
                } else Berserk_Timer -= diff;

                if (ShapeShift_Timer <= diff)
                {
                    if (inBearForm)
                    {
                        // me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1, 5122);
                        // me->MonsterYell(YELL_SHIFTEDTOTROLL, LANG_UNIVERSAL, NULL);
                        DoPlaySoundToSet(me, SOUND_YELL_TOTROLL);
                        me->RemoveAurasDueToSpell(SPELL_BEARFORM);
                        Surge_Timer = urand(15000, 20000);
                        BrutalSwipe_Timer = urand(7000, 12000);
                        Mangle_Timer = urand(10000, 15000);
                        ShapeShift_Timer = urand(45000, 50000);
                        inBearForm = false;
                    }
                    else
                    {
                        // me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1, 0);
                        // me->MonsterYell(YELL_SHIFTEDTOBEAR, LANG_UNIVERSAL, NULL);
                        DoPlaySoundToSet(me, SOUND_YELL_TOBEAR);
                        DoCast(me, SPELL_BEARFORM, true);
                        LaceratingSlash_Timer = 2000; // dur 18s
                        RendFlesh_Timer = 3000;  // dur 5s
                        DeafeningRoar_Timer = urand(5000, 10000);  // dur 2s
                        ShapeShift_Timer = urand(20000, 25000); // dur 30s
                        inBearForm = true;
                    }
                } else ShapeShift_Timer -= diff;

                if (!inBearForm)
                {
                    if (BrutalSwipe_Timer <= diff)
                    {
                        DoCastVictim(SPELL_BRUTALSWIPE);
                        BrutalSwipe_Timer = urand(7000, 12000);
                    } else BrutalSwipe_Timer -= diff;

                    if (Mangle_Timer <= diff)
                    {
                        if (me->GetVictim() && !me->GetVictim()->HasAura(SPELL_MANGLEEFFECT))
                        {
                            DoCastVictim(SPELL_MANGLE);
                            Mangle_Timer = 1000;
                        }
                        else Mangle_Timer = urand(10000, 15000);
                    } else Mangle_Timer -= diff;

                    if (Surge_Timer <= diff)
                    {
                        // me->MonsterYell(YELL_SURGE, LANG_UNIVERSAL, NULL);
                        DoPlaySoundToSet(me, SOUND_YELL_SURGE);
                        Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 45, true);
                        if (target)
                            DoCast(target, SPELL_SURGE);
                        Surge_Timer = urand(15000, 20000);
                    } else Surge_Timer -= diff;
                }
                else
                {
                    if (LaceratingSlash_Timer <= diff)
                    {
                        DoCastVictim(SPELL_LACERATINGSLASH);
                        LaceratingSlash_Timer = urand(18000, 23000);
                    } else LaceratingSlash_Timer -= diff;

                    if (RendFlesh_Timer <= diff)
                    {
                        DoCastVictim(SPELL_RENDFLESH);
                        RendFlesh_Timer = urand(5000, 10000);
                    } else RendFlesh_Timer -= diff;

                    if (DeafeningRoar_Timer <= diff)
                    {
                        DoCastVictim(SPELL_DEAFENINGROAR);
                        DeafeningRoar_Timer = urand(15000, 20000);
                    } else DeafeningRoar_Timer -= diff;
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetZulAmanAI<boss_nalorakkAI>(creature);
        }
};

void AddSC_boss_nalorakk()
{
    new boss_nalorakk();
}

