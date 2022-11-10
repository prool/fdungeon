// $Id: interp.c,v 1.132 2004/03/25 14:46:39 wagner Exp $
// Copyrights (C) 1998-2001, Forgotten Dungeon team.
// Read ours copyrights and license terms in 'license.fd'

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "interp.h"
#include "tables.h"
#include "magic.h"

#ifdef WITH_UNICORN
#include "unicorn.h"
#endif

DECLARE_SPELL_FUN( spell_null              );
CHAR_DATA *find_questmob(int level);
void    smash_tilde     args( ( char *str ) );

char *flag_string args((const struct flag_type *flag_table,int64 bits));
void complete_quest(CHAR_DATA *ch, CHAR_DATA *questman, char *buf);
int group_cost(CHAR_DATA *ch,int gn);
int64 flag_convert64(char letter );
char        *pflag64(int64 flag);
int64 fread_flag64( FILE *fp);
bool        check_social    args( ( CHAR_DATA *ch, char *command,
                            const char *argument ) );

bool fLogAll = FALSE;

struct  cmd_type        cmd_table       [] =
{
 // Common movement command
 { "north",  "�����",  do_north, POS_STANDING,  0,  LOG_NEVER,  NOLOG|FREEZE },
 { "east",   "������", do_east,  POS_STANDING,  0,  LOG_NEVER,  NOLOG|FREEZE },
 { "south",  "��",     do_south, POS_STANDING,  0,  LOG_NEVER,  NOLOG|FREEZE },
 { "west",   "�����",  do_west,  POS_STANDING,  0,  LOG_NEVER,  NOLOG|FREEZE },
 { "up",     "�����",  do_up,    POS_STANDING,  0,  LOG_NEVER,  NOLOG|FREEZE },
 { "down",   "����",   do_down,  POS_STANDING,  0,  LOG_NEVER,  NOLOG|FREEZE },

 // Common other commands
 { "at",          "��",          do_at,          POS_DEAD,    102,  WIZ_SECURE, NOMOB|HIDE },
 { "auction",     "�������",     do_auction,     POS_SLEEPING,  0,  WIZ_GSPEAKS,SHOW|NOMOB|SPAM|NOARMY|FREEZE|NOLOG },
 { "account",     "����",        do_account,     POS_SLEEPING,  0,  WIZ_SECURE, SHOW|NOMOB|NOORDER|FREEZE|SAVE },
 { "cast",        "���������",   do_cast,        POS_FIGHTING,  0,  WIZ_SKILLS, SHOW|NOZRIT|NOALL|FREEZE|NOLOG },
 { "chat",        "���",         do_chat,        POS_DEAD,      0,  WIZ_GSPEAKS,SHOW|MORPH|HIDE|SPAM|NOMOB },
 { "censored",    "�������",     do_censored,    POS_DEAD,      1,  WIZ_GSPEAKS,SHOW|MORPH|HIDE|SPAM|NOMOB|FREEZE },
 { "buy",         "������",      do_buy,         POS_RESTING,   0,  WIZ_SECURE, SHOW|NOMOB|FREEZE|NOLOG },
 { "bounty",      "�������",     do_bounty,      POS_DEAD,      0,  WIZ_SKILLS, SHOW|MORPH|NOMOB|SPAM|FREEZE|SAVE },
 { "channels",    "������",      do_channels,    POS_DEAD,      0,  WIZ_CONFIG, SHOW|MORPH|HIDE|NOMOB|NOLOG },
 { "deaf",        "������",      do_deaf,        POS_DEAD,      0,  WIZ_CONFIG, SHOW|MORPH|HIDE|NOMOB|NOLOG },
 { "exits",       "������",      do_exits,       POS_RESTING,   0,  LOG_NEVER,  SHOW|MORPH|HIDE|NOMOB|NOLOG },
 { "get",         "�����",       do_get,         POS_RESTING,   0,  WIZ_SECURE, SHOW|NOZRIT|FREEZE|NOLOG },
 { "goto",        "����",        do_goto,        POS_DEAD,    102,  WIZ_SECURE, SHOW|FREEZE },
 { "group",       "������",      do_group,       POS_SLEEPING,  0,  LOG_NEVER,  SHOW|HIDE|NOLOG },
 { "guild",       "�������",     do_guild,       POS_DEAD,      1,  WIZ_SECURE, SHOW|MORPH|NOMOB|NOORDER|FREEZE },
 { "hit",         "�������",     do_kill,        POS_FIGHTING,  0,  WIZ_SKILLS, NOZRIT|FREEZE|NOLOG },
 { "inventory",   "������",      do_inventory,   POS_DEAD,      0,  WIZ_SECURE, SHOW|HIDE|NOLOG|NOMOB },
 { "kill",        "�����",       do_kill,        POS_FIGHTING,  0,  WIZ_SKILLS, SHOW|NOZRIT|FREEZE|NOLOG },
 { "look",        "��������",    do_look,        POS_RESTING,   0,  WIZ_SECURE, SHOW|MORPH|HIDE|NOLOG },
 { "leader",      "�����",       do_leader,      POS_DEAD,      0,  WIZ_CONFIG, SHOW|MORPH|NOMOB|HIDE|NOORDER|FREEZE },
 { "clan",        "�����",       do_clantalk,    POS_SLEEPING,  0,  WIZ_SPEAKS, SHOW|MORPH|HIDE|SPAM },
 { "clanwork",    "��������",    do_clanwork,    POS_DEAD,    107,  WIZ_SECURE, SHOW|MORPH|HIDE|NOMOB|FREEZE|NOPUB },
 { "clanrecall",  "���������",   do_clan_recall, POS_FIGHTING,  0,  WIZ_SECURE, SHOW|NOADR|FREEZE|SAVE|NOLOG },
 { "sprecal",     "�������",     do_sprecall,    POS_FIGHTING,  0,  WIZ_SECURE, SHOW|NOADR|FREEZE|SAVE|NOLOG|NOMOB },
 { "petition",    "���������",   do_petition,    POS_SLEEPING,  0,  WIZ_SECURE, SHOW|HIDE|NOMOB|NOORDER|FREEZE|SAVE },
 { "cleader",     "���������",   do_cleader,     POS_DEAD,    107,  WIZ_SECURE, SHOW|OLC|NOMOB|FREEZE },
 { "music",       "����",        do_music,       POS_SLEEPING,  0,  WIZ_GSPEAKS,SHOW|MORPH|SPAM|NOMOB },
 { "oauction",    "��������",    do_oauction,    POS_SLEEPING,  0,  WIZ_GSPEAKS,SHOW|SPAM|NOMOB },
 { "order",       "������",      do_order,       POS_RESTING,   0,  WIZ_SKILLS, SHOW|FREEZE|NOLOG },
 { "practice",    "������������",do_practice,    POS_SLEEPING,  0,  WIZ_SECURE, SHOW|NOMOB|NOORDER|FREEZE|SAVE|NOLOG },
 { "rest",        "��������",    do_rest,        POS_SLEEPING,  0,  LOG_NEVER,  SHOW|FREEZE|NOLOG },
 { "sit",         "�����",       do_sit,         POS_SLEEPING,  0,  LOG_NEVER,  SHOW|FREEZE|NOLOG },
 { "sockets",     "�����",       do_sockets,     POS_DEAD,    104,  WIZ_SECURE, SHOW|MORPH|NOLOG|OLC|FREEZE|NOPUB|NOMOB },
 { "rape",        "����������",  do_rape,        POS_FIGHTING,  2,  WIZ_SKILLS, SHOW|NOMOB|NOZRIT|SPAM|FREEZE|NOLOG },
 { "mlove",       "����",        do_rape,        POS_RESTING,   2,  WIZ_SPEAKS, SHOW|NOMOB|NOZRIT|SPAM|FREEZE|NOLOG },
 { "stand",       "������",      do_stand,       POS_SLEEPING,  0,  LOG_NEVER,  SHOW|FREEZE|NOLOG },
 { "stopfoll",    "����������",  do_stopfoll,    POS_DEAD,      0,  WIZ_CONFIG, SHOW|NOMOB|HIDE|FREEZE|NOLOG },
 { "tell",        "�������",     do_tell,        POS_RESTING,   0,  WIZ_SPEAKS, SHOW|MORPH|HIDE|SPAM },
 { "unlock",      "��������",    do_unlock,      POS_RESTING,   0,  WIZ_SECURE, SHOW|FREEZE|NOLOG },
 { "wield",       "�����������", do_wear,        POS_RESTING,   0,  WIZ_SECURE, SHOW|FREEZE|SAVE|NOLOG },
 { "wizhelp",     "����������",  do_wizhelp,     POS_DEAD,    102,  LOG_NEVER,  SHOW|MORPH|HIDE|NOLOG|NOMOB },
 { "spec",        "����",        do_spec,        POS_FIGHTING,  0,  WIZ_SKILLS, SHOW|NOMOB|FREEZE|NOLOG },
 { "spam",        "����",        do_spam,        POS_DEAD,      0,  WIZ_CONFIG, SHOW|NOMOB|HIDE|MORPH|NOORDER|NOLOG },
 { "affects",     "�������",     do_affects,     POS_DEAD,      0,  LOG_NEVER,  SHOW|NOMOB|MORPH|HIDE|NOLOG },
 { "areas",       "����",        do_areas,       POS_DEAD,      0,  LOG_NEVER,  SHOW|MORPH|HIDE|NOMOB|NOLOG },
 { "changes",     "���������",   do_changes,     POS_DEAD,      0,  WIZ_NOTES,  SHOW|MORPH|HIDE|NOMOB|NOLOG },
 { "commands",    "�������",     do_commands,    POS_DEAD,      0,  LOG_NEVER,  SHOW|MORPH|HIDE|NOMOB|NOLOG },
 { "compare",     "��������",    do_compare,     POS_RESTING,   0,  LOG_NEVER,  SHOW|NOMOB|FREEZE|NOLOG },
 { "complain",    "������",      do_complain,    POS_SLEEPING,  0,  WIZ_NOTES,  SHOW|MORPH|HIDE|NOMOB|NOLOG },
 { "consider",    "����������",  do_consider,    POS_RESTING,   0,  WIZ_SECURE, SHOW|NOMOB|FREEZE|NOLOG },
 { "count",       "�������",     do_count,       POS_SLEEPING,  0,  LOG_NEVER,  SHOW|MORPH|HIDE|NOMOB|NOLOG },
 { "credits",     "������",      do_credits,     POS_DEAD,      0,  LOG_NEVER,  SHOW|MORPH|HIDE|NOLOG },
 { "equipment",   "����������",  do_equipment,   POS_DEAD,      0,  WIZ_SECURE, SHOW|HIDE|NOMOB|NOLOG },
 { "examine",     "�������",     do_examine,     POS_RESTING,   0,  LOG_NEVER,  SHOW|NOMOB|NOLOG },
 { "help",        "�������",     do_help,        POS_DEAD,      0,  LOG_NEVER,  SHOW|MORPH|HIDE|NOLOG },
 { "idea",        "����",        do_idea,        POS_DEAD,      0,  WIZ_NOTES,  SHOW|MORPH|HIDE|NOMOB|NOLOG },
 { "bugreport",   "���������",   do_bugreport,   POS_DEAD,      0,  WIZ_NOTES,  SHOW|MORPH|HIDE|NOMOB|NOLOG },
 { "inform",      "����������",  do_inform,      POS_SLEEPING,  0,  LOG_NEVER,  SHOW|HIDE|NOLOG|NOMOB },
 { "motd",        "����",        do_motd,        POS_DEAD,      0,  LOG_NEVER,  MORPH|NOMOB|NOLOG },
 { "news",        "�������",     do_news,        POS_DEAD,      0,  WIZ_NOTES,  SHOW|MORPH|HIDE|NOMOB|NOLOG },
 { "bulletin",    "���������",   do_news,        POS_DEAD,      0,  WIZ_NOTES,  SHOW|MORPH|HIDE|NOMOB|NOLOG },
 { "report",      "������",      do_report,      POS_RESTING,   0,  WIZ_SECURE, SHOW|NOLOG|FREEZE },
 { "magazine",    "������",      do_newspaper,   POS_DEAD,      0,  WIZ_NOTES,  SHOW|MORPH|HIDE|NOMOB|NOLOG },
 { "run",         "������",      do_run,         POS_STANDING,  0,  WIZ_SECURE, SHOW|HIDE|NOMOB|NOORDER|FREEZE|NOLOG },
 { "rules",       "�������",     do_rules,       POS_DEAD,      0,  LOG_NEVER,  SHOW|MORPH|HIDE|NOLOG },
 { "scan",        "�����������", do_scan,        POS_RESTING,   0,  WIZ_SECURE, SHOW|HIDE|NOLOG },
 { "oscore",      "�����������", do_oscore,      POS_DEAD,      0,  LOG_NEVER,  SHOW|MORPH|HIDE|NOLOG|NOMOB },
 { "online",      "������",      do_online,      POS_DEAD,      0,  WIZ_SECURE, SHOW|NOMOB|MORPH|HIDE|NOLOG },
 { "score",       "����������",  do_score,       POS_DEAD,      0,  LOG_NEVER,  SHOW|MORPH|HIDE|NOLOG|NOMOB },
 { "skills",      "������",      do_skills,      POS_DEAD,      0,  LOG_NEVER,  SHOW|MORPH|HIDE|NOLOG|NOMOB },
 { "socials",     "�������",     do_socials,     POS_DEAD,      0,  LOG_NEVER,  SHOW|MORPH|HIDE|NOZRIT|SPAM|FREEZE|NOLOG },
 { "gsocials",    "���������",   do_gsocial,     POS_DEAD,      3,  WIZ_GSPEAKS,SHOW|MORPH|SPAM|FREEZE|NOMOB|NOLOG },
 { "sedit",       "�����",       do_sedit,       POS_DEAD,    103,  WIZ_SECURE, SHOW|OLC|NOMOB },
 { "spells",      "����������",  do_spells,      POS_DEAD,      0,  LOG_NEVER,  SHOW|MORPH|NOLOG|NOMOB },
 { "time",        "�����",       do_time,        POS_DEAD,      0,  LOG_NEVER,  SHOW|MORPH|HIDE|NOLOG|NOMOB },
 { "tournament",  "tournament",  do_tournament,  POS_SLEEPING,  0,  WIZ_GSPEAKS,SHOW|NOORDER|NOMOB|FREEZE|NOPUB },
 { "weather",     "������",      do_weather,     POS_RESTING,   0,  LOG_NEVER,  SHOW|HIDE|NOLOG|NOMOB },
 { "who",         "���",         do_who,         POS_DEAD,      0,  WIZ_SECURE, SHOW|MORPH|HIDE|NOLOG|NOMOB },
 { "whois",       "��������",    do_whois,       POS_DEAD,      0,  WIZ_SECURE, SHOW|MORPH|HIDE|NOLOG|NOMOB },
 { "wizlist",     "����",        do_wizlist,     POS_DEAD,      0,  LOG_NEVER,  SHOW|MORPH|HIDE|NOLOG|NOMOB },
 { "worth",       "�����",       do_worth,       POS_SLEEPING,  0,  WIZ_SECURE, SHOW|HIDE|NOLOG|NOMOB },
 { "charge",      "charge",      do_charge,      POS_STANDING,  0,  WIZ_SKILLS, SHOW|NOMOB|NOORDER|NOLOG|FREEZE },

 // Configuration commands
 { "alias",       "�����",         do_alias,       POS_DEAD,      0,  WIZ_CONFIG, MORPH|HIDE|FULL|NOMOB|NOORDER|FREEZE|SAVE|NOFORCE },
 { "autolist",    "����������",    do_autolist,    POS_DEAD,      0,  WIZ_CONFIG, SHOW|MORPH|HIDE|NOMOB|NOLOG },
 { "autoassist",  "����������",    do_autoassist,  POS_DEAD,      0,  WIZ_CONFIG, SHOW|MORPH|HIDE|NOMOB|FREEZE|SAVE|NOLOG },
 { "autogold",    "����������",    do_autogold,    POS_DEAD,      0,  WIZ_CONFIG, SHOW|MORPH|HIDE|NOMOB|NOLOG },
 { "autoloot",    "��������������",do_autoloot,    POS_DEAD,      0,  WIZ_CONFIG, SHOW|MORPH|HIDE|NOMOB|NOLOG },
 { "autosac",     "����������",    do_autosac,     POS_DEAD,      0,  WIZ_CONFIG, SHOW|MORPH|HIDE|NOMOB|NOLOG },
 { "brief",       "������",        do_brief,       POS_DEAD,      0,  WIZ_CONFIG, SHOW|MORPH|HIDE|NOMOB|NOLOG },
 { "colour",      "�����",         do_colour,      POS_DEAD,      0,  WIZ_CONFIG, SHOW|MORPH|HIDE|NOMOB|NOLOG },
 { "description", "��������",      do_description, POS_DEAD,      0,  WIZ_CONFIG, SHOW|MORPH|HIDE|NOMOB|NOLOG },
 { "remove",      "�����",         do_remove,      POS_RESTING,   0,  WIZ_SECURE, SHOW|FREEZE|NOLOG },
 { "delete",      "�������",       do_delete,      POS_STANDING,  0,  WIZ_SECURE, SHOW|FULL|NOMOB|NOORDER|FREEZE|NOFORCE },
 { "remort",      "�����������",   do_remort,      POS_STANDING,101,  WIZ_SECURE, SHOW|FULL|NOMOB|NOORDER|FREEZE },
 { "nofollow",    "���������",     do_nofollow,    POS_DEAD,      0,  WIZ_CONFIG, SHOW|MORPH|NOLOG },
 { "nosummon",    "�����������",   do_nosummon,    POS_DEAD,      0,  WIZ_CONFIG, SHOW|MORPH|HIDE|NOMOB|NOORDER|NOLOG },
 { "nodelete",    "��������",      do_nodelete,    POS_DEAD,    109,  WIZ_PENALT, SHOW|MORPH|HIDE|NOMOB|FULL },
 { "nocancel",    "��������",      do_nocancel,    POS_DEAD,      0,  WIZ_CONFIG, SHOW|MORPH|HIDE|NOMOB|NOORDER|NOLOG },
 { "nosend",      "�����������",   do_nosend,      POS_DEAD,      0,  WIZ_CONFIG, SHOW|MORPH|HIDE|NOMOB|NOORDER|FREEZE|NOLOG },
 { "outfit",      "������",        do_outfit,      POS_RESTING,   0,  WIZ_SECURE, SHOW|NOMOB|FREEZE|SAVE },
 { "offline",     "�������",       do_offline,     POS_DEAD,      0,  WIZ_SECURE, SHOW|HIDE|NOMOB|OLC|NOPUB },
 { "offer",       "�����",         do_offer,       POS_DEAD,      1,  WIZ_SECURE, SHOW|HIDE|NOMOB|NOORDER|FREEZE },
 { "password",    "������",        do_password,    POS_DEAD,      0,  LOG_NEVER,  SHOW|MORPH|HIDE|NOMOB|NOORDER|SAVE|NOLOG|NOFORCE },
 { "prompt",      "������",        do_prompt,      POS_DEAD,      0,  WIZ_CONFIG, SHOW|MORPH|HIDE|NOMOB|NOLOG },
 { "scroll",      "���������",     do_scroll,      POS_DEAD,      0,  WIZ_CONFIG, SHOW|MORPH|HIDE|NOMOB|NOLOG },
 { "title",       "����",          do_title,       POS_DEAD,      1,  WIZ_CONFIG, SHOW|MORPH|HIDE|NOMOB|NOLOG },
 { "unalias",     "�������",       do_unalias,     POS_DEAD,      0,  WIZ_CONFIG, SHOW|MORPH|HIDE|NOMOB|FULL|NOORDER|FREEZE|SAVE },
 { "wimpy",       "��������",      do_wimpy,       POS_DEAD,      0,  WIZ_CONFIG, SHOW|MORPH|HIDE|FREEZE|NOLOG },

 /*** Communication commands. ***/
 { "afk",         "���",       do_afk,         POS_SLEEPING,  0,  WIZ_CONFIG, SHOW|MORPH|HIDE|NOMOB|FREEZE|SAVE|NOLOG },
 { "answer",      "�����",     do_answer,      POS_SLEEPING,  0,  WIZ_GSPEAKS,SHOW|SPAM|NOMOB },
 { "emote",       "������",    do_emote,       POS_RESTING,   0,  WIZ_SPEAKS, SHOW|SPAM|NOZRIT },
 { "pmote",       "�����",     do_pmote,       POS_RESTING,   0,  WIZ_SPEAKS, SHOW|SPAM|NOZRIT },
 { ".",           "�������",   do_gossip,      POS_SLEEPING,  0,  WIZ_GSPEAKS,SPAM|NOARMY },
 { "gossip",      "�������",   do_gossip,      POS_SLEEPING,  0,  WIZ_GSPEAKS,SHOW|SPAM|NOARMY },
 { ",",           "������",    do_emote,       POS_RESTING,   0,  WIZ_SPEAKS, SPAM|NOZRIT },
 { "newbiechat",  "�������",   do_newbiechat,  POS_SLEEPING,  0,  WIZ_GSPEAKS,SHOW|HIDE|SPAM|NOMOB },
 { "grats",       "����������",do_grats,       POS_SLEEPING,  0,  WIZ_GSPEAKS,SHOW|HIDE|SPAM|NOMOB },
 { "gtell",       "����������",do_gtell,       POS_DEAD,      0,  WIZ_SPEAKS, SHOW|HIDE|SPAM },
 { ";",           ";",         do_gtell,       POS_DEAD,      0,  WIZ_SPEAKS, HIDE|SPAM },
 { "note",        "���������", do_note,        POS_SLEEPING,  0,  WIZ_NOTES,  SHOW|MORPH|HIDE|NOMOB|NOORDER|NOLOG },
 { "offtopic",    "���������", do_offtopic,    POS_SLEEPING,  0,  WIZ_NOTES,  SHOW|MORPH|HIDE|NOMOB|NOORDER|NOLOG },
 { "quest",       "�������",   do_quest,       POS_SLEEPING,  0,  WIZ_SECURE, SHOW|NOMOB|HIDE|FREEZE|SAVE|NOLOG },
 { "question",    "������",    do_question,    POS_SLEEPING,  0,  WIZ_GSPEAKS,SHOW|SPAM },
 { "quenia",      "������",    do_quenia,      POS_RESTING,   0,  WIZ_SPEAKS, SHOW|FREEZE|NOMOB },
 { "quote",       "����������",do_quote,       POS_SLEEPING,  0,  WIZ_GSPEAKS,SHOW|SPAM|NOMOB },
 { "reply",       "��������",  do_reply,       POS_SLEEPING,  0,  WIZ_SPEAKS, SHOW|HIDE|SPAM },
 { "replay",      "���������", do_replay,      POS_SLEEPING,  0,  WIZ_CONFIG, SHOW|MORPH|HIDE|NOMOB },
 { "say",         "����������",do_say,         POS_RESTING,   0,  WIZ_SPEAKS, SHOW|SPAM },
 { "'",           "'",         do_say,         POS_RESTING,   0,  WIZ_SPEAKS, SPAM },
 { "shout",       "��������",  do_shout,       POS_RESTING,   3,  WIZ_GSPEAKS,SHOW|SPAM },
 { "alli",        "��������",  do_alli,        POS_SLEEPING,  0,  WIZ_SPEAKS, SHOW|SPAM|HIDE|NOMOB|MORPH },
 { "yell",        "�����",     do_yell,        POS_RESTING,   0,  WIZ_SPEAKS, SHOW|SPAM },

 { "showprac",    "���������", do_showprac,    POS_SLEEPING,  0,  LOG_NEVER,  SHOW|HIDE|NOMOB|NOLOG },
 { "showskill",   "����������",do_showskill,   POS_DEAD,      0,  LOG_NEVER,  SHOW|MORPH|HIDE|NOMOB|NOLOG },
 { "unread",      "����������",do_unread,      POS_SLEEPING,  0,  LOG_NEVER,  SHOW|MORPH|HIDE|NOMOB|NOLOG },

 /*** Object manipulation commands. ***/
 { "brandish",    "���������", do_brandish,    POS_RESTING,   0,  WIZ_SKILLS, SHOW|NOZRIT|NOALL|FREEZE|SAVE|NOLOG },
 { "close",       "�������",   do_close,       POS_RESTING,   0,  WIZ_SECURE, SHOW|FREEZE|NOLOG },
 { "drink",       "������",    do_drink,       POS_RESTING,   0,  WIZ_SECURE, SHOW|FREEZE|SAVE|NOLOG },
 { "drop",        "�������",   do_drop,        POS_RESTING,   0,  WIZ_SECURE, SHOW|NOZRIT|FREEZE|NOLOG },
 { "eat",         "������",    do_eat,         POS_RESTING,   0,  WIZ_SECURE, SHOW|FREEZE|NOLOG },
 { "envenom",     "��������",  do_envenom,     POS_RESTING,   0,  WIZ_SKILLS, SHOW|FREEZE|NOLOG },
 { "fill",        "���������", do_fill,        POS_RESTING,   0,  WIZ_SECURE, SHOW|NOZRIT|FREEZE|SAVE|NOLOG },
 { "give",        "����",      do_give,        POS_RESTING,   0,  WIZ_SECURE, SHOW|NOZRIT|FREEZE|NOLOG },
 { "send",        "�������",   do_send,        POS_RESTING,   0,  WIZ_SECURE, SHOW|NOMOB|NOZRIT|FREEZE |NOLOG},
 { "heal",        "������",    do_heal,        POS_RESTING,   0,  WIZ_SECURE, SHOW|NOZRIT|FREEZE|SAVE|NOLOG },
 { "hold",        "�����",     do_wear,        POS_RESTING,   0,  WIZ_SECURE, SHOW|NOZRIT|FREEZE|SAVE|NOLOG },
 { "list",        "������",    do_list,        POS_RESTING,   0,  WIZ_SECURE, SHOW|HIDE|FREEZE|NOLOG },
 { "lock",        "��������",  do_lock,        POS_RESTING,   0,  WIZ_SECURE, SHOW|FREEZE|NOLOG },
 { "open",        "�������",   do_open,        POS_RESTING,   0,  WIZ_SECURE, SHOW|FREEZE|NOLOG },
 { "pick",        "��������",  do_pick,        POS_RESTING,   0,  WIZ_SKILLS, SHOW|NOZRIT|FREEZE|NOLOG },
 { "pour",        "����������",do_pour,        POS_RESTING,   0,  WIZ_SECURE, SHOW|FREEZE|SAVE|NOLOG },
 { "push",        "�����",     do_push,        POS_STANDING,  0,  WIZ_SKILLS, SHOW|NOZRIT|FREEZE|NOLOG },
 { "put",         "��������",  do_put,         POS_RESTING,   0,  WIZ_SECURE, SHOW|NOZRIT|FREEZE|NOLOG },
 { "quaff",       "��������",  do_quaff,       POS_RESTING,   0,  WIZ_SECURE, SHOW|NOZRIT|FREEZE|SAVE|NOLOG },
 { "recite",      "��������",  do_recite,      POS_RESTING,   0,  WIZ_SKILLS, SHOW|NOZRIT|NOALL|FREEZE|NOLOG },
 { "sell",        "�������",   do_sell,        POS_RESTING,   0,  WIZ_SECURE, SHOW|NOMOB|FREEZE|NOLOG },
 { "take",        "�����",     do_get,         POS_RESTING,   0,  WIZ_SECURE, SHOW|NOZRIT|FREEZE|NOLOG },
 { "talks",       "���������", do_talk,        POS_DEAD,      0,  WIZ_CONFIG, SHOW|NOMOB|NOORDER|MORPH|HIDE|FREEZE|SAVE },
 { "pose",        "����",      do_pose,        POS_RESTING,   0,  WIZ_CONFIG, SHOW|NOMOB|NOORDER|MORPH|HIDE|NOLOG },
 { "sacrifice",   "����������",do_sacrifice,   POS_RESTING,   0,  WIZ_SACCING,SHOW|NOZRIT|FREEZE|NOLOG },
 { "junk",        "����������������", do_sacrifice,POS_RESTING,0, WIZ_SACCING,NOZRIT|FREEZE|NOLOG },
 { "tap",         "����������",do_sacrifice,   POS_RESTING,   0,  WIZ_SACCING,NOZRIT|FREEZE|NOLOG },
 { "value",       "�������",   do_value,       POS_RESTING,   0,  WIZ_SECURE, SHOW|NOMOB|FREEZE|NOLOG },
 { "wear",        "������",    do_wear,        POS_RESTING,   0,  WIZ_SECURE, SHOW|FREEZE|NOLOG },
 { "zap",         "�������",   do_zap,         POS_RESTING,   0,  WIZ_SKILLS, SHOW|NOZRIT|NOALL|FREEZE|NOLOG },
 { "exchange",    "��������",  do_exchange,    POS_RESTING,   0,  WIZ_SECURE, SHOW|NOMOB|FREEZE|NOLOG },
 { "clanfit",     "���������", do_clanfit,     POS_RESTING,   0,  WIZ_SECURE, SHOW|NOMOB|FREEZE|SAVE },
 { "clanbank",    "��������",  do_clanbank,    POS_RESTING,   0,  WIZ_SECURE, SHOW|NOMOB|NOORDER|OLC|NOARMY|FREEZE|SAVE },
 { "repair",      "������",    do_repair,      POS_STANDING,  0,  WIZ_SKILLS, SHOW|NOMOB|NOORDER|FREEZE|SAVE },
 { "reward",      "���������", do_reward,      POS_DEAD,    106,  WIZ_SECURE, SHOW|NOMOB|NOORDER|FREEZE|SAVE },
 { "damage",      "���������", do_damage,      POS_DEAD,    107,  WIZ_SECURE, SHOW|NOMOB|NOORDER|FREEZE|SAVE },
 { "confiscate",  "�������",   do_seize,       POS_DEAD,    105,  WIZ_SECURE, SHOW|NOMOB|NOORDER|FREEZE|SAVE },
 { "cfix",        "cfix",      do_cfix,        POS_DEAD,    109,  WIZ_CONFIG, SHOW|NOMOB|NOORDER|FREEZE|SAVE },
 { "gfix",        "gfix",      do_gfix,        POS_DEAD,    109,  WIZ_CONFIG, SHOW|NOMOB|NOORDER|FREEZE|SAVE },
 { "forge",       "������",    do_forge,       POS_STANDING,109,  WIZ_SKILLS, SHOW|NOMOB|NOORDER|FREEZE|SAVE },
 { "ldefend",     "�������",   do_ldefend,     POS_STANDING,110,  WIZ_SKILLS, SHOW|NOMOB|NOORDER|FREEZE|SAVE },

 // Combat commands
 { "backstab",    "�������",    do_backstab,    POS_STANDING,  0,  WIZ_SKILLS, SHOW|NOALL|FREEZE|NOLOG },
 { "bash",        "�����",      do_bash,        POS_FIGHTING,  0,  WIZ_SKILLS, SHOW|NOALL|FREEZE|NOLOG },
 { "throw",       "�������",    do_throw,       POS_FIGHTING,  0,  WIZ_SKILLS, SHOW|NOZRIT|NOMOB|FREEZE|NOLOG },
 { "bs",          "��",         do_backstab,    POS_FIGHTING,  0,  WIZ_SKILLS, NOALL|FREEZE|NOLOG },
 { "berserk",     "������",     do_berserk,     POS_FIGHTING,  0,  WIZ_SKILLS, SHOW|FREEZE|NOLOG },
 { "cleave",      "�������",    do_cleave,      POS_FIGHTING,  0,  WIZ_SKILLS, SHOW|NOALL|FREEZE|NOLOG },
 { "dirt",        "��������",   do_dirt,        POS_FIGHTING,  0,  WIZ_SKILLS, SHOW|FREEZE|NOLOG },
 { "disarm",      "�����������",do_disarm,      POS_FIGHTING,  0,  WIZ_SKILLS, SHOW|NOALL|FREEZE|SAVE|NOLOG },
 { "flee",        "�������",    do_flee,        POS_FIGHTING,  0,  WIZ_SKILLS, SHOW|FREEZE|NOLOG },
 { "kick",        "�����",      do_kick,        POS_FIGHTING,  0,  WIZ_SKILLS, SHOW|NOALL|FREEZE|NOLOG },
 { "murder",      "�����",      do_murder,      POS_FIGHTING,  0,  WIZ_SKILLS, SHOW|FULL|HIDE|FREEZE|NOLOG },
 { "rescue",      "������",     do_rescue,      POS_FIGHTING,  0,  WIZ_SKILLS, SHOW|NOZRIT|FREEZE|NOLOG },
 { "trip",        "��������",   do_trip,        POS_FIGHTING,  0,  WIZ_SKILLS, SHOW|NOALL|FREEZE|NOLOG },
 { "pray",        "��������",   do_pray,        POS_FIGHTING,  0,  WIZ_SKILLS, SHOW|NOORDER|FREEZE|NOLOG },
 { "shock",       "�������",    do_shock_hit,   POS_FIGHTING,  0,  WIZ_SKILLS, SHOW|NOALL|FREEZE|NOLOG },
 { "strangle",    "���������",  do_strangle,    POS_STANDING,  0,  WIZ_SKILLS, SHOW|NOALL|FREEZE|NOLOG },
 { "frame",       "����������", do_frame,       POS_FIGHTING,  0,  WIZ_SKILLS, SHOW|NOALL|FREEZE|NOLOG },

 // Miscellaneous commands
 { "polyanarecall","polyanare",  do_polyanarecall,POS_SLEEPING,  0,  LOG_NEVER,  SHOW|HIDE|NOMOB|NOFORCE },
 { "config",      "������������", do_config,      POS_DEAD,      0,  WIZ_CONFIG, SHOW|MORPH|HIDE|NOMOB|NOORDER|SAVE|NOLOG },
 { "seen",        "����",         do_seen,        POS_DEAD,      0,  WIZ_SECURE, SHOW|HIDE|NOMOB|NOPUB },
 { "backup",      "�����",        do_backup,      POS_DEAD,      0,  WIZ_SECURE, SHOW|FULL|NOMOB|NOORDER|OLC|FREEZE|NOPUB },
 { "backup2",     "�����2",       do_backup2,     POS_DEAD,    102,  WIZ_SECURE, SHOW|FULL|NOMOB|NOORDER|OLC|FREEZE|NOPUB },
 { "blink",       "�������",      do_blink,       POS_DEAD,      0,  WIZ_SECURE, SHOW|MORPH|HIDE|NOMOB|SAVE|NOLOG|FREEZE },
 { "enter",       "�����",        do_enter,       POS_STANDING,  0,  WIZ_SECURE, SHOW|FREEZE|NOLOG },
 { "follow",      "���������",    do_follow,      POS_STANDING,  0,  WIZ_SECURE, SHOW|FREEZE|NOLOG },
 { "gain",        "�����",        do_gain,        POS_RESTING,   0,  WIZ_SECURE, SHOW|NOMOB|NOORDER|FREEZE|SAVE },
 { "go",          "�����",        do_enter,       POS_STANDING,  0,  WIZ_SECURE, NOORDER|FREEZE|NOLOG },
 { "hide",        "����������",   do_hide,        POS_RESTING,   0,  WIZ_SKILLS, SHOW|FREEZE|NOLOG },
 { "play",        "���������",    do_play,        POS_RESTING,   2,  WIZ_SECURE, SHOW|FREEZE|NOMOB },
 { "mob",         "���",          do_mob,         POS_DEAD,      0,  LOG_NEVER,  NOORDER|FREEZE|NOFORCE },
 { "quit",        "�����",        do_quit,        POS_DEAD,      0,  WIZ_LOGINS, SHOW|FULL|NOMOB|NOORDER|FREEZE },
 { "quiet",       "�������",      do_quiet,       POS_DEAD,      0,  WIZ_CONFIG, SHOW|FULL|NOMOB|NOORDER|FREEZE|NOLOG },
 { "recall",      "�����",        do_recall,      POS_FIGHTING,  0,  WIZ_SECURE, SHOW|FREEZE|SAVE|NOLOG },
 { "/",           "/",            do_recall,      POS_FIGHTING,  0,  WIZ_SECURE, FREEZE|NOLOG },
 { "save",        "���������",    do_save,        POS_DEAD,      0,  WIZ_SECURE, SHOW|HIDE|NOMOB|NOORDER|FREEZE },
 { "sleep",       "�����",        do_sleep,       POS_SLEEPING,  0,  WIZ_SECURE, SHOW|FREEZE|SAVE|NOLOG },
 { "sneak",       "��������",     do_sneak,       POS_STANDING,  0,  WIZ_SKILLS, SHOW|HIDE|FREEZE|NOLOG },
 { "split",       "��������",     do_split,       POS_RESTING,   0,  WIZ_SECURE, SHOW|FREEZE|NOLOG },
 { "steal",       "�������",      do_steal,       POS_STANDING,  0,  WIZ_SKILLS, SHOW|HIDE|FREEZE },
 { "train",       "�����������",  do_train,       POS_RESTING,   0,  WIZ_SECURE, SHOW|NOMOB|NOORDER|FREEZE|NOLOG },
 { "visible",     "�������",      do_visible,     POS_SLEEPING,  0,  LOG_NEVER,  SHOW|FREEZE|NOLOG },
 { "wake",        "����������",   do_wake,        POS_SLEEPING,  0,  LOG_NEVER,  SHOW|FREEZE|NOLOG },
 { "where",       "���",          do_where,       POS_RESTING,   0,  WIZ_SECURE, SHOW|HIDE|NOLOG|NOMOB },
 { "crimereport", "��������",     do_crimereport, POS_RESTING,   0,  WIZ_SECURE, SHOW|NOMOB|NOORDER|FREEZE },
 { "ahelp",       "��������",     do_ahelp,       POS_DEAD,      0,  WIZ_SECURE, SHOW|MORPH|NOMOB|HIDE|NOLOG },
 
 // Deity commands
 { "devote",      "���������", do_devote,      POS_DEAD,     21,  WIZ_SECURE,  MORPH|HIDE|NOMOB|NOORDER|NOFORCE },
// { "supplicate",  "������",    do_supplicate,  POS_STUNNED,   0,  WIZ_SECURE,  MORPH|HIDE|NOMOB|NOORDER|NOFORCE },
 { "deity",       "������",    do_deity,       POS_DEAD,    109,  WIZ_SECURE,  MORPH|HIDE|NOMOB|NOORDER|NOFORCE },

 // Immortal commands
 { "player",      "unknown",   do_player,      POS_DEAD,    109,  WIZ_SECURE, MORPH|NOMOB|OLC|NOORDER|NOFORCE },
 { "pseudoname",  "unknown",   do_pseudoname,  POS_DEAD,    102,  WIZ_CONFIG, SHOW|MORPH|SAVE|NOMOB },
 { "index",       "������",    do_index,       POS_DEAD,    109,  WIZ_SECURE, MORPH|NOMOB|OLC|NOORDER|NOFORCE },
 { "family",      "�����",     do_family,      POS_DEAD,    104,  LOG_NEVER,  MORPH|HIDE|NOMOB|NOORDER },
 { "srcwrite",    "���",       do_srcwrite,    POS_DEAD,    109,  LOG_NEVER,  MORPH|HIDE|NOMOB|NOPUB|NOORDER },
 { "file",        "����",      do_file,        POS_DEAD,    109,  WIZ_SECURE, MORPH|NOMOB|OLC|NOORDER|NOFORCE },
 { "rename",      "������",    do_rename,      POS_DEAD,    107,  WIZ_SECURE, SHOW|MORPH|OLC|NOMOB|NOPUB|NOORDER },
 { "advance",     "������",    do_advance,     POS_DEAD,    102,  WIZ_SECURE, SHOW|MORPH|OLC|NOMOB|NOFORCE|NOORDER },
 { "dump",        "����",      do_dump,        POS_DEAD,    109,  WIZ_SECURE, MORPH|NOPUB|NOMOB|NOFORCE },
 { "trust",       "unknown",   do_trust,       POS_DEAD,    109,  WIZ_SECURE, SHOW|MORPH|NOMOB|NOFORCE|NOORDER },
 { "allow",       "unban",     do_allow,       POS_DEAD,    106,  WIZ_SECURE, SHOW|MORPH|OLC|NOFORCE },
 { "ban",         "���",       do_ban,         POS_DEAD,    105,  WIZ_SECURE, SHOW|MORPH|OLC|NOPUB|NOFORCE },
 { "pban",        "����",      do_pban,        POS_DEAD,    101,  WIZ_SECURE, SHOW|MORPH|OLC|NOPUB|NOMOB|NOFORCE },
 { "deny",        "unknown",   do_deny,        POS_DEAD,    104,  WIZ_PENALT, SHOW|MORPH|OLC },
 { "tipsy",       "unknown",   do_tipsy,       POS_DEAD,    103,  WIZ_PENALT, SHOW|FULL|MORPH|OLC }, // tipsy by Dinger
 { "disconnect",  "unknown",   do_disconnect,  POS_DEAD,    103,  WIZ_PENALT, SHOW|MORPH|NOMOB },
 { "flag",        "unknown",   do_flag,        POS_DEAD,    109,  WIZ_SECURE, SHOW|MORPH|OLC|NOMOB },
 { "freeze",      "unknown",   do_freeze,      POS_DEAD,    103,  WIZ_PENALT, SHOW|MORPH|OLC },
 { "protect",     "unknown",   do_protect,     POS_DEAD,    109,  WIZ_SECURE, SHOW|MORPH|NOMOB },
 { "reboot",      "unknown",   do_reboot,      POS_DEAD,      0,  WIZ_SECURE, SHOW|FULL|NOMOB|NOORDER|NOFORCE },
 { "set",         "unknown",   do_set,         POS_DEAD,    106,  WIZ_SECURE, SHOW|MORPH|OLC|NOMOB|NOORDER|NOFORCE },
 { "sset",        "unknown",   do_sset,        POS_DEAD,    102,  WIZ_SECURE, SHOW|MORPH|OLC|NOMOB|NOORDER|NOFORCE },
 { "cset",        "unknown",   do_sset,        POS_DEAD,    109,  WIZ_SECURE, SHOW|MORPH|OLC|NOMOB|NOORDER|NOFORCE },
 { "force",       "unknown",   do_force,       POS_DEAD,    102,  WIZ_SECURE, SHOW|MORPH|OLC|NOMOB|NOFORCE|NOORDER },
 { "load",        "unknown",   do_load,        POS_DEAD,    105,  WIZ_LOAD,   SHOW|MORPH|OLC|NOMOB|NOORDER },
 { "nochannels",  "unknown",   do_nochannels,  POS_DEAD,    103,  WIZ_PENALT, SHOW|MORPH|OLC|NOMOB },
 { "noemote",     "unknown",   do_noemote,     POS_DEAD,    103,  WIZ_PENALT, SHOW|MORPH|NOMOB },
 { "nomlove",     "unknown",   do_nomlove,     POS_DEAD,    103,  WIZ_PENALT, SHOW|MORPH|NOMOB },
 { "nogsocial",   "unknown",   do_nogsocial,   POS_DEAD,    103,  WIZ_PENALT, SHOW|MORPH|OLC|NOMOB },
 { "nopost",      "unknown",   do_nopost,      POS_DEAD,    103,  WIZ_PENALT, SHOW|MORPH|NOMOB|OLC },
 { "notell",      "unknown",   do_notell,      POS_DEAD,    103,  WIZ_PENALT, SHOW|MORPH|OLC|NOMOB },
 { "pecho",       "unknown",   do_pecho,       POS_DEAD,    102,  WIZ_SECURE, SHOW|MORPH },
 { "pardon",      "unknown",   do_pardon,      POS_DEAD,    107,  WIZ_SECURE, SHOW|MORPH|OLC },
 { "purge",       "unknown",   do_purge,       POS_DEAD,    106,  WIZ_SECURE, SHOW|OLC },
 { "restore",     "unknown",   do_restore,     POS_DEAD,    103,  WIZ_RESTORE,SHOW|MORPH|OLC },
 { "slay",        "unknown",   do_slay,        POS_DEAD,    103,  WIZ_SECURE, SHOW|FULL },
 { "transfer",    "unknown",   do_transfer,    POS_DEAD,    103,  WIZ_SECURE, SHOW|MORPH|OLC },
 { "poofin",      "unknown",   do_bamfin,      POS_DEAD,    102,  WIZ_CONFIG, SHOW|MORPH|SAVE|NOMOB|NOLOG },
 { "poofout",     "unknown",   do_bamfout,     POS_DEAD,    102,  WIZ_CONFIG, SHOW|MORPH|SAVE|NOMOB|NOLOG },
 { "gecho",       "unknown",   do_echo,        POS_DEAD,    102,  WIZ_GSPEAKS,SHOW|MORPH },
 { "holylight",   "unknown",   do_holylight,   POS_DEAD,    102,  WIZ_CONFIG, SHOW|MORPH|NOMOB|NOLOG },
 { "incognito",   "unknown",   do_incognito,   POS_DEAD,    102,  WIZ_CONFIG, SHOW|MORPH|NOMOB|NOLOG },
 { "log",         "unknown",   do_log,         POS_DEAD,    109,  WIZ_SECURE, SHOW|MORPH|NOMOB|NOFORCE|NOORDER },
 { "memory",      "unknown",   do_memory,      POS_DEAD,    107,  LOG_NEVER,  SHOW|MORPH|NOPUB|NOMOB },
 { "mwhere",      "unknown",   do_mwhere,      POS_DEAD,    102,  WIZ_SECURE, SHOW|MORPH|NOMOB },
 { "owhere",      "unknown",   do_owhere,      POS_DEAD,    102,  WIZ_SECURE, SHOW|MORPH|NOMOB },
 { "peace",       "unknown",   do_peace,       POS_DEAD,    102,  WIZ_SECURE, SHOW|OLC|NOORDER|NOFORCE },
 { "penalty",     "unknown",   do_penalty,     POS_DEAD,    103,  WIZ_PENALT, SHOW|MORPH|NOMOB },
 { "echo",        "unknown",   do_recho,       POS_DEAD,    102,  WIZ_SPEAKS, SHOW|MORPH },
 { "snoop",       "unknown",   do_snoop,       POS_DEAD,    106,  WIZ_SNOOPS, SHOW|MORPH|OLC|NOPUB|NOMOB },
 { "stat",        "unknown",   do_stat,        POS_DEAD,    103,  WIZ_SECURE, SHOW|MORPH|HIDE|NOMOB },
 { "string",      "unknown",   do_string,      POS_DEAD,    104,  WIZ_SECURE, SHOW|MORPH|NOORDER },
 { "wizinvis",    "unknown",   do_invis,       POS_DEAD,    102,  WIZ_SECURE, SHOW|MORPH|NOMOB },
 { "vnum",        "unknown",   do_vnum,        POS_DEAD,    103,  LOG_NEVER,  SHOW|MORPH|NOMOB },
 { "zecho",       "unknown",   do_zecho,       POS_DEAD,    102,  WIZ_SPEAKS, SHOW|MORPH },
 { "clone",       "unknown",   do_clone,       POS_DEAD,    109,  WIZ_LOAD,   SHOW|NOPUB|NOMOB },
 { "wiznet",      "unknown",   do_wiznet,      POS_DEAD,    102,  WIZ_CONFIG, SHOW|MORPH|NOMOB },
 { "immtalk",     "unknown",   do_immtalk,     POS_DEAD,      0,  WIZ_GSPEAKS,SHOW|MORPH|NOORDER|HIDE },
 { ":",           "unknown",   do_immtalk,     POS_DEAD,      0,  WIZ_GSPEAKS,MORPH },
 { "smote",       "unknown",   do_smote,       POS_DEAD,    102,  WIZ_SPEAKS, SHOW|MORPH },
 { "prefix",      "unknown",   do_prefix,      POS_DEAD,    102,  LOG_NEVER,  SHOW|MORPH|FULL },
 { "nuke",        "unknown",   do_nuke,        POS_SLEEPING,  0,  WIZ_SECURE, SHOW|NOMOB|FREEZE|SAVE },
 { "vislist",     "unknown",   do_vislist,     POS_DEAD,    102,  WIZ_SKILLS, SHOW|NOMOB|HIDE|NOORDER},
 { "ignorelist",  "unknown",   do_ignorelist,  POS_DEAD,    106,  WIZ_SKILLS, SHOW|NOMOB|HIDE|NOORDER},
 { "setcurse",    "��������",  do_setcurse,    POS_DEAD,    107,  WIZ_PENALT, SHOW|NOMOB|OLC|FREEZE|NOPUB|NOORDER },

 // Race Skills
 { "tail",        "�����",     do_tail,        POS_FIGHTING,  0,  WIZ_SKILLS, SHOW|NOORDER|FREEZE|NOLOG },
 { "crush",       "��������",  do_crush,       POS_FIGHTING,  0,  WIZ_SKILLS, SHOW|NOORDER|FREEZE|NOLOG },
 { "blacksmith",  "����������",do_blacksmith,  POS_STANDING,  0,  WIZ_SECURE, SHOW|HIDE|NOMOB|NOORDER|FREEZE },
 { "vbite",       "����",      do_vbite,       POS_FIGHTING,  0,  WIZ_SKILLS, SHOW|NOORDER|NOMOB|FREEZE|NOLOG },
 { "resurrect",   "���������", do_resurrect,   POS_FIGHTING,  0,  WIZ_SKILLS, SHOW|NOMOB|FREEZE|NOLOG },

 { "travel",      "��������������",   do_travel,      POS_STANDING,  0,  WIZ_SECURE, NOMOB|FREEZE },

 // OLC
 { "edit",        "unknown",     do_olc,         POS_DEAD,    103,  WIZ_SECURE, SHOW|OLC|NOMOB },
 { "asave",       "unknown",     do_asave,       POS_DEAD,    103,  WIZ_SECURE, SHOW|OLC|NOMOB|NOFORCE },
 { "alist",       "unknown",     do_alist,       POS_DEAD,    103,  LOG_NEVER,  SHOW|NOMOB|NOLOG },
 { "resets",      "unknown",     do_resets,      POS_DEAD,    103,  LOG_NEVER,  SHOW|NOMOB },
 { "redit",       "unknown",     do_redit,       POS_DEAD,    103,  LOG_NEVER,  SHOW|OLC|NOMOB },
 { "medit",       "unknown",     do_medit,       POS_DEAD,    103,  LOG_NEVER,  SHOW|OLC|NOMOB },
 { "aedit",       "unknown",     do_aedit,       POS_DEAD,    103,  LOG_NEVER,  SHOW|OLC|NOMOB },
 { "oedit",       "unknown",     do_oedit,       POS_DEAD,    103,  LOG_NEVER,  SHOW|OLC|NOMOB },
 { "qstat",       "unknown",     do_qstat,       POS_DEAD,    103,  WIZ_SECURE, SHOW|NOMOB|NOLOG },
 { "pcheck",      "���������",   do_check,       POS_DEAD,    109,  WIZ_SECURE, NOMOB|OLC|NOPUB|NOORDER|FULL|NOLOG },
 { "skillstat",   "unknown",     do_skillstat,   POS_DEAD,    103,  LOG_NEVER,  SHOW|NOMOB|NOLOG },
 { "spellstat",   "unknown",     do_spellstat,   POS_DEAD,    103,  LOG_NEVER,  SHOW|NOMOB|NOLOG },
 { "arenarecall", "�����",       do_arecall,     POS_FIGHTING,  0,  WIZ_SECURE, SHOW|NOADR|NOORDER|SAVE|NOMOB|NOLOG|FREEZE },
 { "addlag",      "unknown",     do_addlag,      POS_DEAD,    103,  WIZ_PENALT, SHOW|OLC|NOPUB|NOMOB },
 { "itemlist",    "unknown",     do_itemlist,    POS_DEAD,    110,  WIZ_SECURE, SHOW|OLC|NOPUB|NOMOB },
 { "moblist",     "unknown",     do_moblist,     POS_DEAD,    110,  WIZ_SECURE, SHOW|OLC|NOPUB|NOMOB },
 { "mpdump",      "unknown",     do_mpdump,      POS_DEAD,    105,  LOG_NEVER,  SHOW|NOMOB },
 { "mpstat",      "unknown",     do_mpstat,      POS_DEAD,    105,  LOG_NEVER,  SHOW|NOMOB|NOLOG },
 { "mpsave",      "unknown",     do_mpsave,      POS_DEAD,    105,  WIZ_SECURE, SHOW|OLC|NOMOB|NOLOG },
 { "mpedit",      "unknown",     do_mpedit,      POS_DEAD,    105,  WIZ_SECURE, SHOW|OLC|NOMOB },
 { "mplist",      "unknown",     do_mplist,      POS_DEAD,    105,  LOG_NEVER,  SHOW|NOMOB|NOLOG },
 { "vlist",       "unknown",     do_vlist,       POS_DEAD,    103,  LOG_NEVER,  SHOW|NOMOB|OLC|NOLOG },
 { "rlist",       "unknown",     do_rlist,       POS_DEAD,    103,  LOG_NEVER,  SHOW|NOMOB|OLC|NOLOG },
 { "lore",        "������",      do_lore,        POS_RESTING,   0,  WIZ_SKILLS, SHOW|FREEZE|NOMOB|NOLOG|FREEZE },
 { "morph",       "������������",do_polymorph,   POS_RESTING,   0,  WIZ_SECURE, SHOW|HIDE|NOMOB|NOORDER|FREEZE },
 { "reform",      "������������",do_reform,      POS_RESTING,   0,  WIZ_SECURE, SHOW|MORPH|HIDE|NOMOB|NOORDER|FREEZE|SAVE },
 { "referi",      "������",      do_referi,      POS_STANDING,  0,  WIZ_SECURE, SHOW|HIDE|NOMOB|NOORDER|FREEZE|NOLOG },
 { "test",        "unknown",     do_test,        POS_RESTING,   1,  LOG_NEVER,  SHOW|MORPH|HIDE|NOMOB|FREEZE|NOPUB },
 { "version",     "������",      do_version,     POS_DEAD,      0,  WIZ_SECURE, SHOW|MORPH|HIDE|NOMOB },
 { "vote",        "����������",  do_vote,        POS_DEAD,      0,  WIZ_SECURE, SHOW|MORPH|HIDE|NOMOB|NOLOG },
 { "global",      "������",      do_global,      POS_DEAD,    103,  WIZ_CONFIG, SHOW|MORPH|HIDE|NOMOB|OLC },
 { "suicide",     "�������",     do_suicide,     POS_DEAD,      0,  WIZ_SECURE, SHOW|MORPH|HIDE|NOMOB|FULL|NOADR|NOORDER|FREEZE|SAVE },
 { "diplomacy",   "����������",  do_diplomacy,   POS_DEAD,      1,  WIZ_CONFIG, SHOW|MORPH|HIDE|NOMOB|NOORDER|FREEZE },
 { "room",        "unknown",     do_room,        POS_DEAD,    103,  WIZ_SECURE, SHOW|MORPH|HIDE|NOMOB|NOLOG },
 { "addpenalty",  "unknown",     do_add_penalty, POS_DEAD,    103,  WIZ_SECURE, SHOW|MORPH|HIDE|NOMOB|OLC },
 { "kazad",       "�����",       do_kazad,       POS_DEAD,      0,  WIZ_SPEAKS, SHOW|NOORDER|MORPH|HIDE|NOMOB },
 { "avenge",      "�������",     do_avenge,      POS_DEAD,      0,  WIZ_SPEAKS, SHOW|NOORDER|MORPH|HIDE|NOMOB },
 { "gaccount",    "�����",       do_gaccount,    POS_DEAD,      0,  WIZ_SKILLS, SHOW|NOORDER|MORPH|HIDE|NOMOB|FREEZE|SAVE },
 { "setclass",    "unknown",     do_setclass,    POS_SLEEPING,103,  WIZ_SECURE, SHOW|HIDE|NOMOB|NOORDER|OLC},
 { "dampool",     "unknown",     do_dampool,     POS_SLEEPING,102,  WIZ_SECURE, SHOW|HIDE|NOMOB|NOORDER},
 { "marry",       "�����������", do_marry,       POS_STANDING,  1,  WIZ_SKILLS, SHOW|NOMOB|NOORDER|NOADR|FREEZE|OLC},
 { "ptalk",       "��������",    do_ptalk,       POS_DEAD,      1,  WIZ_SPEAKS, SHOW|NOMOB|HIDE|MORPH },
 { "divorce",     "������",      do_divorce,     POS_STANDING,  1,  WIZ_SKILLS, SHOW|NOMOB|HIDE|NOORDER|FREEZE|OLC},
 { "race",        "����",        do_race,        POS_DEAD,    106,  WIZ_SECURE, SHOW|NOMOB|HIDE|OLC|NOORDER},
 { "fly",         "��������",    do_fly,         POS_FIGHTING,  1,  WIZ_SKILLS, SHOW|FREEZE|NOLOG},
 { "walk",        "�����������", do_walk,        POS_FIGHTING,  1,  WIZ_SKILLS, SHOW|FREEZE|NOLOG},
 { "smoke",       "������",      do_smoke,       POS_RESTING,   1,  WIZ_SPEAKS, HIDE|NOMOB|NOORDER|NOPUB|FREEZE },
 { "russian",     "�������",     do_russian,     POS_DEAD,      1,  WIZ_SECURE, SHOW|MORPH|NOORDER|NOMOB|FREEZE },
 { "mist",        "�����",       do_mist,        POS_STANDING,  1,  WIZ_SKILLS, SHOW|FREEZE|NOLOG },
 { "howl",        "���",         do_howl,        POS_STANDING,  1,  WIZ_SKILLS, SHOW|MORPH|NOMOB|FREEZE|NOLOG },
 { "gquest",      "������",      do_gquest,      POS_DEAD,      1,  WIZ_SKILLS, HIDE|MORPH|NOMOB|FREEZE|NOARMY|NOORDER },
 { "lash",        "���",         do_lash,        POS_FIGHTING,  1,  WIZ_SKILLS, MORPH|NOMOB|FREEZE|NOLOG },
 { "dig",         "����������",  do_dig,         POS_DEAD,      1,  WIZ_SKILLS, MORPH|NOMOB|FREEZE },
 { "pacify",      "��..�����",   do_pacify,      POS_STANDING,  1,  WIZ_SKILLS, SHOW|NOMOB|FREEZE|NOARMY|NOALL },
 { "",             0,            0,              POS_DEAD,      0,  LOG_NEVER,  HIDE }
};

void interpret( CHAR_DATA *ch, const char *argument )
{
  unsigned char command[MAX_STRING_LENGTH];
  unsigned char logline[MAX_STRING_LENGTH];
  struct cmd_type *cmd_ptr = NULL;
  int cmd, trust;

  // Strip leading spaces.
  while ( isspace(*argument) ) argument++;
  if ( argument[0] == '\0' )   return;
  smash_tilde((char*)argument);

// Grab the command word. Special parsing so ' can be a command,
//   also no spaces needed after punctuation.
  strcpy( logline, argument );

  if (!IS_LETTER(argument[0]) && !IS_DIGIT(argument[0]))
  {
    command[0] = argument[0];
    command[1] = '\0';
    argument++;
    while ( isspace(*argument) ) argument++;
  }
  else argument = one_argument( argument, command );

  // Look for command in command table

  trust = get_trust( ch );
  //printf("Name %s\n", ch->name); // prool
  if (IS_NPC(ch)) trust=UMIN(101,trust);
  command[0]=LOWER(command[0]);
  for ( cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ )
  {
    if(cmd_table[cmd].level > trust) continue;
    if(command[0]!=cmd_table[cmd].name[0]) continue;
    if(IS_SET(cmd_table[cmd].flag,FULL)    ?
     str_cmp( command, cmd_table[cmd].name):
     str_prefix(command,cmd_table[cmd].name)) continue;
    if (IS_SET(global_cfg,CFG_PUBLIC) && IS_SET(cmd_table[cmd].flag,NOPUB)) continue;
    {
      cmd_ptr = &cmd_table[cmd];
      break;
    }
  }
  /* what is it for? To enable non-public commands in public version? */
  if (!cmd_ptr)
  {
    for ( cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ )
    {
      if(command[0]==cmd_table[cmd].rname[0] &&
        IS_SET(cmd_table[cmd].flag,FULL)?!str_cmp(command,cmd_table[cmd].rname):
        !str_prefix(command,cmd_table[cmd].rname)
        && cmd_table[cmd].level<=trust)
      {
       cmd_ptr = &cmd_table[cmd];
       break;
      }
    }
  }
#ifdef WITH_DSO
  if (!cmd_ptr)
    {
      struct command *c;
      CMDS_FOREACH (c)
        if (command[0] == c->cmd.name[0] && c->cmd.level <= trust &&
          ((c->cm_nice < 100 && !str_prefix (command, c->cmd.name))
           || !str_cmp (command, c->cmd.name)))
        {
          cmd_ptr = &c->cmd;
          break;
        }
    }
#endif

  if ( !cmd_ptr )
  {
    if (!check_social(ch,command,argument)) stc("���?\n\r",ch);
    return;
  }

  if (IS_NPC(ch) && IS_SET(cmd_ptr->flag,NOMOB))
  {
   stc("�� �� ������ ������� ���.\n\r",ch);
   return;
  }

  if( IS_SET(cmd_ptr->flag, DENY) )
  {
    stc("{R��� ������� ��������� ������!{x\n\r",ch);
    return;
  }

  if (!IS_NPC(ch))
  {
    if (IS_SET(ch->pcdata->cfg,CFG_ZRITEL) && IS_SET(cmd_ptr->flag,NOZRIT))
    {
     stc("�� �� ������ �������,����� �� �������.\n\r",ch);
     return;
    }

    if (IS_SET(ch->act,PLR_FREEZE) && IS_SET(cmd_ptr->flag,FREEZE))
    {
      stc( "�� ��������� ���������!\n\r", ch );
      return;
    }
    if (ch->morph_obj!=NULL && !IS_SET(cmd_ptr->flag,MORPH))
    {
      stc("�� ������ ������� ��������� � ���� ���������� �����.\n\r",ch);
      return;
    };
    if (IS_SET(cmd_ptr->flag,NOADR)&& ch->pcdata->condition[COND_ADRENOLIN]!=0)
    {
      stc("���� ���� ����������.\n\r",ch);
      return;
    }
    if (IS_SET(cmd_ptr->flag,NOARMY) && IS_SET(ch->act,PLR_ARMY))
    {
      stc("�� �� ������ ������ ��� � �����.\n\r",ch);
      return;
    };
  }

  if ( (cmd_ptr->name == "east") || (cmd_ptr->name == "west")
      || (cmd_ptr->name == "north") || (cmd_ptr->name == "south")
      || (cmd_ptr->name == "up") || (cmd_ptr->name == "down") )
   {
     if ( (!IS_SET(cmd_ptr->flag, HIDE) )
       && ( ( !IS_AFFECTED(ch,AFF_HIDE) )
            || ( !IS_AFFECTED(ch,AFF_SNEAK) )
            || (ch->clan == NULL) ))
       REM_BIT( ch->affected_by, AFF_HIDE );
   }
   else if (!IS_SET(cmd_ptr->flag, HIDE)) REM_BIT( ch->affected_by, AFF_HIDE );

  // Character not in position for command?
  if ( ch->position < cmd_ptr->position && ch->trust<111)
  {
    switch( ch->position )
    {
      case POS_DEAD:stc( "���� ��������, �� ����.\n\r", ch );break;
      case POS_MORTAL:
      case POS_INCAP:stc( "�� ������� �������.\n\r", ch );break;
      case POS_STUNNED:stc( "�� ������� � �� ������ ����� �������.\n\r", ch );break;
      case POS_SLEEPING:stc( "�� �� �����!\n\r", ch );break;
      case POS_RESTING:stc( "���...�������� ���� �������...\n\r", ch);break;
      case POS_SITTING:stc( "������� ������.\n\r",ch);break;
      case POS_FIGHTING:stc( "�� ��� ��� ����������!\n\r", ch);break;
    }
    return;
  }

  if (IS_SET(ch->act, PLR_LOG) || fLogAll || cmd_ptr->log!=LOG_NEVER)
  {
    smash_percent(logline);
    do_printf(log_buf,"%s%s%s",ch->name,get_logchar(cmd_ptr->log),logline);
    if (!NOPUBLIC && !IS_SET(cmd_ptr->flag,NOLOG)) log_string(log_buf);
    if (!IS_SET(ch->comm,COMM_COMSYS))
    {
      smash_dollar(log_buf);
      wiznet(log_buf,ch,NULL,cmd_ptr->log,get_loglevel(cmd_ptr->log));
    }
  }

  if (IS_SET(cmd_ptr->flag,OLC))
  {
    char tmp[MAX_STRING_LENGTH];
    strftime(tmp,19,"%d%m %a %H:%M:%S:",localtime(&current_time));
    do_printf(log_buf,"%s[%s]%s\n",tmp,ch->name,logline);
    ID_FILE=OLC_FILE;
    stf(log_buf,NULL);
  }
  if (IS_SET(cmd_ptr->flag,SAVE)) WILLSAVE(ch);

  if ( ch->desc && ch->desc->snoop_by)
  {
    write_to_buffer( ch->desc->snoop_by, "% ",    2 );
    write_to_buffer( ch->desc->snoop_by, logline, 0 );
    write_to_buffer( ch->desc->snoop_by, "\n\r",  2 );
  }
 (*cmd_ptr->do_fun) ( ch, argument );
 tail_chain( );
}

// function to keep argument safe in all commands -- no static strings
void do_function (CHAR_DATA *ch, DO_FUN *do_fun, const char *argument)
{
 const char *command_string;

 command_string = str_dup(argument);
 (*do_fun) (ch, command_string);
 free_string(command_string);
}

bool check_social( CHAR_DATA *ch, char *command, const char *argument )
{
 char arg[MAX_STRING_LENGTH];
 CHAR_DATA *victim;
 int cmd;
 bool found;

 found  = FALSE;
 for ( cmd = 0; social_table[cmd].name[0] != '\0'; cmd++ )
 {
   if ( command[0] == social_table[cmd].name[0]
        &&   !str_prefix( command, social_table[cmd].name ) )
    {
     found = TRUE;
     break;
    }
 }


 if ( !found ) return FALSE;

 if (IS_NPC(ch) && IS_SET(social_table[cmd].flag,SOC_NOMOB))
 {
   do_function(ch,&do_emote,"���������� ������ �������, �� ������� �������.");
   return TRUE;
 }

 if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE) )
 {
  stc( "�� �� ������ ����� �������!\n\r", ch );
  return TRUE;
 }

 switch ( ch->position )
 {
    case POS_DEAD: stc( "���� ������, �� ����.\n\r", ch ); return TRUE;
    case POS_INCAP:
    case POS_MORTAL:stc( "�� ������� �������.\n\r", ch );return TRUE;
    case POS_STUNNED:stc( "�� ������� � �� ������ ����� �������.\n\r", ch );return TRUE;
    case POS_SLEEPING:
        if ( !str_cmp( social_table[cmd].name, "snore" ) ) break;
        stc( "�� �� �����!\n\r", ch );
        return TRUE;
  }

 if (IS_SET(ch->act,PLR_TIPSY)) if (tipsy(ch,"social")) return TRUE;

 if (IS_SET(social_table[cmd].flag,SOC_CULTURAL))
 {
   if (ch->move<3)
   {
     stc("�� ������� �����.\n\r",ch);
     return TRUE;
   }
   ch->move-=3;
 }

 one_argument( argument, arg );
 victim = NULL;
 if ( arg[0] == '\0' )
 {
  act( social_table[cmd].others_no_arg, ch, NULL, victim, TO_ROOM    );
  act( social_table[cmd].char_no_arg,   ch, NULL, victim, TO_CHAR    );
  return TRUE;
 }

 victim=get_char_room(ch, arg);
 if (victim==NULL) victim = get_pchar_world(ch, arg);
 if (victim==NULL)
 {
  stc("��� ����� ���.\n\r", ch);
  return TRUE;
 }
 if (victim->position==POS_SLEEPING)
 {
   act( "$O ���� � �� ����� ����.", ch, NULL, victim, TO_CHAR);
   return TRUE;
 }
 if ( victim == ch )
 {
  act( social_table[cmd].others_auto,   ch, NULL, victim, TO_ROOM );
  act( social_table[cmd].char_auto,     ch, NULL, victim, TO_CHAR );
 }
 else
 {
  act( social_table[cmd].others_found,  ch, NULL, victim, TO_NOTVICT );
  act( social_table[cmd].char_found,    ch, NULL, victim, TO_CHAR    );
  act( social_table[cmd].vict_found,    ch, NULL, victim, TO_VICT    ); 

  if ( !IS_NPC(ch) && IS_NPC(victim) && !IS_AFFECTED(victim, AFF_CHARM)
     && IS_AWAKE(victim) && victim->desc == NULL)
   {
    switch ( number_bits( 4 ) )
    {
     case 0:

     case 1: case 2: case 3: case 4:
     case 5: case 6: case 7: case 8:
       act( social_table[cmd].others_found,victim, NULL, ch, TO_NOTVICT );
       act( social_table[cmd].char_found,victim, NULL, ch, TO_CHAR    ); 
       act( social_table[cmd].vict_found,victim, NULL, ch, TO_VICT    ); 
       break;

     case 9: case 10: case 11: case 12:
       act( "$c1 ���� �������� $C2.",  victim, NULL, ch, TO_NOTVICT );
       act( "�� ����� �������� $C2.",  victim, NULL, ch, TO_CHAR    );
       act( "$c1 ���� ���� ��������.", victim, NULL, ch, TO_VICT    );
       break;
    }
   }
  }
 return TRUE;
}

// Return true if an argument is completely numeric.
bool is_number ( const char *arg )
{
 if ( *arg == '\0' ) return FALSE;
 if ( *arg == '+' || *arg == '-' ) arg++;

 for ( ; *arg != '\0'; arg++ )
 {
  if ( !isdigit( *arg ) ) return FALSE;
 }
 return TRUE;
}

// Given a string like 14.foo, return 14 and 'foo'
int number_argument( char *argument, char *arg )
{
 char *pdot;
 int number;

 for ( pdot = argument; *pdot != '\0'; pdot++ )
 {
  if ( *pdot == '.' )
  {
   *pdot = '\0';
   number = atoi( argument );
   *pdot = '.';
   strcpy( arg, pdot+1 );
   return number;
  }
 }
 strcpy( arg, argument );
 return 1;
}

// Given a string like 14*foo, return 14 and 'foo'
int mult_argument(char *argument, char *arg)
{
 char *pdot;
 int number;

 for ( pdot = argument; *pdot != '\0'; pdot++ )
 {
  if ( *pdot == '*' )
  {
   *pdot = '\0';
   number = atoi( argument );
   *pdot = '*';
   strcpy( arg, pdot+1 );
   return number;
  }
 }
 
 strcpy( arg, argument );
 return 1;
}

// Pick off one argument from a string and return the rest. Understands quotes.
const char *one_argument( const char *argument, char *arg_first )
{
 char cEnd;

 while ( isspace(*argument) ) argument++;

 cEnd = ' ';
 if ( *argument == '\'' || *argument == '"' ) cEnd = *argument++;

 while ( *argument != '\0' )
 {
   if ( *argument == cEnd )
   {
    argument++;
    break;
   }
   *arg_first = LOWER(*argument);
   arg_first++;
   argument++;
 }
 *arg_first = '\0';
 while ( isspace(*argument) ) argument++;
 return argument;
}

// Contributed by Alander.
void do_commands( CHAR_DATA *ch, const char *argument )
{
  int cmd=0, col=0;
 

  ptc(ch, "{G[{Y%-12s %12s{G] [{Y%-12s %12s{G] [{Y%-12s %12s{G]\n\r",
     "English","�������","English","�������","English","�������");
  for (;cmd_table[cmd].name[0]!= '\0'; cmd++ )
  {
    if (cmd_table[cmd].level<LEVEL_HERO
      && cmd_table[cmd].level<=get_trust(ch)
      && IS_SET(cmd_table[cmd].flag,SHOW))
    {
     ptc(ch,"{G[{Y%-12s %12s{G] ",cmd_table[cmd].name,cmd_table[cmd].rname);
     if (++col % 3==0) stc( "{x\n\r", ch );
    }
  }
#ifdef WITH_DSO
  {
  struct command *c;
  CMDS_FOREACH(c)
        if (c->cmd.level < LEVEL_HERO
          && c->cmd.level <= get_trust(ch)
          && IS_SET(c->cmd.flag,SHOW)) {
             ptc(ch,"{G[{Y%-12s %12s{G] ",c->cmd.name,c->cmd.rname);
             if (++col % 3==0) stc( "{x\n\r", ch );
        }
  }
#endif
  if (col % 3!=0) stc("{x\n\r",ch);
  return;
}

void do_wizhelp( CHAR_DATA *ch, const char *argument )
{
  int level,cmd,col=0;
 
  for (level=102;level<=get_trust(ch);level++)
  {
    col=0;
    ptc(ch,"{C%3d:{x",level);
      
    for ( cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ )
    {                                            
      if ( cmd_table[cmd].level == level
/*        && IS_SET(cmd_table[cmd].flag, SHOW)*/)
      {
        if (col == 6)
        {
          col=0;
          stc("    ",ch);
        }
        ptc( ch, "%-12s", cmd_table[cmd].name );
        if (++col % 6 == 0 ) stc("\n\r", ch );
      }                                   
    }                                      
    if (col % 6 != 0) stc("{x\n\r", ch);
  }

#ifdef WITH_DSO
  {
  struct command *c;
  CMDS_FOREACH(c)
    if (c->cmd.level >= LEVEL_HERO
      && c->cmd.level <= get_trust(ch)
      && IS_SET(c->cmd.flag,SHOW)) 
    {
      ptc(ch,"%-12s",c->cmd.name);
      if (++col % 6 == 0) stc( "\n\r", ch );
    }
  }
#endif
//  if ( col % 6 != 0 ) stc( "\n\r", ch );
  return;
}

void do_stopfoll( CHAR_DATA *ch, const char *argument )
{
 if (IS_AFFECTED(ch, AFF_CHARM) && ch->master!=NULL)
 {
   stc("�������� �������� �������??\n\r",ch);
   return;
 }
 if ( IS_SET(ch->affected_by, AFF_CHARM))
  {
      REM_BIT(ch->affected_by, AFF_CHARM);
  }

 die_follower(ch);
}

void do_ahelp(CHAR_DATA *ch, const char *argument)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  bool found=FALSE;

  argument=one_argument(argument,arg1);
  argument=one_argument(argument,arg2);

  if (arg1[0]=='\0')
  {
    stc("{C Advanced Help ���������� ����������, �������� �� �� �������� ����.\n\r",ch);
    stc("��� ���������� �������� ���������� � �� ������� ������� ����������.\n\r",ch);
    stc("Advanced Help ���������:\n\r{x",ch);
    stc(" ahelp race\n\r",ch);
    stc(" ahelp skill\n\r",ch);
    stc(" ahelp class\n\r",ch);
    stc(" ahelp stat <race>\n\r",ch);
    stc(" ahelp group\n\r",ch);
    stc(" ahelp damage\n\r",ch);
    stc(" ahelp material\n\r",ch);
    return;
  }
  
  if (!str_prefix(arg1,"race"))
  {
    int race;
    int64 spec;
    if (arg2[0]=='\0')
    {
      stc("Syntax: ahelp race <��� ����>\n\r\n\r",ch);
      stc("������ ��������� ���:\n\r",ch);
      stc("{W#No {C��� ����     {G�������� {W#No {C��� ����     {G��������\n\r",ch);
      for ( race = 1; race_table[race].name; race+=2 )
      {
       ptc(ch,"{W%2d {C%15s %s",race,race_table[race].name,
         (race_table[race].pc_race==TRUE) ? "{G��{x " : "{R���{x");
       if (race_table[race+1].name) ptc(ch,"     {W%2d {C%15s %s\n\r",race+1,race_table[race+1].name,
         (race_table[race+1].pc_race==TRUE) ? "{G��{x " : "{R���{x");
       else stc("\n\r",ch);
      }
      return;
    }

    for ( race = 1; race_table[race].name != NULL; race++ )
    {
      if (!str_prefix(arg2,race_table[race].name))
      {
        found=TRUE;
        break;
      }
    }
    if (!found)
    {
      stc("��� ����� ����.\n\r",ch);
      return;
    }
    stc("{C=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n\r",ch);
    ptc(ch,"| Advanced help: ����{Y %s  {x( %s )\n\r",
      race_table[race].name,race_table[race].who_name);
    stc("{C=-------------------------------------------------------------------=\n\r",ch);
      ptc(ch,"{CCreation Points:{Y %3d {C������:{Y %s\n\r",race_table[race].points,size_table[race_table[race].size].name);
      ptc(ch,"��� �����: %s\n\r",race_table[race].hand_dam);
      ptc(ch,"{C���������� ������:{G %s %s %s %s %s\n\r",race_table[race].skills[0],
        race_table[race].skills[1],race_table[race].skills[2],
        race_table[race].skills[3],race_table[race].skills[4]);
      ptc(ch,"{C��������� ���������   :{Y%3d %3d %3d %3d %3d\n\r",race_table[race].stats[0],
        race_table[race].stats[1],race_table[race].stats[2],
        race_table[race].stats[3],race_table[race].stats[4]);
      ptc(ch,"{C����. ���� ���������  :{R%3d %3d %3d %3d %3d\n\r",race_table[race].max_stats[0],
        race_table[race].max_stats[1],race_table[race].max_stats[2],
        race_table[race].max_stats[3],race_table[race].max_stats[4]);
      ptc(ch,"{C������������ ���������:{R%3d %3d %3d %3d %3d\n\r",race_table[race].high_stats[0],
        race_table[race].high_stats[1],race_table[race].high_stats[2],
        race_table[race].high_stats[3],race_table[race].high_stats[4]);
      stc("������������ ��������� ������� ��� ����������. � �������� ��� ������. (+2 � primary class stat)\n\r",ch);
      ptc(ch,"{CClass Mult            :{MMag:[{Y%3d{M] Cle:[{Y%3d{M] Thi:[{Y%3d{M] War:[{Y%3d{M]{x\n\r",race_table[race].class_mult[0],
        race_table[race].class_mult[1],race_table[race].class_mult[2],
        race_table[race].class_mult[3]);
    stc("{C=-------------------------------------------------------------------=\n\r",ch);
    ptc(ch,"{C���������� �������:{c %s\n\r",affect_bit_name(race_table[race].aff));
    ptc(ch,"{C����������        :{R %s\n\r",imm_bit_name(race_table[race].vuln));
    ptc(ch,"{C����������������  :{G %s\n\r",imm_bit_name(race_table[race].res));
    ptc(ch,"{C���������         :{W %s\n\r{x",imm_bit_name(race_table[race].imm));
    stc("{C=-------------------------------------------------------------------=\n\r",ch);
    if (race_table[race].c_pen!=0)
    {                                                   
      stc("{R�������� ��� ���������: [",ch);
      if (IS_SET(race_table[race].c_pen,WATER))       stc("Water ",ch);
      if (IS_SET(race_table[race].c_pen,AIR))         stc("Air ",ch);
      if (IS_SET(race_table[race].c_pen,EARTH))       stc("Earth ",ch);
      if (IS_SET(race_table[race].c_pen,FIRE))        stc("Fire ",ch);
      if (IS_SET(race_table[race].c_pen,SPIRIT))      stc("Spirit ",ch);
      if (IS_SET(race_table[race].c_pen,MIND))        stc("Mind ",ch);
      if (IS_SET(race_table[race].c_pen,PROTECT))     stc("Protection ",ch);
      if (IS_SET(race_table[race].c_pen,LIGHT))       stc("Light ",ch);
      if (IS_SET(race_table[race].c_pen,DARK))        stc("Dark ",ch);
      if (IS_SET(race_table[race].c_pen,FORTITUDE))   stc("Fortitude ",ch);
      if (IS_SET(race_table[race].c_pen,CURATIVE))    stc("Curative ",ch);
      if (IS_SET(race_table[race].c_pen,PERCEP))      stc("Perception ",ch);
      if (IS_SET(race_table[race].c_pen,LEARN))       stc("Learning ",ch);
      if (IS_SET(race_table[race].c_pen,MAKE))        stc("MakeMastery",ch);
      stc ("]{x\n\r",ch);
    }
    if (race_table[race].c_bon!=0)
    {
      stc("{G����� ��� ���������: [",ch);
      if (IS_SET(race_table[race].c_bon,WATER))       stc("Water ",ch);       
      if (IS_SET(race_table[race].c_bon,AIR))         stc("Air ",ch);         
      if (IS_SET(race_table[race].c_bon,EARTH))       stc("Earth ",ch);        
      if (IS_SET(race_table[race].c_bon,FIRE))        stc("Fire ",ch);        
      if (IS_SET(race_table[race].c_bon,SPIRIT))      stc("Spirit ",ch);      
      if (IS_SET(race_table[race].c_bon,PROTECT))     stc("Protection ",ch);
      if (IS_SET(race_table[race].c_bon,MIND))        stc("Mind ",ch);        
      if (IS_SET(race_table[race].c_bon,LIGHT))       stc("Light ",ch);       
      if (IS_SET(race_table[race].c_bon,DARK))        stc("Dark ",ch);        
      if (IS_SET(race_table[race].c_bon,FORTITUDE))   stc("Fortitude ",ch);
      if (IS_SET(race_table[race].c_bon,CURATIVE))    stc("Curative ",ch);
      if (IS_SET(race_table[race].c_bon,PERCEP))      stc("Perception ",ch);  
      if (IS_SET(race_table[race].c_bon,LEARN))       stc("Learning ",ch);    
      if (IS_SET(race_table[race].c_bon,MAKE))        stc("MakeMastery",ch); 
      stc("]\n\r{C=-------------------------------------------------------------------=\n\r",ch);
    }
    ptc(ch,"{C�����������    :{Y %s\n\r",spec_bit_name(race_table[race].spec));
    for(spec=0;rspec_table[spec].bit!=0;spec++)
      if (IS_SET(race_table[race].spec, rspec_table[spec].bit))
        ptc(ch, "{G%s{x\n\r", rspec_table[spec].info);
    stc("{C=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n\r",ch);
    return;
  }
  

  if (!str_prefix(arg1,"damage"))
  {
    int temp;

    if (IS_IMMORTAL(ch)) stc("\n\r{C#No. {D[{C  ��{D - {C �� {D] [{xMessage{D]{x\n\r",ch);
    else stc("\n\r{C#No. {D[{xmessage{D]{x\n\r",ch);
    for (temp=0;;temp++)
    {
      if (IS_IMMORTAL(ch))
      ptc(ch,"{C%3d. {D[{C%4d{D - {C%4d{D] [{x%s{D]{x\n\r",temp,dam_msg_table[temp].from,dam_msg_table[temp].to,dam_msg_table[temp].vs);
      else ptc(ch,"{C%3d. {D[{x%s{D]{x\n\r",temp,dam_msg_table[temp].vs);
      if (dam_msg_table[temp].to==-1) break;
    }
    return;
  }

  if( !str_prefix( arg1, "material") )
  {
    int mIndex, cols=0;

    if (EMPTY(arg2) )
    {
      stc("ahelp material list       : list of materials\n\r", ch);
      stc("ahelp material durability : list of durability messages\n\r", ch);
      stc("ahelp material <material> : extra info on material\n\r", ch);
      return;
    }

    if (!str_prefix(arg2,"list"))
    {
      ptc( ch,"{C+------------------------------------------------------------------+{x\n\r");
      ptc( ch,"{C[{x%24s%s%24s{C]{x\n\r", "", "������� ����������", "");
      ptc( ch,"{C+------------------------------------------------------------------+{x\n\r");
      for( mIndex=0; material_table[mIndex].d_dam != 0 ; mIndex++ )
      {
        ptc( ch,"{C[{Y%15s{C]{x", material_table[mIndex].real_name);
        cols++;
        if( cols == 4 )
        {
          stc("\n\r", ch);
          cols=0;
        }
      }
      ptc( ch,"{C+------------------------------------------------------------------+{x\n\r");
      if( material_table[mIndex].name == "NULL" )  return;
    }

    else if (!str_prefix(arg2,"durability"))
    {
      if( IS_ELDER(ch) )
      {
        ptc( ch, "{cFrom   To {G��������� ���������� � ������� �����������{w:{x\n\r");
        for( mIndex=0; item_durability_table[mIndex].to != -1; mIndex++ )
        {
          ptc( ch,"{m%4d {R%4d{x ",item_durability_table[mIndex].from, (item_durability_table[mIndex].to - 1) );
          ptc( ch, "{C[{Y%s{C]{x\n\r", item_durability_table[mIndex].d_message);
        }
      }
      else 
      {
        ptc( ch, "{G��������� ���������� � ������� �����������.{x\n\r");
        for( mIndex=0; item_durability_table[mIndex].to != -1; mIndex++ )
          ptc( ch, "{C[{Y%s{C]{x\n\r", item_durability_table[mIndex].d_message);
      }
    }

    else if ( !str_cmp(arg2, material_lookup(arg2)) )
    {
      int mn_arg2;

      if( !str_prefix(arg2,"bug") ) 
      {
        bug("Kakogo-to hrena gluchit esli arg2 == bug v ahelp material:(", 0);
        if( !IS_ELDER(ch) ) WAIT_STATE( ch, 10);
        return;
      }

      if( (mn_arg2 = material_num(arg2)) < MATERIAL_NONE )
      {
        stc("{RBUG in reading material table data.{x", ch);
        return;
      }
      else 
      {
        ptc( ch, "{G��������{w: {Y[{R%s{Y][{R%s{Y]{x\n\r", material_table[mn_arg2].real_name, arg2);
        for( mIndex = 0; item_durability_table[mIndex].to != -1; mIndex++)
          if( material_table[mn_arg2].d_dam > item_durability_table[mIndex].from && material_table[mn_arg2].d_dam <= item_durability_table[mIndex].to)
          {
            ptc( ch, "{G������� ��������� ��� ������ ���������{w:{x %s", item_durability_table[mIndex].d_message );
            if( IS_ELDER(ch) ) 
            {
              ptc( ch, " {Y[{R%d{Y]{x.\n\r", material_table[mn_arg2].d_dam );
              ptc( ch, "{G��������� � ��������� ���������� {Y[{m%d{Y]-[{R%d{Y]{x.\n\r", item_durability_table[mIndex].from, item_durability_table[mIndex].to );
            }
            else stc( "{x.\n\r", ch);
            return;
          }
      }
    }

    else if ( !str_cmp(arg2, materialrus_lookup(arg2)) )
    {
      int mnr_arg2;

      if( !str_prefix(arg2,"bug") ) 
      {
        bug("Kakogo-to hrena gluchit esli arg2 == bug v ahelp material:(", 0);
        if( !IS_ELDER(ch) ) WAIT_STATE( ch, 10);
        return;
      }

      if( (mnr_arg2 = materialrus_num(arg2)) < MATERIAL_NONE )
      {
        stc("{RBUG :({x", ch);
        return;
      }
      else 
      {
        ptc( ch, "{G��������{w: {Y[{R%s{Y][{R%s{Y]{x\n\r", arg2, material_table[mnr_arg2].name );
        for( mIndex = 0; item_durability_table[mIndex].to != -1; mIndex++)
          if( material_table[mnr_arg2].d_dam > item_durability_table[mIndex].from && material_table[mnr_arg2].d_dam <= item_durability_table[mIndex].to)
          {
            ptc( ch, "{G������� ��������� ��� ������ ���������{w:{x %s", item_durability_table[mIndex].d_message );
            if( IS_ELDER(ch) ) 
            {
              ptc( ch, " {Y[{R%d{Y]{x.\n\r", material_table[mnr_arg2].d_dam );
              ptc( ch, "{G��������� � ��������� ���������� {Y[{m%d{Y]-[{R%d{Y]{x.\n\r", item_durability_table[mIndex].from, item_durability_table[mIndex].to );
            }
            else stc( "{x.\n\r", ch);
            return;
          }
      }
    }
  else return;
  return;
  }

  if( !str_prefix(arg1,"group") )
  {
    int gn,i;

    if (EMPTY(arg2))
    {
      stc("ahelp group list\n\r",ch);
      stc("ahelp group <group>\n\r",ch);
      return;
    }

    if (!str_prefix(arg2,"list"))
    {
      int col=0;

      for(i=0;i<MAX_GROUP;i++)
      {
        if (group_table[i].name == NULL) break;
        ptc(ch,"%3d %15s  ",i+1,group_table[i].name);
        col++;
        if (col==4)
        {
          col=0;
          stc("\n\r",ch);
        }
      }
      return;
    }
    gn=group_lookup(arg2);
    if (gn == -1)
    {
      stc("����� ������ �� �������.\n\r",ch);
      return ;
    }
    ptc(ch,"������ : [{C%s{x]\n\r",group_table[gn].name);
    if (group_cost(ch,gn) >0) ptc(ch,"{C���������: %d{x\n\r",group_cost(ch,gn));
    ptc(ch,"{C��������� ���: Mag:[%3d]  Cle:[%3d]  Thi:[%3d]  War:[%3d]{x\n\r",
      group_table[gn].rating[0],group_table[gn].rating[1],
      group_table[gn].rating[2],group_table[gn].rating[3]);
    stc("{g� ������ ������ ��������� ������ � ����������:{x\n\r",ch);
    for(i=0;group_table[gn].spells[i]!=NULL;i++)
    {
      ptc(ch,"{c[%2d]{Y %s{x\n\r",i+1,group_table[gn].spells[i]);
    }
    return;
  }

  if (!str_prefix(arg1,"class"))
  {
    if (arg2[0]=='\0')
    {
      stc("Syntax: ahelp class bonus\n\r\n\r",ch);
      stc("Syntax: ahelp class list\n\r\n\r",ch);
      return;
    }
    if (!str_prefix(arg2,"bonus"))
    {
      stc("������ �� ������. (����������� ��� ������������):\n\r",ch);
      stc("{wMage   {x:              +2 Learning   +2 Mind\n\r",ch);
      stc("{wCleric {x:              +2 Spirit     +2 Curative\n\r",ch);
      stc("{wThief  {x:              +3 Perception +1 Offence\n\r",ch);
      stc("{wWarrior{x:              +2 Fortitude  +2 Offence\n\r",ch);
                                     
      stc("{GBattleMage{x [M+W]:     +1 Fortitude  +2 Learning   +1 MakeMastery +1 Offence\n\r",ch);
      stc("{GWizard    {x [M+C]:     +2 Fire,Air,Water,Earth\n\r",ch);
      stc("{GNightBlade{x [M+T]:     +2 Perception +2 Mind       +1 Learn\n\r",ch);
      stc("{GHeretic   {x [C+T]:     +2 Perception +2 Spirit     +1 Fire\n\r",ch);
      stc("{GPaladin   {x [C+W]:     +2 Spirit     +1 Curative   +2 Protection\n\r",ch);
      stc("{GNinja     {x [W+T]:     +1 Fortitude  +2 Perception +2 Offence\n\r",ch);

      stc("{RSpellSword{x [M+W+T]:   +1 Offence    +1 Learning   +1 Perception\n\r",ch);
      stc("{RTemplar   {x [C+W+T]:   +1 Spirit     +1 Perception +1 Protection\n\r",ch);
      stc("{RMonk      {x [M+C+W]:   +1 Protection +1 Curative   +1 Fortitude   +1 Spirit\n\r",ch);
      stc("{RBard      {x [M+C+T]:   +1 Mind       +1 Learning   +1 Perception\n\r",ch);

      stc("Race bonus/penalty: 4\n\r",ch);
      return;
    }
    if (!str_prefix(arg2,"list"))
    {
      stc("������ ��������� �������:\n\r",ch);
      stc("�������� ������:\n\r",ch);
      stc("{GWarrior - ����      (Str)\n\r",ch);
      stc("{GMage    - ��������� (Int)\n\r",ch);
      stc("{GThief   - ��������  (Dex)\n\r",ch);
      stc("{GCleric  - ��������  (Wis)\n\r{x",ch);
      return;
    }
    do_ahelp (ch,"class");
    return;
  }

  if (!str_prefix(arg1,"skill") || !str_prefix(arg1,"spell"))
  {
    int sn;

    if (arg2[0]=='\0')
    {
      stc("Syntax: ahelp <skill> <��� ������ ��� ����������>\n\r",ch);
      stc("        ahelp <skill> list\n\r",ch);
      stc("        ahelp <skill> list <��� ������ ��� �����>\n\r",ch);
      stc("������: ahelp kick\n\r",ch);
      stc("        ahelp 'dispel go\n\r",ch);
      stc("        ahelp list fortitude\n\r",ch);
      stc("        ahelp list fire air\n\r",ch);
      return;
    }

    if ( !str_prefix(arg2,"list"))
    {
      int col=0;
  
      if (EMPTY(argument))
      {
        stc("{C����������: {x\n\r",ch);
        for ( sn = 0; sn < max_skill; sn++ )
        {
          if ( skill_table[sn].name == NULL ) break;
          if ( IS_SET(skill_table[sn].group,SPELL))
          {
            ptc( ch, "%18s ", skill_table[sn].name);
            if ((col++)==3) {stc("\n\r",ch);col=0;}
          }
        }
        stc("\n\r{G������:{x\n\r",ch);
        for ( sn = 0; sn < max_skill; sn++ )
        {
          if ( skill_table[sn].name == NULL ) break;
          if ( IS_SET(skill_table[sn].group,SKILL))
          {
            ptc( ch, "%18s ", skill_table[sn].name);
            if ((col++)==3) {stc("\n\r",ch);col=0;}
          }
        }
        return;
      }
      else
      {
        bool fortitude=FALSE,curative=FALSE;
        bool dark=FALSE,light=FALSE,mind=FALSE,spirit=FALSE,percep=FALSE,make=FALSE,learn=FALSE;
        bool fire=FALSE,water=FALSE,earth=FALSE,air=FALSE;
        bool protect=FALSE;

        if (is_name("fortitude",argument))  fortitude=TRUE;
        if (is_name("curative",argument))   curative=TRUE;
        if (is_name("dark",argument))       dark=TRUE;
        if (is_name("protection",argument)) protect=TRUE;
        if (is_name("mind",argument))       mind=TRUE;
        if (is_name("light",argument))      light=TRUE;
        if (is_name("spirit",argument))     spirit=TRUE;
        if (is_name("earth",argument))      earth=TRUE;
        if (is_name("air",argument))        air=TRUE;
        if (is_name("fire",argument))       fire=TRUE;
        if (is_name("water",argument))      water=TRUE;
        if (is_name("learning",argument))   learn=TRUE;
        if (is_name("perception",argument)) percep=TRUE;
        if (is_name("makemastery",argument))make=TRUE;
        for(sn=0;sn<max_skill;sn++)
        {
          if ( skill_table[sn].name == NULL ) break;
          if ( fortitude && !IS_SET(skill_table[sn].group,FORTITUDE)) continue;
          if ( curative && !IS_SET(skill_table[sn].group,CURATIVE)) continue;
          if ( air && !IS_SET(skill_table[sn].group,AIR)) continue;
          if ( fire && !IS_SET(skill_table[sn].group,FIRE)) continue;
          if ( water && !IS_SET(skill_table[sn].group,WATER)) continue;
          if ( earth && !IS_SET(skill_table[sn].group,EARTH)) continue;
          if ( protect && !IS_SET(skill_table[sn].group,PROTECT)) continue;
          if ( learn && !IS_SET(skill_table[sn].group,LEARN)) continue;
          if ( percep && !IS_SET(skill_table[sn].group,PERCEP)) continue;
          if ( make && !IS_SET(skill_table[sn].group,MAKE)) continue;
          if ( dark && !IS_SET(skill_table[sn].group,DARK)) continue;
          if ( light && !IS_SET(skill_table[sn].group,LIGHT)) continue;
          if ( spirit && !IS_SET(skill_table[sn].group,SPIRIT)) continue;
          if ( mind && !IS_SET(skill_table[sn].group,MIND)) continue;
          ptc (ch,"{C%18s {G[{M%s{G]{x\n\r",skill_table[sn].name,spell_group_name(skill_table[sn].group));
        }
        return;
      }
    }

    sn = skill_lookup(arg2);
    if (sn==-1)
    {
      stc("������ ���������� ��� ������ �� �������.\n\r",ch);
      return;
    }

    if (skill_table[sn].spell_fun==spell_null)
    {
      ptc(ch,"{G�����: {Y%s{x\n\r",skill_table[sn].name);

      if (skill_table[sn].rating[CLASS_MAG]!=0 && skill_table[sn].skill_level[CLASS_MAG]<102)
        ptc(ch,"{G���{x   : ������� {Y%3d{x, ����������� {Y%d{x\n\r",
        skill_table[sn].skill_level[CLASS_MAG],
        skill_table[sn].rating[CLASS_MAG]);
      else stc("{G���{x   : {RN/A{x\n\r",ch);

      if (skill_table[sn].rating[CLASS_CLE]!=0 && skill_table[sn].skill_level[CLASS_CLE]<102)
        ptc(ch,"{G������{x: ������� {Y%3d{x, ����������� {Y%d{x\n\r",
        skill_table[sn].skill_level[CLASS_CLE],
        skill_table[sn].rating[CLASS_CLE]);
      else stc("{G������{x: {RN/A{x\n\r",ch);

      if (skill_table[sn].rating[CLASS_THI]!=0 && skill_table[sn].skill_level[CLASS_THI]<102)
        ptc(ch,"{G���{x   : ������� {Y%3d{x, ����������� {Y%d{x\n\r",
        skill_table[sn].skill_level[CLASS_THI],
        skill_table[sn].rating[CLASS_THI]);
      else stc("{G���{x   : {RN/A{x\n\r",ch);

      if (skill_table[sn].rating[CLASS_WAR]!=0 && skill_table[sn].skill_level[CLASS_WAR]<102)
        ptc(ch,"{G����{x  : ������� {Y%3d{x, ����������� {Y%d{x\n\r",
        skill_table[sn].skill_level[CLASS_WAR],
        skill_table[sn].rating[CLASS_WAR]);
      else stc("{G����{x  : {RN/A{x\n\r",ch);
    }
    else
    {
      ptc(ch,"{C����������: {Y%s{x\n\r",skill_table[sn].name);

      if (skill_table[sn].rating[CLASS_MAG]!=0 && skill_table[sn].skill_level[CLASS_MAG]<102)
        ptc(ch,"{G���{x   : ������� {Y%3d{x, ����������� {Y%d{x\n\r",
        skill_table[sn].skill_level[CLASS_MAG],
        skill_table[sn].rating[CLASS_MAG]);
      else stc("{G���{x   : {RN/A{x\n\r",ch);

      if (skill_table[sn].rating[CLASS_CLE]!=0 && skill_table[sn].skill_level[CLASS_CLE]<102)
        ptc(ch,"{G������{x: ������� {Y%3d{x, ����������� {Y%d{x\n\r",
        skill_table[sn].skill_level[CLASS_CLE],
        skill_table[sn].rating[CLASS_CLE]);
      else stc("{G������{x: {RN/A{x\n\r",ch);

      if (skill_table[sn].rating[CLASS_THI]!=0 && skill_table[sn].skill_level[CLASS_THI]<102)
        ptc(ch,"{G���{x   : ������� {Y%3d{x, ����������� {Y%d{x\n\r",
        skill_table[sn].skill_level[CLASS_THI],
        skill_table[sn].rating[CLASS_THI]);
      else stc("{G���{x   : {RN/A{x\n\r",ch);

      if (skill_table[sn].rating[CLASS_WAR]!=0 && skill_table[sn].skill_level[CLASS_WAR]<102)
        ptc(ch,"{G����{x  : ������� {Y%3d{x, ����������� {Y%d{x\n\r",
        skill_table[sn].skill_level[CLASS_WAR],
        skill_table[sn].rating[CLASS_WAR]);
      else stc("{G����{x  : {RN/A{x\n\r",ch);

      ptc(ch,"{Y����������� ���������� ����: {C%d{x\n\r",skill_table[sn].min_mana);
      if (IS_SET(skill_table[sn].flag,C_NODUAL)) stc("{R�� ����� ������������� second weapon ������������ ������.{x\n\r",ch);
    }
      ptc(ch,"{Y�������������� � �������: [{M%s{Y]{x\n\r",spell_group_name(skill_table[sn].group));
      ptc(ch,"%s{x\n\r",skill_table[sn].help);
  }
  
  if (!str_prefix(arg1,"stat"))
  {
    int i,j;
    bool is_found=FALSE;
    if (EMPTY(arg2))
    {
      ptc(ch,"Syntax: ahelp stat <race>\n\r");
      return;
    }
    i=race_lookup(arg2);
    if (i==0)
    {
      ptc(ch,"����� ���� �� ����������.\n\r");
      return;
    }      
    ptc(ch,"{G���������������� � {R���������� {x��� {W%s{x:\n\r",race_table[i].name);
    for ( j = 0;j < DAM_MAX;j++)
    {
      if ( race_table[i].dambonus[j] < 0)
      {
        ptc(ch,"{C%s{x: {G%d%%\n\r{x",bonus_dam_types[j],race_table[i].dambonus[j]);
        is_found=TRUE;
      }
    }
    for ( j = 0;j < DAM_MAX;j++)
    {
      if ( race_table[i].dambonus[j] > 0 )
      {
        ptc(ch,"{C%s{x: {R+%d%%\n\r{x",bonus_dam_types[j],race_table[i].dambonus[j]);
        is_found=TRUE;
      }
    }
    if (!is_found) stc("���.\n\r",ch);
    is_found=FALSE;
    stc("{wWeapon bonuses{x:\n\r",ch);
    for ( j = 0; j < WEAPON_MAX; j++)
    {
      if ( race_table[i].weapon_bonus[j] > 0 )
      {
        ptc(ch,"{c%s{x: {G+%d%%\n\r",weapon_bonus_types[j],race_table[i].weapon_bonus[j]);
        is_found=TRUE;
      }
    }
    for ( j = 0; j < WEAPON_MAX; j++)
    {
      if ( race_table[i].weapon_bonus[j] < 0 )
      {
        ptc(ch,"{c%s{W: {R%d%%\n\r",weapon_bonus_types[j],race_table[i].weapon_bonus[j]);
        is_found=TRUE;
      }
    }
    if (!is_found) stc("���.\n\r",ch);
  }
  
}

// ��������� ������ �� f2 � f1
bool is_set(int64 f1,int64 f2)
{
 int f1h,f2h,f1l,f2l;
 if (!f1 || !f2) return FALSE;
 if (f1 < f2) return FALSE;
 f1h = (int)((f1 >> 32) & 0xFFFFFFFF);
 f2h = (int)((f2 >> 32) & 0xFFFFFFFF);
 f1l = (int)f1;
 f2l = (int)f2;
 return ((f1l & f2l) == f2l) && ((f1h & f2h) == f2h);
}

void set_bit(int64 flag,int64 bit)
{
 int flagh,bith,flagl,bitl;
 if (!flag || !bit) return;
 flagh = (int)((flag >> 32) & 0xFFFFFFFF);
 bith = (int)((bit >> 32) & 0xFFFFFFFF);
 flagl = (int)flag;
 bitl = (int)bit;
 if (!flagh && !bith)
 {
  flagl |= bitl;
  flag = flagl;
 }
 else
 {
  flagh |= bith;
  flagl |= bitl;
  flag = ((int64)flagh << 32) + flagl;
 }
}

void rem_bit(int64 flag,int64 bit)
{
 int flagh,bith,flagl,bitl;
 if (!flag || !bit) return;
 flagh = (int)((flag >> 32) & 0xFFFFFFFF);
 bith = (int)((bit >> 32) & 0xFFFFFFFF);
 flagl = (int)flag;
 bitl = (int)bit;
 if (!flagh && !bith)
 {
  flagl &= ~(bitl);
  flag = flagl;
 }
 else 
 {
  flagh &= ~(bith);
  flagl &= ~(bitl);
  flag = ((int64)flagh << 32) + flagl;
 }
}

// does aliasing and other fun stuff
void substitute_alias(DESCRIPTOR_DATA *d, const char *argument)
{
  CHAR_DATA *ch;
  char buf[MAX_STRING_LENGTH],prefix[MAX_STRING_LENGTH],name[MAX_STRING_LENGTH];
  const char *point;
  int alias;

  ch = d->character;

  // check for prefix
  if (ch->prefix[0] != '\0' && str_prefix("prefix",argument))
  {
    if (strlen(ch->prefix) + strlen(argument) > MAX_INPUT_LENGTH)
      stc("������ ������� �������.\r\n",ch);
    else
    {
      do_printf(prefix,"%s %s",ch->prefix,argument);
      argument = prefix;
    }
  }

  if (IS_NPC(ch) || ch->pcdata->alias[0] == NULL
    ||  !str_prefix(argument,"alias") || !str_prefix(argument,"unalias")
    ||  !str_prefix(argument,"prefix"))
  {
    interpret(d->character,argument);
    return;
  }

  strcpy(buf,argument);

  for (alias = 0; alias < MAX_ALIAS; alias++)
  {
    if (ch->pcdata->alias[alias] == NULL) break;

    if (!str_prefix(ch->pcdata->alias[alias],argument))
    {
      point = one_argument(argument,name);
      if (!strcmp(ch->pcdata->alias[alias],name))
      {
        if (( strlen(ch->pcdata->alias_sub[alias])+strlen(point) ) > MAX_INPUT_LENGTH - 6)
        {
          stc("������ ������� �������. ��������.\r\n",ch);
          buf[MAX_INPUT_LENGTH -1] = '\0';
        }
        else
        {
         buf[0] = '\0';
         strcat(buf,ch->pcdata->alias_sub[alias]);
         strcat(buf," ");
         strcat(buf,point);
        }
        break;
      }
    }
  }
  interpret(d->character,buf);
}

void do_alias(CHAR_DATA *ch, const char *argument)
{
  CHAR_DATA *rch;
  char arg[MAX_STRING_LENGTH];
  int pos;

  if (ch->desc == NULL) rch = ch;
  else rch = ch;

  argument = one_argument(argument,arg);

  if (arg[0] == '\0')
  {
    if (rch->pcdata->alias[0] == NULL)
    {
      stc("� ���� ��� �� ����� ����������������� �������.\n\r",ch);
      return;
    }
    stc("���� ����������������� �������:\n\r",ch);

    for (pos = 0; pos < MAX_ALIAS; pos++)
    {
      if (rch->pcdata->alias[pos] == NULL
        ||  rch->pcdata->alias_sub[pos] == NULL) break;

      ptc(ch,"    %s:  %s\n\r",rch->pcdata->alias[pos],rch->pcdata->alias_sub[pos]);
    }
    return;
  }

  if (!str_prefix("una",arg) || !str_cmp("alias",arg))
  {
    stc("��� ����� ���������������.\n\r",ch);
    return;
  }

  if (argument[0] == '\0')
  {
    for (pos = 0; pos < MAX_ALIAS; pos++)
    {
      if (rch->pcdata->alias[pos] == NULL
        ||  rch->pcdata->alias_sub[pos] == NULL) break;

      if (!str_cmp(arg,rch->pcdata->alias[pos]))
      {
        ptc(ch, "%s ������ �������� '%s'.\n\r",rch->pcdata->alias[pos],
                        rch->pcdata->alias_sub[pos]);
        return;
      }
    }
    stc("����������������� ����� �� ����������.\n\r",ch);
    return;
  }

  if (!str_prefix(argument,"delete") || !str_prefix(argument,"prefix"))
  {
    stc("��� �� ����� �������.\n\r",ch);
    return;
  }

  for (pos = 0; pos < MAX_ALIAS; pos++)
  {
    if (rch->pcdata->alias[pos] == NULL) break;

    if (!str_cmp(arg,rch->pcdata->alias[pos])) /* redefine an alias */
    {
      free_string(rch->pcdata->alias_sub[pos]);
      rch->pcdata->alias_sub[pos] = str_dup(argument);
      ptc(ch, "%s ������������� �� '%s'.\n\r",arg,argument);
      return;
    }
  }

  if (pos >= MAX_ALIAS)
  {
    stc("� ���� ��� ���������� ����������������� ������.\n\r",ch);
    return;
  }
  rch->pcdata->alias[pos]            = str_dup(arg);
  rch->pcdata->alias_sub[pos]        = str_dup(argument);
  ptc(ch, "%s ������ �������� '%s'.\n\r",arg,argument);
}

void do_unalias(CHAR_DATA *ch, const char *argument)
{
  CHAR_DATA *rch;
  char arg[MAX_STRING_LENGTH];
  int pos;
  bool found = FALSE;

  if (ch->desc == NULL) rch = ch;
  else  rch = ch;

  argument = one_argument(argument,arg);

  if (arg[0] == '\0')
  {
    stc("������ ����� �������?\n\r",ch);
    return;
  }

  for (pos = 0; pos < MAX_ALIAS; pos++)
  {
    if (rch->pcdata->alias[pos] == NULL) break;

    if (found)
    {
      rch->pcdata->alias[pos-1]           = rch->pcdata->alias[pos];
      rch->pcdata->alias_sub[pos-1]       = rch->pcdata->alias_sub[pos];
      rch->pcdata->alias[pos]             = NULL;
      rch->pcdata->alias_sub[pos]         = NULL;
      continue;
    }

    if(!strcmp(arg,rch->pcdata->alias[pos]))
    {
      stc("����������������� ������� ������.\n\r",ch);
      free_string(rch->pcdata->alias[pos]);
      free_string(rch->pcdata->alias_sub[pos]);
      rch->pcdata->alias[pos] = NULL;
      rch->pcdata->alias_sub[pos] = NULL;
      found = TRUE;
    }
  }
  if (!found) stc("��� ����������������� ������� ����������.\n\r",ch);
}

void do_dampool( CHAR_DATA *ch, const char *argument )
{
 int count;
 CHAR_DATA *victim=NULL;

 victim=get_pchar_world(ch,argument);
 if (!victim) victim=ch;

 ptc(ch,"Dampool for {Y%s{x:\n\r",victim->name);
 for (count=0;count<MAX_VICT;count++)
  ptc(ch,"Name: %s Dampool: %d\n\r",
  !victim->pcdata->victims[count].victim ? "none":get_char_desc(victim->pcdata->victims[count].victim,'1'),
   victim->pcdata->victims[count].dampool);
}

// Pick off one argument from a string and return the rest. Understands quotes.
char *one_arg( char *argument, char *arg_first )
{
 char cEnd;

 while ( isspace(*argument) ) argument++;

 cEnd = ' ';
 if ( *argument == '\'' || *argument == '"' ) cEnd = *argument++;

 while ( *argument != '\0' )
 {
   if ( *argument == cEnd )
   {
    argument++;
    break;
   }
   *arg_first =*argument;
   arg_first++;
   argument++;
 }
 *arg_first = '\0';
 while ( isspace(*argument) ) argument++;
 return argument;
}

void do_tournament( CHAR_DATA *ch, const char *argument )
{
  char arg[256];
  char chn1[256],chn2[256];
  CHAR_DATA *ch1,*ch2;
  TOURNAMENT_DATA *t=tournament;
  BET_DATA *b,*b1;
  char str[256];
  int i,k;

  if (EMPTY(argument))
  {
    stc("tournament bet <���> <qp> - ������� ������\n\r",ch);
    stc("           info           - ������� ����������\n\r",ch);
    if (get_trust(ch) > 101)
    {
      stc("           propose <char1> <char2> <prize> - �������� ������\n\r",ch);
      stc("           startbet - ��������� ������ ������\n\r",ch);
      stc("           fight    - ������ ���\n\r",ch);
      stc("           cancel   - �������� ������\n\r",ch);
      stc("           complete <���> - ��������� �����, ��������� ������\n\r",ch);
    }
    return;
  }
    
  argument=one_argument(argument,arg);

  if (!str_prefix(arg,"info"))
  {
    if (!t)
    {
      stc("No active tournament found.\n\r",ch);
      return;
    }
       
    if ( get_trust(ch)< 102 )
    {
      ptc(ch,"{DTournament: {C%s {rvs. {C%s{x\n\r",t->ch1->name,t->ch2->name);
      return;
    }
    ptc(ch,"{DTournament: {C%s {rvs. {C%s{x, prize {Y%d qp\n\n\r{RBets:{x\n\r",t->ch1->name,t->ch2->name,t->prize);
    for ( b1=t->bet; b1; b1=b1->next )
      ptc(ch,"{G%s {x�������� {Y%d qp{x �� {C%s\n\r",b1->chname,b1->prize,b1->victimname);
    return;
  }

  if (!str_prefix(arg,"bet"))
  {
    int r;
        
    if (!t) {stc("��� �������� ��������.\n\r",ch); return;}
        
    if ( !t->isbet )
    {
      stc("�������� ����� ��������.\n\r",ch);
      return;
    }
       
    if ( ch->level < 5 && !ch->remort )
    {
      stc("������� �� 5 ������.\n\r",ch);
      return;
    }
       
    argument=one_argument(argument,chn1);
    if ( !(ch1=get_pchar_world(ch,chn1)) )
    {
      stc("��� ������ ������.\n\r",ch);
      return;
    }
       
    if ( !ch1->pcdata->tournament )
    {
      stc("�� �� �������� �������.\n\r",ch);
      return;
    }
       
    if (!argument || !argument[0] || !is_number(argument))
    {
      stc("������� �� ������ ���������?",ch);
      return;
    }
       
    r=atoi(argument);
    if ( ch->questpoints < 10 )
    {
      ptc(ch,"You got much-much qp, right?\n\r");
      return;
    }
       
    if ( !r || r > t->prize || r > ch->questpoints || r < 10 )
    {
      ptc(ch,"�� ������ ������ ������ �� 10 �� %d qp.\n\r",UMIN(t->prize,ch->questpoints));
      return;
    }
       
    if ( ch == ch1 || ch == t->ch2 )
    {
      stc("�������� ����� ��������. � ������ ��� ����!\n\r",ch);
      return;
    }
       
    for ( b1=t->bet; b1; b1=b1->next )
      if ( !strcmp(b1->chname,ch->name) )
      {
        stc("�� ��� ������ ������.",ch);
        return;
      }
       
/*
    if (!strcmp(ch->name,"Belka"))
    {
      ptc(ch,"{R{+WARNING{-{x{w: one {MBelka{c found in do_tournament()\n\r");
      stc("�� ��������� ���������!\n\r",ch);
      //return; :)))))
    }
*/
    b=malloc(sizeof(BET_DATA));
    if (!b) return;
    b->next=t->bet;
    t->bet=b;
    strcpy(b->chname,ch->name);
    strcpy(b->victimname,ch1->name);
    b->prize=r;
    ch->questpoints-=r;
    t->bank+=r;
       
    ch->pcdata->bet=b;
       
    ptc(ch,"{w�����, {W%s. {w�� �������� {Y%d qp {x�� {C%s{x",b->chname,b->prize,b->victimname);
    return;
  }

  if (!str_prefix(arg,"propose"))
  {
    if ( get_trust(ch)<102) return;

    if (t)
    {
      stc("��� ���� ���� ������.\n\r",ch);
      return;
    }
        
    argument=one_argument(argument,chn1);
    argument=one_argument(argument,chn2);

    if (EMPTY(chn1) || EMPTY(chn2))
    {
      stc("������� ����� �������.\n\r",ch);
      return;
    }

    ch1=get_pchar_world(ch,chn1);
    ch2=get_pchar_world(ch,chn2);

    if (!ch1 || !ch2)
    {
      stc("��������� ������ �� �������.\n\r",ch);
      return;
    }
        
    if (EMPTY(argument) || !is_number(argument))
    {
      stc("�� �������� ���� �������.\n\r",ch);
      return;
    }
        
    if (ch1==ch2)
    {
      stc("Fatal error 0xFFA7. Call to Borland.\n\r",ch);
      return;
    }
        
    if ( ch1->pcdata->tournament || ch2->pcdata->tournament )
    {
      stc("���� �� ������� ��� ��������� ������� � �������.\n\r",ch);
      return;
    }

    if ( ch1->level < 5 || ch2->level < 5 )
    {
      stc("������ ������ ���������� 5 ������, ����� ������� ������� � �������.\n\r",ch);
      return;
    }
        
    if ( atoi(argument) < 10 )
    {
      ptc(ch,"� ���� ���� ����� ���� ?\n\r");
      return;
    }
        
    t=malloc(sizeof(TOURNAMENT_DATA));
    if (!t)
    {
      stc("Fatal error 0x00A7. Call to Borland.\n\r",ch);
      return;
    }
        
    t->next=tournament;
    tournament=t;
    t->bet=0;
    t->ch1=ch1;
    t->ch2=ch2;
    t->prize=URANGE(0,atoi(argument),1000);
    t->bank=0;
    t->vnum=1;
    t->isbet=0;
    t->isfight=0;
        
    ch1->pcdata->tournament=t;
    ch2->pcdata->tournament=t;
        
    do_printf(str,"\n\r{D[Tournament]{w: {x������������ ������: {C%s {rvs. {C%s.{x ����: {Y%d qp{x\n\r",ch1->name,ch2->name,t->prize);
    gecho(str);
    return;
  }

  if (!str_prefix(arg,"startbet"))
  {
    if ( get_trust(ch)<102) return;
    if (!t) { stc("��� �������� ��������.\n\r",ch); return; }
    t->isbet=1;
    do_printf(str,"\n\r{D[Tournament]{w: {W������� ���� ������, �������! {C%s {rvs. {C%s.{x ����: {Y%d qp{x\n\r",t->ch1->name,t->ch2->name,t->prize);
    gecho(str);
    return;
  }

  if (!str_prefix(arg,"fight"))
  {
    if ( get_trust(ch)< 102 ) return;
    t->isbet=0;
    t->isfight=1;
    do_printf(str,"\n\r{D[Tournament]{w: {Y������ ������ �� �����������. {wLet the {RFIGHT {wbegin!!!{x\n\r");
    gecho(str);
    return;
  }

  if (!str_prefix(arg,"cancel"))
  {
    if ( get_trust(ch)< 105 ) return;
       
    if (!t) 
    {
      stc("No tournaments active now.\n\r",ch);
      return;
    }
       
    t->ch1->pcdata->tournament=0;
    t->ch2->pcdata->tournament=0;
    do_printf(str,"\n\r{D[Tournament]{w: {C������ {R�������.{x\n\r");
    gecho(str);
       
    for ( b1=t->bet; b1; b1=b1->next )
    {
      ch1=get_pchar_world(ch,b1->chname);
      if (!ch1) continue;
           
      ch1->pcdata->bet=0;
      ch1->questpoints+=b1->prize;
      stc("�� ������� ����� ���� ������.\n\r",ch1);
    }
    tournament=0;
    return;
  }

  if (!str_prefix(arg,"complete"))
  {
    if ( get_trust(ch)< 105 ) return;
       
    if (!t) 
    {
      stc("No tournaments active now.\n\r",ch);
      return;
    }
       
    argument=one_argument(argument,chn1);
    ch1=get_pchar_world(ch,chn1);
       
    if (!ch1)
    {
      stc("��� ������ ������.\n\r",ch);
      return;
    }
       
    if ( !ch1->pcdata->tournament )
    {       
      stc("�� �� �������� �������.",ch);
      return;
    }
       
    ch1->questpoints+=t->prize;
    ptc(ch1,"{W�����������, {C%s,{W �� ������� ������.{c �� ��������� {Y%d qp{x\n\r",ch1->name,t->prize);
       
    t->ch1->pcdata->tournament=0;
    t->ch2->pcdata->tournament=0;
    do_printf(str,"\n\r{D[Tournament]{w: {C%s {xWINS. {RFatality.{x\n\r",ch1->name);
    gecho(str);
       
    i=0; //total winners' bets
    for ( b1=t->bet; b1; b1=b1->next )
    {
      ch2=get_pchar_world(ch,b1->chname);
      if (!ch2) continue;
      if (!strcmp(b1->victimname,ch1->name)) i+=b1->prize;
    }
       
    for ( b1=t->bet; b1; b1=b1->next )
    {
      ch2=get_pchar_world(ch,b1->chname);
      if (!ch2) continue;
      ch2->pcdata->bet=0;
           
      if (!strcmp(b1->victimname,ch1->name))
      {
        k=b1->prize*t->bank/i;
        ptc(ch2,"{W�� ���� ��! {c�� ������� {Y%d qp{x!\n\r",k-(b1->prize));
        ch2->questpoints+=k;
      }
    }
    tournament=0;
    return;
  }
  do_tournament(ch,"");
}

void do_test( CHAR_DATA *ch, const char *argument )
{
  act("[$c1] [$c2] [$c3]",ch,NULL,NULL,TO_CHAR);
}
