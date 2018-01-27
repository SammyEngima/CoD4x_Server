/*
===========================================================================
    Copyright (C) 2010-2013  Ninja and TheKelm

    This file is part of CoD4X18-Server source code.

    CoD4X18-Server source code is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    CoD4X18-Server source code is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>
===========================================================================
*/

#include "q_shared.h"
#include "scr_vm.h"
#include "scr_vm_functions.h"
#include "qcommon_io.h"
#include "cvar.h"
#include "cmd.h"
#include "misc.h"
#include "sys_main.h"
#include "sv_bots.h"
//#include "scr_vm_classfunc.h"
#include "stringed_interface.h"

#include <stdarg.h>
#include <ctype.h>

typedef struct
{
    char *name;
    xfunction_t offset;
    qboolean developer;
} v_function_t;

char *var_typename[] =
    {
        "undefined",
        "object",
        "string",
        "localized string",
        "vector",
        "float",
        "int",
        "codepos",
        "precodepos",
        "function",
        "stack",
        "animation",
        "developer codepos",
        "include codepos",
        "thread",
        "thread",
        "thread",
        "thread",
        "struct",
        "removed entity",
        "entity",
        "array",
        "removed thread"};

// Original: 0x08215780
// Last element must be zeroed.
// Warning: max 64 elements in array. Hardcoded into VM structures.
// How to understand?
//   name is the name of script field.
//   offset is the address in gclient_t structure.
//   type is the type of variable.
//   setter is the function pointer to be called when trying to set variable (self.statusicon = "something")
//   getter is the function pointer to be called when trying to get variable (iprintln(self.statusicon))
// Some notes: (based on Scr_GetClientField, 0x080C89D8)
//   - if offset is 0 then setter/getter should be set. Offset 0 still can be used (rarely).
//     getter/setter will be called with passed client_field_t value.
//   - if getter/setter is 0 then field will be get from/set to specified offset and type from client_t structure.
/*
client_fields_t client_fields[] = {
    {"name", 0, F_LSTRING, ClientScr_ReadOnly, ClientScr_GetName},
    {"sessionteam", 0, F_STRING, ClientScr_SetSessionTeam, ClientScr_GetSessionTeam},
    {"sessionstate", 0, F_STRING, ClientScr_SetSessionState, ClientScr_GetSessionState},
    {"maxhealth", 0x2FE8, F_INT, ClientScr_SetMaxHealth, 0},
    {"score", 0x2F78, F_INT, ClientScr_SetScore, 0},
    {"deaths", 0x2F7C, F_INT, 0, 0},
    {"statusicon", 0, F_STRING, ClientScr_SetStatusIcon, ClientScr_GetStatusIcon},
    {"headicon", 0, F_STRING, ClientScr_SetHeadIcon, ClientScr_GetHeadIcon},
    {"headiconteam", 0, F_STRING, ClientScr_SetHeadIconTeam, ClientScr_GetHeadIconTeam},
    {"kills", 0x2F80, F_INT, 0, 0},
    {"assists", 0x2F84, F_INT, 0, 0},
    {"hasradar", 0x3178, F_INT, 0, 0},
    {"spectatorclient", 0x2F68, F_INT, ClientScr_SetSpectatorClient, ClientScr_GetSpectatorClient},
    {"killcamentity", 0x2F6C, F_INT, ClientScr_SetKillCamEntity, 0},
    {"archivetime", 0x2F74, F_FLOAT, ClientScr_SetArchiveTime, ClientScr_GetArchiveTime},
    {"psoffsettime", 0x3070, F_INT, ClientScr_SetPSOffsetTime, ClientScr_GetPSOffsetTime},
    {"pers", 0x2F88, F_MODEL, ClientScr_ReadOnly, 0},
    {0, 0, 0, 0, 0}
};
*/

// Original: 0x082202A0
// This array used in patch inside cod4loader routines.
// If you have decompiled one of mentioned there functions, make sure
//   to remove patch.
// Appears to have max 16384 elements.

/*
ent_field_t ent_fields[] = {
    {"classname", 0x170, F_STRING, Scr_ReadOnlyField},
    {"origin", 0x13C, F_VECTOR, Scr_SetOrigin},
    {"model", 0x168, F_MODEL, Scr_ReadOnlyField},
    {"spawnflags", 0x17C, F_INT, Scr_ReadOnlyField},
    {"target", 0x172, F_STRING, 0},
    {"targetname", 0x174, F_STRING, 0},
    {"count", 0x1AC, F_INT, 0},
    {"health", 0x1A0, F_INT, Scr_SetHealth},
    {"dmg", 0x1A8, F_INT, 0},
    {"angles", 0x148, F_VECTOR, Scr_SetAngles},
    {0, 0, 0, 0}
};
*/

stringIndex_t scr_const;
/*
ent_field_t* __internalGet_ent_fields()
{
    return ent_fields;
}
*/
void Scr_AddStockFunctions()
{
	Scr_AddFunction("createprintchannel", GScr_CreatePrintChannel, 1 );
	Scr_AddFunction("setprintchannel", GScr_printChannelSet, 1 );
	Scr_AddFunction("print", print, 1 );
	Scr_AddFunction("println", println, 1 );
	Scr_AddFunction("iprintln", iprintln, 0 );
	Scr_AddFunction("iprintlnbold", iprintlnbold, 0 );
	Scr_AddFunction("print3d", GScr_print3d, 1 );
	Scr_AddFunction("line", GScr_line, 1 );
	Scr_AddFunction("getent", Scr_GetEnt, 0 );
	Scr_AddFunction("getentarray", Scr_GetEntArray, 0 );
	Scr_AddFunction("spawnplane", GScr_SpawnPlane, 0 );
	Scr_AddFunction("spawnturret", GScr_SpawnTurret, 0 );
	Scr_AddFunction("precacheturret", GScr_PrecacheTurret, 0 );
	Scr_AddFunction("spawnstruct", Scr_AddStruct, 0 );
	Scr_AddFunction("assert", assertCmd, 1 );
	Scr_AddFunction("assertex", assertexCmd, 1 );
	Scr_AddFunction("assertmsg", assertmsgCmd, 1 );
	Scr_AddFunction("isdefined", GScr_IsDefined, 0 );
	Scr_AddFunction("isstring", GScr_IsString, 0 );
	Scr_AddFunction("isalive", GScr_IsAlive, 0 );
	Scr_AddFunction("gettime", GScr_GetTime, 0 );
	Scr_AddFunction("getentbynum", Scr_GetEntByNum, 1 );
	Scr_AddFunction("getweaponmodel", Scr_GetWeaponModel, 0 );
	Scr_AddFunction("getanimlength", GScr_GetAnimLength, 0 );
	Scr_AddFunction("animhasnotetrack", GScr_AnimHasNotetrack, 0 );
	Scr_AddFunction("getnotetracktimes", GScr_GetNotetrackTimes, 0 );
	Scr_AddFunction("getbrushmodelcenter", GScr_GetBrushModelCenter, 0 );
	Scr_AddFunction("objective_add", Scr_Objective_Add, 0 );
	Scr_AddFunction("objective_delete", Scr_Objective_Delete, 0 );
	Scr_AddFunction("objective_state", Scr_Objective_State, 0 );
	Scr_AddFunction("objective_icon", Scr_Objective_Icon, 0 );
	Scr_AddFunction("objective_position", Scr_Objective_Position, 0 );
	Scr_AddFunction("objective_onentity", Scr_Objective_OnEntity, 0 );
	Scr_AddFunction("objective_current", Scr_Objective_Current, 0 );
	Scr_AddFunction("missile_createattractorent", Scr_MissileCreateAttractorEnt, 0 );
	Scr_AddFunction("missile_createattractororigin", Scr_MissileCreateAttractorOrigin, 0 );
	Scr_AddFunction("missile_createrepulsorent", Scr_MissileCreateRepulsorEnt, 0 );
	Scr_AddFunction("missile_createrepulsororigin", Scr_MissileCreateRepulsorOrigin, 0 );
	Scr_AddFunction("missile_deleteattractor", Scr_MissileDeleteAttractor, 0 );
	Scr_AddFunction("bullettrace", Scr_BulletTrace, 0 );
	Scr_AddFunction("bullettracepassed", Scr_BulletTracePassed, 0 );
	Scr_AddFunction("sighttracepassed", Scr_SightTracePassed, 0 );
	Scr_AddFunction("physicstrace", Scr_PhysicsTrace, 0 );
	Scr_AddFunction("playerphysicstrace", Scr_PlayerPhysicsTrace, 0 );
	Scr_AddFunction("getmovedelta", GScr_GetMoveDelta, 0 );
	Scr_AddFunction("getangledelta", GScr_GetAngleDelta, 0 );
	Scr_AddFunction("getnorthyaw", GScr_GetNorthYaw, 0 );
	Scr_AddFunction("randomint", Scr_RandomInt, 0 );
	Scr_AddFunction("randomfloat", Scr_RandomFloat, 0 );
	Scr_AddFunction("randomintrange", Scr_RandomIntRange, 0 );
	Scr_AddFunction("randomfloatrange", Scr_RandomFloatRange, 0 );
	Scr_AddFunction("sin", GScr_sin, 0 );
	Scr_AddFunction("cos", GScr_cos, 0 );
	Scr_AddFunction("tan", GScr_tan, 0 );
	Scr_AddFunction("asin", GScr_asin, 0 );
	Scr_AddFunction("acos", GScr_acos, 0 );
	Scr_AddFunction("atan", GScr_atan, 0 );
	Scr_AddFunction("int", GScr_CastInt, 0 );
	Scr_AddFunction("abs", GScr_abs, 0 );
	Scr_AddFunction("min", GScr_min, 0 );
	Scr_AddFunction("max", GScr_max, 0 );
	Scr_AddFunction("floor", GScr_floor, 0 );
	Scr_AddFunction("ceil", GScr_ceil, 0 );
	Scr_AddFunction("sqrt", GScr_sqrt, 0 );
	Scr_AddFunction("vectorfromlinetopoint", GScr_VectorFromLineToPoint, 0 );
	Scr_AddFunction("pointonsegmentnearesttopoint", GScr_PointOnSegmentNearestToPoint, 0 );
	Scr_AddFunction("distance", Scr_Distance, 0 );
	Scr_AddFunction("distance2d", Scr_Distance2D, 0 );
	Scr_AddFunction("distancesquared", Scr_DistanceSquared, 0 );
	Scr_AddFunction("length", Scr_Length, 0 );
	Scr_AddFunction("lengthsquared", Scr_LengthSquared, 0 );
	Scr_AddFunction("closer", Scr_Closer, 0 );
	Scr_AddFunction("vectordot", Scr_VectorDot, 0 );
	Scr_AddFunction("vectornormalize", Scr_VectorNormalize, 0 );
	Scr_AddFunction("vectortoangles", Scr_VectorToAngles, 0 );
	Scr_AddFunction("vectorlerp", Scr_VectorLerp, 0 );
	Scr_AddFunction("anglestoup", Scr_AnglesToUp, 0 );
	Scr_AddFunction("anglestoright", Scr_AnglesToRight, 0 );
	Scr_AddFunction("anglestoforward", Scr_AnglesToForward, 0 );
	Scr_AddFunction("combineangles", Scr_CombineAngles, 0 );
	Scr_AddFunction("issubstr", Scr_IsSubStr, 0 );
	Scr_AddFunction("getsubstr", Scr_GetSubStr, 0 );
	Scr_AddFunction("tolower", Scr_ToLower, 0 );
	Scr_AddFunction("strtok", Scr_StrTok, 0 );
	Scr_AddFunction("musicplay", Scr_MusicPlay, 0 );
	Scr_AddFunction("musicstop", Scr_MusicStop, 0 );
	Scr_AddFunction("soundfade", Scr_SoundFade, 0 );
	Scr_AddFunction("ambientplay", Scr_AmbientPlay, 0 );
	Scr_AddFunction("ambientstop", Scr_AmbientStop, 0 );
	Scr_AddFunction("precachemodel", Scr_PrecacheModel, 0 );
	Scr_AddFunction("precacheshellshock", Scr_PrecacheShellShock, 0 );
	Scr_AddFunction("precacheitem", Scr_PrecacheItem, 0 );
	Scr_AddFunction("precacheshader", Scr_PrecacheShader, 0 );
	//Scr_AddFunction("precachestring", Scr_PrecacheString, 0 );
	Scr_AddFunction("precacherumble", Scr_PrecacheRumble, 0 );
	Scr_AddFunction("loadfx", Scr_LoadFX, 0 );
	Scr_AddFunction("playfx", Scr_PlayFX, 0 );
	Scr_AddFunction("playfxontag", Scr_PlayFXOnTag, 0 );
	Scr_AddFunction("playloopedfx", Scr_PlayLoopedFX, 0 );
	Scr_AddFunction("spawnfx", Scr_SpawnFX, 0 );
	Scr_AddFunction("triggerfx", Scr_TriggerFX, 0 );
	Scr_AddFunction("physicsexplosionsphere", Scr_PhysicsExplosionSphere, 0 );
	Scr_AddFunction("physicsexplosioncylinder", Scr_PhysicsExplosionCylinder, 0 );
	Scr_AddFunction("physicsjolt", Scr_PhysicsRadiusJolt, 0 );
	Scr_AddFunction("physicsjitter", Scr_PhysicsRadiusJitter, 0 );
	Scr_AddFunction("setexpfog", Scr_SetExponentialFog, 0 );
	Scr_AddFunction("grenadeexplosioneffect", Scr_GrenadeExplosionEffect, 0 );
	Scr_AddFunction("radiusdamage", GScr_RadiusDamage, 0 );
	Scr_AddFunction("setplayerignoreradiusdamage", GScr_SetPlayerIgnoreRadiusDamage, 0 );
	Scr_AddFunction("getnumparts", GScr_GetNumParts, 0 );
	Scr_AddFunction("getpartname", GScr_GetPartName, 0 );
	Scr_AddFunction("earthquake", GScr_Earthquake, 0 );
	Scr_AddFunction("newteamhudelem", GScr_NewTeamHudElem, 0 );
	Scr_AddFunction("resettimeout", Scr_ResetTimeout, 0 );
	Scr_AddFunction("weaponfiretime", GScr_WeaponFireTime, 0 );
	Scr_AddFunction("isweaponcliponly", GScr_IsWeaponClipOnly, 0 );
	Scr_AddFunction("weaponclipsize", GScr_WeaponClipSize, 0 );
	Scr_AddFunction("weaponissemiauto", GScr_WeaponIsSemiAuto, 0 );
	Scr_AddFunction("weaponisboltaction", GScr_WeaponIsBoltAction, 0 );
	Scr_AddFunction("weapontype", GScr_WeaponType, 0 );
	Scr_AddFunction("weaponclass", GScr_WeaponClass, 0 );
	Scr_AddFunction("weaponinventorytype", GScr_WeaponInventoryType, 0 );
	Scr_AddFunction("weaponstartammo", GScr_WeaponStartAmmo, 0 );
	Scr_AddFunction("weaponmaxammo", GScr_WeaponMaxAmmo, 0 );
	Scr_AddFunction("weaponaltweaponname", GScr_WeaponAltWeaponName, 0 );
	Scr_AddFunction("isplayer", GScr_IsPlayer, 0 );
	Scr_AddFunction("isplayernumber", GScr_IsPlayerNumber, 0 );
	Scr_AddFunction("setwinningplayer", GScr_SetWinningPlayer, 0 );
	Scr_AddFunction("setwinningteam", GScr_SetWinningTeam, 0 );
	Scr_AddFunction("announcement", GScr_Announcement, 0 );
	Scr_AddFunction("clientannouncement", GScr_ClientAnnouncement, 0 );
	Scr_AddFunction("getteamscore", GScr_GetTeamScore, 0 );
	Scr_AddFunction("setteamscore", GScr_SetTeamScore, 0 );
	Scr_AddFunction("setclientnamemode", GScr_SetClientNameMode, 0 );
	Scr_AddFunction("updateclientnames", GScr_UpdateClientNames, 0 );
	Scr_AddFunction("getteamplayersalive", GScr_GetTeamPlayersAlive, 0 );
	Scr_AddFunction("objective_team", GScr_Objective_Team, 0 );
	Scr_AddFunction("logprint", GScr_LogPrint, 0 );
	Scr_AddFunction("worldentnumber", GScr_WorldEntNumber, 0 );
	Scr_AddFunction("obituary", GScr_Obituary, 0 );
	Scr_AddFunction("positionwouldtelefrag", GScr_positionWouldTelefrag, 0 );
	Scr_AddFunction("getstarttime", GScr_getStartTime, 0 );
	Scr_AddFunction("precachemenu", GScr_PrecacheMenu, 0 );
	Scr_AddFunction("precachestatusicon", GScr_PrecacheStatusIcon, 0 );
	Scr_AddFunction("precacheheadicon", GScr_PrecacheHeadIcon, 0 );
	Scr_AddFunction("precachelocationselector", GScr_PrecacheLocationSelector, 0 );
	Scr_AddFunction("map_restart", GScr_MapRestart, 0 );
	Scr_AddFunction("exitlevel", GScr_ExitLevel, 0 );
	Scr_AddFunction("setarchive", GScr_SetArchive, 0 );
	Scr_AddFunction("allclientsprint", GScr_AllClientsPrint, 0 );
	Scr_AddFunction("clientprint", GScr_ClientPrint, 0 );
	Scr_AddFunction("mapexists", GScr_MapExists, 0 );
	Scr_AddFunction("isvalidgametype", GScr_IsValidGameType, 0 );
	Scr_AddFunction("matchend", GScr_MatchEnd, 0 );
	Scr_AddFunction("setplayerteamrank", GScr_SetPlayerTeamRank, 0 );
	Scr_AddFunction("sendranks", GScr_SendXboxLiveRanks, 0 );
	Scr_AddFunction("endparty", GScr_EndXboxLiveLobby, 0 );
	Scr_AddFunction("setteamradar", GScr_SetTeamRadar, 0 );
	Scr_AddFunction("getteamradar", GScr_GetTeamRadar, 0 );
	Scr_AddFunction("getassignedteam", GScr_GetAssignedTeam, 0 );
	Scr_AddFunction("setvotestring", GScr_SetVoteString, 0 );
	Scr_AddFunction("setvotetime", GScr_SetVoteTime, 0 );
	Scr_AddFunction("setvoteyescount", GScr_SetVoteYesCount, 0 );
	Scr_AddFunction("setvotenocount", GScr_SetVoteNoCount, 0 );
	Scr_AddFunction("fprintfields", GScr_FPrintFields, 1 );
	Scr_AddFunction("fgetarg", GScr_FGetArg, 1 );
	//Scr_AddFunction("kick", GScr_KickPlayer, 0 );
	//Scr_AddFunction("ban", GScr_BanPlayer, 0 );
	Scr_AddFunction("map", GScr_LoadMap, 0 );
	Scr_AddFunction("playrumbleonposition", Scr_PlayRumbleOnPosition, 0 );
	Scr_AddFunction("playrumblelooponposition", Scr_PlayRumbleLoopOnPosition, 0 );
	Scr_AddFunction("stopallrumbles", Scr_StopAllRumbles, 0 );
	Scr_AddFunction("soundexists", ScrCmd_SoundExists, 0 );
	Scr_AddFunction("issplitscreen", Scr_IsSplitscreen, 0 );
	Scr_AddFunction("setminimap", GScr_SetMiniMap, 0 );
	Scr_AddFunction("setmapcenter", GScr_SetMapCenter, 0 );
	Scr_AddFunction("setgameendtime", GScr_SetGameEndTime, 0 );
	Scr_AddFunction("getarraykeys", GScr_GetArrayKeys, 0 );
	Scr_AddFunction("searchforonlinegames", GScr_SearchForOnlineGames, 0 );
	Scr_AddFunction("quitlobby", GScr_QuitLobby, 0 );
	Scr_AddFunction("quitparty", GScr_QuitParty, 0 );
	Scr_AddFunction("startparty", GScr_StartParty, 0 );
	Scr_AddFunction("startprivatematch", GScr_StartPrivateMatch, 0 );
	Scr_AddFunction("visionsetnaked", Scr_VisionSetNaked, 0 );
	Scr_AddFunction("visionsetnight", Scr_VisionSetNight, 0 );
	Scr_AddFunction("tablelookup", Scr_TableLookup, 0 );
	Scr_AddFunction("tablelookupistring", Scr_TableLookupIString, 0 );
	Scr_AddFunction("endlobby", GScr_EndLobby, 0 );
	Scr_AddFunction("logstring", Scr_LogString, 0);

    //User edited functions
    Scr_AddFunction("spawn", GScr_Spawn, 0);
    //	Scr_AddFunction("spawnvehicle", GScr_SpawnVehicle, 0);
    Scr_AddFunction("spawnhelicopter", GScr_SpawnHelicopter, 0);
    Scr_AddFunction("getdvar", GScr_GetCvar, 0);
    Scr_AddFunction("getdvarint", GScr_GetCvarInt, 0);
    Scr_AddFunction("getdvarfloat", GScr_GetCvarFloat, 0);
    Scr_AddFunction("setdvar", GScr_SetCvar, 0);
    Scr_AddFunction("vectoradd", GScr_VectorAdd, 0);
    Scr_AddFunction("precachestring", Scr_PrecacheString_f, 0);
    Scr_AddFunction("newhudelem", GScr_NewHudElem, 0);
    Scr_AddFunction("newclienthudelem", GScr_NewClientHudElem, 0);
    Scr_AddFunction("addtestclient", GScr_SpawnBot, 0);
    Scr_AddFunction("removetestclient", GScr_RemoveBot, 0);
    Scr_AddFunction("removealltestclients", GScr_RemoveAllBots, 0);
    Scr_AddFunction("makedvarserverinfo", GScr_MakeCvarServerInfo, 0);
    Scr_AddFunction("openfile", GScr_FS_FOpen, 0);
    Scr_AddFunction("closefile", GScr_FS_FClose, 0);
    Scr_AddFunction("fprintln", GScr_FS_WriteLine, 0);
    Scr_AddFunction("freadln", GScr_FS_ReadLine, 0);
    Scr_AddFunction("kick", GScr_KickClient, 0);
    Scr_AddFunction("ban", GScr_BanClient, 0);
    Scr_AddFunction("fs_fopen", GScr_FS_FOpen, 0);
    Scr_AddFunction("fs_fclose", GScr_FS_FClose, 0);
    Scr_AddFunction("fs_fcloseall", GScr_FS_FCloseAll, 0);
    Scr_AddFunction("fs_testfile", GScr_FS_TestFile, 0);
    Scr_AddFunction("fs_readline", GScr_FS_ReadLine, 0);
    Scr_AddFunction("fs_writeline", GScr_FS_WriteLine, 0);
    Scr_AddFunction("fs_remove", GScr_FS_Remove, 0);
    Scr_AddFunction("getrealtime", GScr_GetRealTime, 0);
    Scr_AddFunction("timetostring", GScr_TimeToString, 0);
    Scr_AddFunction("strtokbypixlen", GScr_StrTokByPixLen, 0);
    Scr_AddFunction("strpixlen", GScr_StrPixLen, 0);
    Scr_AddFunction("strcolorstrip", GScr_StrColorStrip, 0);
    Scr_AddFunction("copystr", GScr_CopyString, 0);
    Scr_AddFunction("strrepl", GScr_StrRepl, 0);
    Scr_AddFunction("strtokbylen", GScr_StrTokByLen, 0);
    Scr_AddFunction("exec", GScr_CbufAddText, 0);
    Scr_AddFunction("execex", GScr_CbufAddTextEx, 0);
    Scr_AddFunction("sha256", GScr_SHA256, 0);
    Scr_AddFunction("addscriptcommand", GScr_AddScriptCommand, 0);
    Scr_AddFunction("isarray", Scr_IsArray_f, qfalse); // http://zeroy.com/script/variables/isarray.htm
    Scr_AddFunction("iscvardefined", GScr_IsCvarDefined, 0);
    Scr_AddFunction("arraytest", GScr_ArrayTest, 1);
}

void Scr_AddStockMethods()
{


    //PlayerCmd

	Scr_AddMethod("giveweapon", PlayerCmd_giveWeapon, 0 );
	Scr_AddMethod("takeweapon", PlayerCmd_takeWeapon, 0 );
	Scr_AddMethod("takeallweapons", PlayerCmd_takeAllWeapons, 0 );
	Scr_AddMethod("getcurrentweapon", PlayerCmd_getCurrentWeapon, 0 );
	Scr_AddMethod("getcurrentoffhand", PlayerCmd_getCurrentOffhand, 0 );
	Scr_AddMethod("hasweapon", PlayerCmd_hasWeapon, 0 );
	Scr_AddMethod("switchtoweapon", PlayerCmd_switchToWeapon, 0 );
	Scr_AddMethod("switchtooffhand", PlayerCmd_switchToOffhand, 0 );
	Scr_AddMethod("givestartammo", PlayerCmd_giveStartAmmo, 0 );
	Scr_AddMethod("givemaxammo", PlayerCmd_giveMaxAmmo, 0 );
	Scr_AddMethod("getfractionstartammo", PlayerCmd_getFractionStartAmmo, 0 );
	Scr_AddMethod("getfractionmaxammo", PlayerCmd_getFractionMaxAmmo, 0 );
	Scr_AddMethod("setorigin", PlayerCmd_setOrigin, 0 );
	Scr_AddMethod("getvelocity", PlayerCmd_GetVelocity, 0 );
	Scr_AddMethod("setplayerangles", PlayerCmd_setAngles, 0 );
	Scr_AddMethod("getplayerangles", PlayerCmd_getAngles, 0 );
	Scr_AddMethod("usebuttonpressed", PlayerCmd_useButtonPressed, 0 );
	Scr_AddMethod("attackbuttonpressed", PlayerCmd_attackButtonPressed, 0 );
	Scr_AddMethod("meleebuttonpressed", PlayerCmd_meleeButtonPressed, 0 );
	Scr_AddMethod("fragbuttonpressed", PlayerCmd_fragButtonPressed, 0 );
	Scr_AddMethod("secondaryoffhandbuttonpressed", PlayerCmd_secondaryOffhandButtonPressed, 0 );
	Scr_AddMethod("playerads", PlayerCmd_playerADS, 0 );
	Scr_AddMethod("isonground", PlayerCmd_isOnGround, 0 );
	Scr_AddMethod("pingplayer", PlayerCmd_pingPlayer, 0 );
	Scr_AddMethod("setviewmodel", PlayerCmd_SetViewmodel, 0 );
	Scr_AddMethod("getviewmodel", PlayerCmd_GetViewmodel, 0 );
	Scr_AddMethod("setoffhandsecondaryclass", PlayerCmd_setOffhandSecondaryClass, 0 );
	Scr_AddMethod("getoffhandsecondaryclass", PlayerCmd_getOffhandSecondaryClass, 0 );
	Scr_AddMethod("beginlocationselection", PlayerCmd_BeginLocationSelection, 0 );
	Scr_AddMethod("endlocationselection", PlayerCmd_EndLocationSelection, 0 );
	Scr_AddMethod("buttonpressed", PlayerCmd_buttonPressedDEVONLY, 0 );
	Scr_AddMethod("sayall", PlayerCmd_SayAll, 0 );
	Scr_AddMethod("sayteam", PlayerCmd_SayTeam, 0 );
	Scr_AddMethod("showscoreboard", PlayerCmd_showScoreboard, 0 );
	Scr_AddMethod("setspawnweapon", PlayerCmd_setSpawnWeapon, 0 );
	Scr_AddMethod("dropitem", PlayerCmd_dropItem, 0 );
	Scr_AddMethod("finishplayerdamage", PlayerCmd_finishPlayerDamage, 0 );
	Scr_AddMethod("suicide", PlayerCmd_Suicide, 0 );
	Scr_AddMethod("openmenu", PlayerCmd_OpenMenu, 0 );
	Scr_AddMethod("openmenunomouse", PlayerCmd_OpenMenuNoMouse, 0 );
	Scr_AddMethod("closemenu", PlayerCmd_CloseMenu, 0 );
	Scr_AddMethod("closeingamemenu", PlayerCmd_CloseInGameMenu, 0 );
	Scr_AddMethod("freezecontrols", PlayerCmd_FreezeControls, 0 );
	Scr_AddMethod("disableweapons", PlayerCmd_DisableWeapons, 0 );
	Scr_AddMethod("enableweapons", PlayerCmd_EnableWeapons, 0 );
	Scr_AddMethod("setreverb", PlayerCmd_SetReverb, 0 );
	Scr_AddMethod("deactivatereverb", PlayerCmd_DeactivateReverb, 0 );
	Scr_AddMethod("setchannelvolumes", PlayerCmd_SetChannelVolumes, 0 );
	Scr_AddMethod("deactivatechannelvolumes", PlayerCmd_DeactivateChannelVolumes, 0 );
	Scr_AddMethod("setweaponammoclip", PlayerCmd_SetWeaponAmmoClip, 0 );
	Scr_AddMethod("setweaponammostock", PlayerCmd_SetWeaponAmmoStock, 0 );
	Scr_AddMethod("getweaponammoclip", PlayerCmd_GetWeaponAmmoClip, 0 );
	Scr_AddMethod("getweaponammostock", PlayerCmd_GetWeaponAmmoStock, 0 );
	Scr_AddMethod("anyammoforweaponmodes", PlayerCmd_AnyAmmoForWeaponModes, 0 );
	Scr_AddMethod("iprintln", iclientprintln, 0 );
	Scr_AddMethod("iprintlnbold", iclientprintlnbold, 0 );
	Scr_AddMethod("setentertime", PlayerCmd_setEnterTime, 0 );
	Scr_AddMethod("cloneplayer", PlayerCmd_ClonePlayer, 0 );
	Scr_AddMethod("setclientdvar", PlayerCmd_SetClientDvar, 0 );
	Scr_AddMethod("setclientdvars", PlayerCmd_SetClientDvars, 0 );
	Scr_AddMethod("playlocalsound", ScrCmd_PlayLocalSound, 0 );
	Scr_AddMethod("stoplocalsound", ScrCmd_StopLocalSound, 0 );
	Scr_AddMethod("istalking", PlayerCmd_IsTalking, 0 );
	Scr_AddMethod("allowspectateteam", PlayerCmd_AllowSpectateTeam, 0 );
	Scr_AddMethod("allowads", PlayerCmd_AllowADS, 0 );
	Scr_AddMethod("allowjump", PlayerCmd_AllowJump, 0 );
	Scr_AddMethod("allowsprint", PlayerCmd_AllowSprint, 0 );
	Scr_AddMethod("setspreadoverride", PlayerCmd_SetSpreadOverride, 0 );
	Scr_AddMethod("resetspreadoverride", PlayerCmd_ResetSpreadOverride, 0 );
	Scr_AddMethod("setactionslot", PlayerCmd_SetActionSlot, 0 );
	Scr_AddMethod("getweaponslist", PlayerCmd_GetWeaponsList, 0 );
	Scr_AddMethod("getweaponslistprimaries", PlayerCmd_GetWeaponsListPrimaries, 0 );
	Scr_AddMethod("setperk", PlayerCmd_SetPerk, 0 );
	Scr_AddMethod("hasperk", PlayerCmd_HasPerk, 0 );
	Scr_AddMethod("clearperks", PlayerCmd_ClearPerks, 0 );
	Scr_AddMethod("unsetperk", PlayerCmd_UnsetPerk, 0 );
	Scr_AddMethod("setrank", PlayerCmd_SetRank, 0 );
	Scr_AddMethod("updatedmscores", PlayerCmd_UpdateDMScores, 0 );


    Scr_AddMethod("getpower", PlayerCmd_GetPower, 0);
    Scr_AddMethod("setpower", PlayerCmd_SetPower, 0);
    Scr_AddMethod("setuid", PlayerCmd_SetUid, 0);
    Scr_AddMethod("spawn", PlayerCmd_spawn, 0);
    Scr_AddMethod("getguid", PlayerCmd_GetGuid, 0);
    Scr_AddMethod("getxuid", PlayerCmd_GetXuid, 0);
    Scr_AddMethod("getuid", PlayerCmd_GetUid, 0);
    Scr_AddMethod("getsteamid", PlayerCmd_GetSteamID, 0);
    Scr_AddMethod("getplayerid", PlayerCmd_GetPlayerID, 0);
    Scr_AddMethod("getsteamid64", PlayerCmd_GetSteamID, 0);
    Scr_AddMethod("getplayerid64", PlayerCmd_GetPlayerID, 0);
    Scr_AddMethod("getuserinfo", PlayerCmd_GetUserinfo, 0);
    Scr_AddMethod("getping", PlayerCmd_GetPing, 0);


    //HUD Functions

	Scr_AddMethod("settext", HECmd_SetText, 0 );
	Scr_AddMethod("clearalltextafterhudelem", HECmd_ClearAllTextAfterHudElem, 0 );
	Scr_AddMethod("setshader", HECmd_SetMaterial, 0 );
	Scr_AddMethod("settimer", HECmd_SetTimer, 0 );
	Scr_AddMethod("settimerup", HECmd_SetTimerUp, 0 );
	Scr_AddMethod("settenthstimer", HECmd_SetTenthsTimer, 0 );
	Scr_AddMethod("settenthstimerup", HECmd_SetTenthsTimerUp, 0 );
	Scr_AddMethod("setclock", HECmd_SetClock, 0 );
	Scr_AddMethod("setclockup", HECmd_SetClockUp, 0 );
	Scr_AddMethod("setvalue", HECmd_SetValue, 0 );
	Scr_AddMethod("setwaypoint", HECmd_SetWaypoint, 0 );
	Scr_AddMethod("fadeovertime", HECmd_FadeOverTime, 0 );
	Scr_AddMethod("scaleovertime", HECmd_ScaleOverTime, 0 );
	Scr_AddMethod("moveovertime", HECmd_MoveOverTime, 0 );
	Scr_AddMethod("reset", HECmd_Reset, 0 );
//	Scr_AddMethod("destroy", HECmd_Destroy, 0 );
	Scr_AddMethod("setpulsefx", HECmd_SetPulseFX, 0 );
	Scr_AddMethod("setplayernamestring", HECmd_SetPlayerNameString, 0 );
	Scr_AddMethod("setmapnamestring", HECmd_SetMapNameString, 0 );
	Scr_AddMethod("setgametypestring", HECmd_SetGameTypeString, 0 );
	Scr_AddMethod("cleartargetent", HECmd_ClearTargetEnt, 0);
	Scr_AddMethod("settargetent", HECmd_SetTargetEnt, 0 );
    Scr_AddMethod("destroy", Scr_Destroy_f, 0);


    //Scr Cmd Functions
	Scr_AddMethod("attach", ScrCmd_attach, 0 );
	Scr_AddMethod("detach", ScrCmd_detach, 0 );
	Scr_AddMethod("detachall", ScrCmd_detachAll, 0 );
	Scr_AddMethod("getattachsize", ScrCmd_GetAttachSize, 0 );
	Scr_AddMethod("getattachmodelname", ScrCmd_GetAttachModelName, 0 );
	Scr_AddMethod("getattachtagname", ScrCmd_GetAttachTagName, 0 );
	Scr_AddMethod("getattachignorecollision", ScrCmd_GetAttachIgnoreCollision, 0 );
	Scr_AddMethod("getammocount", GScr_GetAmmoCount, 0 );
	Scr_AddMethod("getclanid", ScrCmd_GetClanId, 0 );
	Scr_AddMethod("getclanname", ScrCmd_GetClanName, 0 );
	Scr_AddMethod("hidepart", ScrCmd_hidepart, 0 );
	Scr_AddMethod("showpart", ScrCmd_showpart, 0 );
	Scr_AddMethod("showallparts", ScrCmd_showallparts, 0 );
	Scr_AddMethod("linkto", ScrCmd_LinkTo, 0 );
	Scr_AddMethod("unlink", ScrCmd_Unlink, 0 );
	Scr_AddMethod("enablelinkto", ScrCmd_EnableLinkTo, 0 );
	Scr_AddMethod("getorigin", ScrCmd_GetOrigin, 0 );
	Scr_AddMethod("geteye", ScrCmd_GetEye, 0 );
	Scr_AddMethod("useby", ScrCmd_UseBy, 0 );
	Scr_AddMethod("setstablemissile", Scr_SetStableMissile, 0 );
	Scr_AddMethod("istouching", ScrCmd_IsTouching, 0 );
	Scr_AddMethod("playsound", ScrCmd_PlaySound, 0 );
	Scr_AddMethod("playsoundasmaster", ScrCmd_PlaySoundAsMaster, 0 );
	Scr_AddMethod("playsoundtoteam", ScrCmd_PlaySoundToTeam, 0 );
	Scr_AddMethod("playsoundtoplayer", ScrCmd_PlaySoundToPlayer, 0 );
	Scr_AddMethod("playloopsound", ScrCmd_PlayLoopSound, 0 );
	Scr_AddMethod("stoploopsound", ScrCmd_StopLoopSound, 0 );
	Scr_AddMethod("playrumbleonentity", ScrCmd_PlayRumbleOnEntity, 0 );
	Scr_AddMethod("playrumblelooponentity", ScrCmd_PlayRumbleLoopOnEntity, 0 );
	Scr_AddMethod("stoprumble", ScrCmd_StopRumble, 0 );
	Scr_AddMethod("delete", ScrCmd_Delete, 0 );
	Scr_AddMethod("setmodel", ScrCmd_SetModel, 0 );
	Scr_AddMethod("getnormalhealth", ScrCmd_GetNormalHealth, 0 );
	Scr_AddMethod("setnormalhealth", ScrCmd_SetNormalHealth, 0 );
	Scr_AddMethod("show", ScrCmd_Show, 0 );
	Scr_AddMethod("hide", ScrCmd_Hide, 0 );
	Scr_AddMethod("laseron", ScrCmd_LaserOn, 0 );
	Scr_AddMethod("laseroff", ScrCmd_LaserOff, 0 );
	Scr_AddMethod("showtoplayer", ScrCmd_ShowToPlayer, 0 );
	Scr_AddMethod("setcontents", ScrCmd_SetContents, 0 );
	Scr_AddMethod("getstance", ScrCmd_GetStance, 0 );
	Scr_AddMethod("setcursorhint", GScr_SetCursorHint, 0 );
	Scr_AddMethod("sethintstring", GScr_SetHintString, 0 );
	Scr_AddMethod("usetriggerrequirelookat", GScr_UseTriggerRequireLookAt, 0 );
	Scr_AddMethod("shellshock", GScr_ShellShock, 0 );
	Scr_AddMethod("gettagorigin", GScr_GetTagOrigin, 0 );
	Scr_AddMethod("gettagangles", GScr_GetTagAngles, 0 );
	Scr_AddMethod("stopshellshock", GScr_StopShellShock, 0 );
	Scr_AddMethod("setdepthoffield", GScr_SetDepthOfField, 0 );
	Scr_AddMethod("viewkick", GScr_ViewKick, 0 );
	Scr_AddMethod("localtoworldcoords", GScr_LocalToWorldCoords, 0 );
	Scr_AddMethod("setrightarc", GScr_SetRightArc, 0 );
	Scr_AddMethod("setleftarc", GScr_SetLeftArc, 0 );
	Scr_AddMethod("settoparc", GScr_SetTopArc, 0 );
	Scr_AddMethod("setbottomarc", GScr_SetBottomArc, 0 );
	Scr_AddMethod("radiusdamage", GScr_EntityRadiusDamage, 0 );
	Scr_AddMethod("detonate", GScr_Detonate, 0 );
	Scr_AddMethod("damageconetrace", GScr_DamageConeTrace, 0 );
	Scr_AddMethod("sightconetrace", GScr_SightConeTrace, 0 );
	Scr_AddMethod("getentitynumber", GScr_GetEntityNumber, 0 );
	Scr_AddMethod("enablegrenadetouchdamage", GScr_EnableGrenadeTouchDamage, 0 );
	Scr_AddMethod("disablegrenadetouchdamage", GScr_DisableGrenadeTouchDamage, 0 );
	Scr_AddMethod("enableaimassist", GScr_EnableAimAssist, 0 );
	Scr_AddMethod("disableaimassist", GScr_DisableAimAssist, 0 );
	Scr_AddMethod("placespawnpoint", GScr_PlaceSpawnPoint, 0 );
	Scr_AddMethod("updatescores", PlayerCmd_UpdateScores, 0 );
	Scr_AddMethod("setteamfortrigger", GScr_SetTeamForTrigger, 0 );
	Scr_AddMethod("clientclaimtrigger", GScr_ClientClaimTrigger, 0 );
	Scr_AddMethod("clientreleasetrigger", GScr_ClientReleaseTrigger, 0 );
	Scr_AddMethod("releaseclaimedtrigger", GScr_ReleaseClaimedTrigger, 0 );
	Scr_AddMethod("sendleaderboards", GScr_SendLeaderboards, 0 );
	Scr_AddMethod("setmovespeedscale", ScrCmd_SetMoveSpeedScale, 0 );
	Scr_AddMethod("missile_settarget", GScr_MissileSetTarget, 0 );
	Scr_AddMethod("startragdoll", GScr_StartRagdoll, 0 );
	Scr_AddMethod("isragdoll", GScr_IsRagdoll, 0 );
	Scr_AddMethod("getcorpseanim", GScr_GetCorpseAnim, 0 );
	Scr_AddMethod("itemweaponsetammo", ScrCmd_ItemWeaponSetAmmo, 0 );
	Scr_AddMethod("logstring", ScrCmd_LogString, 0 );
	Scr_AddMethod("isonladder", GScr_IsOnLadder, 0 );
	Scr_AddMethod("ismantling", GScr_IsMantling, 0 );



    //	Scr_AddMethod("setstance", ScrCmd_SetStance, 0);
    Scr_AddMethod("setjumpheight", PlayerCmd_SetJumpHeight, 0);
    Scr_AddMethod("setgravity", PlayerCmd_SetGravity, 0);
    Scr_AddMethod("setgroundreferenceent", PlayerCmd_SetGroundReferenceEnt, 0);
    Scr_AddMethod("setmovespeed", PlayerCmd_SetMoveSpeed, 0);


    Scr_AddMethod("getstat", PlayerCmd_GetStat, 0);
    Scr_AddMethod("setstat", PlayerCmd_SetStat, 0);


    //Scr Ent Functions
	Scr_AddMethod("moveto", ScriptEntCmd_MoveTo, 0 );
	Scr_AddMethod("movex", ScriptEntCmd_MoveX, 0 );
	Scr_AddMethod("movey", ScriptEntCmd_MoveY, 0 );
	Scr_AddMethod("movez", ScriptEntCmd_MoveZ, 0 );
	Scr_AddMethod("movegravity", ScriptEntCmd_GravityMove, 0 );
	Scr_AddMethod("rotateto", ScriptEntCmd_RotateTo, 0 );
	Scr_AddMethod("rotatepitch", ScriptEntCmd_RotatePitch, 0 );
	Scr_AddMethod("rotateyaw", ScriptEntCmd_RotateYaw, 0 );
	Scr_AddMethod("rotateroll", ScriptEntCmd_RotateRoll, 0 );
	Scr_AddMethod("devaddpitch", ScriptEntCmd_DevAddPitch, 1 );
	Scr_AddMethod("devaddyaw", ScriptEntCmd_DevAddYaw, 1 );
	Scr_AddMethod("devaddroll", ScriptEntCmd_DevAddRoll, 1 );
	Scr_AddMethod("vibrate", ScriptEntCmd_Vibrate, 0 );
	Scr_AddMethod("rotatevelocity", ScriptEntCmd_RotateVelocity, 0 );
	Scr_AddMethod("solid", ScriptEntCmd_Solid, 0 );
	Scr_AddMethod("notsolid", ScriptEntCmd_NotSolid, 0 );
	Scr_AddMethod("setcandamage", ScriptEntCmd_SetCanDamage, 0 );
	Scr_AddMethod("physicslaunch", ScriptEntCmd_PhysicsLaunch, 0 );


    //Helicopter Functions
	Scr_AddMethod("freehelicopter", CMD_Heli_FreeHelicopter, 0 );
	Scr_AddMethod("setspeed", CMD_VEH_SetSpeed, 0 );
	Scr_AddMethod("getspeed", CMD_VEH_GetSpeed, 0 );
	Scr_AddMethod("getspeedmph", CMD_VEH_GetSpeedMPH, 0 );
	Scr_AddMethod("resumespeed", CMD_VEH_ResumeSpeed, 0 );
	Scr_AddMethod("setyawspeed", CMD_VEH_SetYawSpeed, 0 );
	Scr_AddMethod("setmaxpitchroll", CMD_VEH_SetMaxPitchRoll, 0 );
	Scr_AddMethod("setturningability", CMD_VEH_SetTurningAbility, 0 );
	Scr_AddMethod("setairresistance", CMD_VEH_SetAirResitance, 0 );
	Scr_AddMethod("sethoverparams", CMD_VEH_SetHoverParams, 0 );
	Scr_AddMethod("setneargoalnotifydist", CMD_VEH_NearGoalNotifyDist, 0 );
	Scr_AddMethod("setvehgoalpos", CMD_VEH_SetGoalPos, 0 );
	Scr_AddMethod("setgoalyaw", CMD_VEH_SetGoalYaw, 0 );
	Scr_AddMethod("cleargoalyaw", CMD_VEH_ClearGoalYaw, 0 );
	Scr_AddMethod("settargetyaw", CMD_VEH_SetTargetYaw, 0 );
	Scr_AddMethod("cleartargetyaw", CMD_VEH_ClearTargetYaw, 0 );
	Scr_AddMethod("setlookatent", CMD_VEH_SetLookAtEnt, 0 );
	Scr_AddMethod("clearlookatent", CMD_VEH_ClearLookAtEnt, 0 );
	Scr_AddMethod("setvehweapon", CMD_VEH_SetWeapon, 0 );
	Scr_AddMethod("fireweapon", CMD_VEH_FireWeapon, 0 );
	Scr_AddMethod("setturrettargetvec", CMD_VEH_SetTurretTargetVec, 0 );
	Scr_AddMethod("setturrettargetent", CMD_VEH_SetTurretTargetEnt, 0 );
	Scr_AddMethod("clearturrettarget", CMD_VEH_ClearTurretTargetEnt, 0 );
	Scr_AddMethod("setvehicleteam", CMD_VEH_SetVehicleTeam, 0 );
	Scr_AddMethod("setdamagestage", CMD_Heli_SetDamageStage, 0 );

    // Player movement detection.
    Scr_AddMethod("forwardbuttonpressed", PlayerCmd_ForwardButtonPressed, 0);
    Scr_AddMethod("backbuttonpressed", PlayerCmd_BackButtonPressed, 0);
    Scr_AddMethod("moveleftbuttonpressed", PlayerCmd_MoveLeftButtonPressed, 0);
    Scr_AddMethod("moverightbuttonpressed", PlayerCmd_MoveRightButtonPressed, 0);
    Scr_AddMethod("sprintbuttonpressed", PlayerCmd_SprintButtonPressed, 0);
    Scr_AddMethod("reloadbuttonpressed", PlayerCmd_ReloadButtonPressed, 0);
    Scr_AddMethod("leanleftbuttonpressed", PlayerCmd_LeanLeftButtonPressed, 0);
    Scr_AddMethod("leanrightbuttonpressed", PlayerCmd_LeanRightButtonPressed, 0);
    Scr_AddMethod("isproning", PlayerCmd_IsProning, 0);
    Scr_AddMethod("iscrouching", PlayerCmd_IsCrouching, 0);
    Scr_AddMethod("isstanding", PlayerCmd_IsStanding, 0);
    Scr_AddMethod("jumpbuttonpressed", PlayerCmd_JumpButtonPressed, 0);
    Scr_AddMethod("isinads", PlayerCmd_IsInADS, 0);
    Scr_AddMethod("holdbreathbuttonpressed", PlayerCmd_HoldBreathButtonPressed, 0);
    Scr_AddMethod("aimbuttonpressed", PlayerCmd_ADSButtonPressed, 0);

    Scr_AddMethod("steamgroupmembershipquery", PlayerCmd_GetSteamGroupMembership, 0);
    Scr_AddMethod("setvelocity", PlayerCmd_SetVelocity, qfalse);

    /* Bot movement */
    Scr_AddBotsMovement();

    Scr_AddMethod("getgeolocation", PlayerCmd_GetGeoLocation, 0);
    Scr_AddMethod("getcountedfps", PlayerCmd_GetCountedFPS, 0);
    Scr_AddMethod("getspectatorclient", PlayerCmd_GetSpectatorClient, 0);

}

void Scr_InitFunctions()
{

    static qboolean initialized = qfalse;

    /*
    No longer

    //Reset everything 1st

    Scr_ClearFunctions();
    Scr_ClearMethods();
*/
    //Then add everything again

    if (!initialized)
    {
        Scr_AddStockFunctions();
        Scr_AddStockMethods();
        initialized = qtrue;
    }
}

int GScr_LoadScriptAndLabel(const char *scriptName, const char *labelName, qboolean mandatory)
{

    int fh;
    PrecacheEntry load_buffer;

    if (!Scr_LoadScript(scriptName, &load_buffer, 0))
    {
        if (mandatory)
        {
            Com_Error(ERR_DROP, "Could not find script '%s'", scriptName);
        }
        else
        {
            Com_DPrintf(CON_CHANNEL_SCRIPT,"Notice: Could not find script '%s' - this part will be disabled\n", scriptName);
        }
        return 0;
    }

    fh = Scr_GetFunctionHandle(scriptName, labelName);

    if (!fh)
    {
        if (mandatory)
        {
            Com_Error(ERR_DROP, "Could not find label '%s' in script '%s'", labelName, scriptName);
        }
        else
        {
            Com_DPrintf(CON_CHANNEL_SCRIPT,"Notice: Could not find label '%s' in script '%s' - this part will be disabled\n", labelName, scriptName);
        }
        return 0;
    }

    return fh;
}

/**************** Mandatory *************************/

struct gameTypeScript_t
{
  char pszScript[64];
  char pszName[64];
  int bTeamBased;
};

struct scr_gametype_data_t
{
  int main;
  int startupgametype;
  int playerconnect;
  int playerdisconnect;
  int playerdamage;
  int playerkilled;
  int votecalled;
  int playervote;
  int playerlaststand;
  int iNumGameTypes;
  struct gameTypeScript_t list[32];
};

#pragma pack(push, 4)
struct corpseInfo_t
{
  struct XAnimTree_s *tree;
  int entnum;
  int time;
  struct clientInfo_t ci;
  byte falling;
  byte pad[3];
};
#pragma pack(pop)

struct scr_data_t
{
  int levelscript;
  int gametypescript;
  struct scr_gametype_data_t gametype;
  int delete;
  int initstructs;
  int createstruct;
  struct corpseInfo_t playerCorpseInfo[8];
};

extern struct scr_data_t g_scr_data;


int script_CallBacks_new[8];

void GScr_LoadGameTypeScript(void)
{

    /**************** Mandatory *************************/
    char gametype_path[64];

    Cvar_RegisterString("g_gametype", "dm", CVAR_LATCH | CVAR_SERVERINFO, "Current game type");

    Com_sprintf(gametype_path, sizeof(gametype_path), "maps/mp/gametypes/%s", sv_g_gametype->string);

    /* Don't raise a fatal error when we couldn't find this gametype script */
    g_scr_data.gametype.main = GScr_LoadScriptAndLabel(gametype_path, "main", 0);
    if (g_scr_data.gametype.main == 0)
    {
        Com_PrintError(CON_CHANNEL_SCRIPT,"Could not find script: %s\n", gametype_path);
        Com_Printf(CON_CHANNEL_SCRIPT,"The gametype %s is not available! Will load gametype dm\n", sv_g_gametype->string);

        Cvar_SetString(sv_g_gametype, "dm");
        Com_sprintf(gametype_path, sizeof(gametype_path), "maps/mp/gametypes/%s", sv_g_gametype->string);
        /* If we can not find gametype dm a fatal error gets raised */
        g_scr_data.gametype.main = GScr_LoadScriptAndLabel(gametype_path, "main", 1);
    }
    g_scr_data.gametype.startupgametype = GScr_LoadScriptAndLabel("maps/mp/gametypes/_callbacksetup", "CodeCallback_StartGameType", 1);
    g_scr_data.gametype.playerconnect = GScr_LoadScriptAndLabel("maps/mp/gametypes/_callbacksetup", "CodeCallback_PlayerConnect", 1);
    g_scr_data.gametype.playerdisconnect = GScr_LoadScriptAndLabel("maps/mp/gametypes/_callbacksetup", "CodeCallback_PlayerDisconnect", 1);
    g_scr_data.gametype.playerdamage = GScr_LoadScriptAndLabel("maps/mp/gametypes/_callbacksetup", "CodeCallback_PlayerDamage", 1);
    g_scr_data.gametype.playerkilled = GScr_LoadScriptAndLabel("maps/mp/gametypes/_callbacksetup", "CodeCallback_PlayerKilled", 1);
    g_scr_data.gametype.playerlaststand = GScr_LoadScriptAndLabel("maps/mp/gametypes/_callbacksetup", "CodeCallback_PlayerLastStand", 1);

    /**************** Additional *************************/
    script_CallBacks_new[SCR_CB_SCRIPTCOMMAND] = GScr_LoadScriptAndLabel("maps/mp/gametypes/_callbacksetup", "CodeCallback_ScriptCommand", 0);
}
/*
void GScr_AddFieldsForEntity()
{
    int i = 0;
    ent_field_t *iterator = ent_fields;
    while(iterator->name)
    {
        Scr_AddClassField(0, iterator->name, i);
        ++i;
        ++iterator;
    }
}

void GScr_AddFieldsForClient()
{
    int i = 0;
    client_fields_t *iterator = client_fields;
    while(iterator->name)
    {
        Scr_AddClassField(0, iterator->name, 0xc000 + i);
        ++i;
        ++iterator;
    }
}

*/


/*

Bug in array - don't use!

void Scr_GetClientField(gclient_t* gcl, int num)
{

    client_fields_t *field = (client_fields_t *)0x8215780;

    field = &field[num];
//    client_fields_t *field = &clientField[num];


    if(field->getfun)

        field->getfun(gcl, field);

    else

        Scr_GetGenericField(gcl, field->type, field->val1);

}


void Scr_SetClientField(gclient_t* gcl, int num)
{
    client_fields_t *field = (client_fields_t *)0x8215780;

    field = &field[num];

//    client_fields_t *field = &clientField[num];

    if(field->setfun)

        field->setfun(gcl, field);

    else

        Scr_SetGenericField(gcl, field->type, field->val1);

}
*/

void __cdecl GScr_LoadScripts(void)
{
    char mappath[MAX_QPATH];
    cvar_t *mapname;
    int i;

    Scr_BeginLoadScripts();
    Scr_InitFunctions();

    g_scr_data.delete = GScr_LoadScriptAndLabel("codescripts/delete", "main", 1);
    g_scr_data.initstructs = GScr_LoadScriptAndLabel("codescripts/struct", "initstructs", 1);
    g_scr_data.createstruct = GScr_LoadScriptAndLabel("codescripts/struct", "createstruct", 1);

    GScr_LoadGameTypeScript();

    mapname = Cvar_RegisterString("mapname", "", CVAR_LATCH | CVAR_SYSTEMINFO, "The current map name");

    Com_sprintf(mappath, sizeof(mappath), "maps/mp/%s", mapname->string);

    g_scr_data.levelscript = GScr_LoadScriptAndLabel(mappath, "main", qfalse);

    for (i = 0; i < 4; ++i)
        Scr_SetClassMap(i);

    GScr_AddFieldsForEntity();
    GScr_AddFieldsForClient();
    GScr_AddFieldsForHudElems();
    GScr_AddFieldsForRadiant();
    Scr_EndLoadScripts();
}

#define MAX_CALLSCRIPTSTACKDEPTH 200

__cdecl unsigned int Scr_LoadScript(const char *scriptname, PrecacheEntry *precache, int iarg_02)
{

    sval_u result;

    char filepath[MAX_QPATH];
    const char *oldFilename;
    void *scr_buffer_handle;

    const char* oldSourceBuf;
    int oldAnimTreeNames;
    unsigned int handle;
    unsigned int variable;
    unsigned int object;

    int i;
    static unsigned int callScriptStackPtr = 0;
    static char callScriptStackNames[MAX_QPATH * (MAX_CALLSCRIPTSTACKDEPTH + 1)];

    Q_strncpyz(&callScriptStackNames[MAX_QPATH * callScriptStackPtr], scriptname, MAX_QPATH);

    if (callScriptStackPtr >= MAX_CALLSCRIPTSTACKDEPTH)
    {
        Com_Printf(CON_CHANNEL_SCRIPT,"Called too many scripts in chain\nThe scripts are:\n");
        for (i = MAX_CALLSCRIPTSTACKDEPTH; i >= 0; --i)
        {
            Com_Printf(CON_CHANNEL_SCRIPT,"*%d: %s\n", i, &callScriptStackNames[MAX_QPATH * i]);
        }
        Com_Error(ERR_FATAL, "CallscriptStack overflowed");
        return 0;
    }

    ++callScriptStackPtr;

    handle = Scr_CreateCanonicalFilename(scriptname);

    if (FindVariable(scrCompilePub.loadedscripts, handle))
    {
        --callScriptStackPtr;

        SL_RemoveRefToString(handle);
        variable = FindVariable(scrCompilePub.scriptsPos, handle);

        if (variable)
        {
            return FindObject(variable);
        }
        return 0;
    }
    else
    {

        variable = GetNewVariable(scrCompilePub.loadedscripts, handle);

        SL_RemoveRefToString(handle);

        oldSourceBuf = scrParserPub.sourceBuf;

        /*
        Try to load our extended scriptfile (gsx) first.
        This allows to create mod- and mapscripts with our extended functionality
        while it is still possible to fall back to default script if our extended functionality is not available.
        */

        Com_sprintf(filepath, sizeof(filepath), "%s.gsx", SL_ConvertToString(handle));
        scr_buffer_handle = Scr_AddSourceBuffer(SL_ConvertToString(handle), filepath, TempMalloc(0), 1);
        if (!scr_buffer_handle)
        {
            /*
        If no extended script was found just load traditional script (gsc)
        */
            Com_sprintf(filepath, sizeof(filepath), "%s.gsc", SL_ConvertToString(handle));
            scr_buffer_handle = Scr_AddSourceBuffer(SL_ConvertToString(handle), filepath, TempMalloc(0), 1);
        }

        if (!scr_buffer_handle)
        {
            --callScriptStackPtr;
            return 0;
        }

        oldAnimTreeNames = scrAnimPub.animTreeNames;
        scrAnimPub.animTreeNames = 0;
        scrCompilePub.far_function_count = 0;
        Scr_InitAllocNode();
        oldFilename = scrParserPub.scriptfilename;
        scrParserPub.scriptfilename = filepath;
        scrCompilePub.in_ptr = "+";
        scrCompilePub.parseBuf = scr_buffer_handle;
        ScriptParse(&result, 0);

        object = SGetObjectA(GetVariable(scrCompilePub.scriptsPos, handle));

        ScriptCompile(result, object, variable, precache, iarg_02);

        scrParserPub.scriptfilename = oldFilename;
        scrParserPub.sourceBuf = oldSourceBuf;
        scrAnimPub.animTreeNames = oldAnimTreeNames;

        --callScriptStackPtr;

        return object;
    }
}

void Scr_Sys_Error_Wrapper(const char *fmt, ...)
{
    va_list argptr;
    char com_errorMessage[4096];

    va_start(argptr, fmt);
    Q_vsnprintf(com_errorMessage, sizeof(com_errorMessage), fmt, argptr);
    va_end(argptr);

    Com_Error(ERR_SCRIPT, "%s", com_errorMessage);
}

//Was only needed to extract the arrays
/*
void GetVirtualFunctionArray(){

    char buffer[1024*1024];
    char line[128];
    int i;
    char *funname;
    xfunction_t funaddr;
    int funtype;
    v_function_t *ptr;

    Com_Memset(buffer, 0, sizeof(buffer));

    for(i = 0, ptr = (v_function_t*)0x821c620; ptr->offset != NULL && i < 25; ptr++, i++){

    funname = ptr->name;
    funaddr = ptr->offset;
    funtype = ptr->developer;

    Com_sprintf(line, sizeof(line), "\t{\"%s\", (void*)%p, %x},\n", funname, funaddr, funtype);
    Q_strncat(buffer, sizeof(buffer), line);

    }

    FS_WriteFile("array.txt", buffer, strlen(buffer));
    Com_Quit_f();
}
*/

/*
void GetVirtualFunctionArray(){

    char buffer[1024*1024];
    char line[128];
    int i;
    char *funname;
    xfunction_t funaddr;
    int funtype;
    v_function_t *ptr;

    Com_Memset(buffer, 0, sizeof(buffer));

    for(i = 0, ptr = (v_function_t*)0x826e060; ptr->offset != NULL; ptr++, i++){

    funname = ptr->name;
    funaddr = ptr->offset;
    funtype = ptr->developer;

    Com_sprintf(line, sizeof(line), "\tScr_AddFunction(\"%s\", (void*)%p, %i);\n", funname, funaddr, funtype);
    Q_strncat(buffer, sizeof(buffer), line);

    }

    FS_WriteFile("array.txt", buffer, strlen(buffer));
    Com_Quit_f();
}

typedef struct{
    char *name;
    int offset;
    int bits;
    int zero;
}subnetlist_t;


typedef struct{
    subnetlist_t *sub;
    int size;
}netlist_t;




void GetDeltaEntArray(){

    char buffer[1024*1024];
    char line[128];
    netlist_t *ptr;
    subnetlist_t *subptr;
    int i, j;

    Com_Memset(buffer, 0, sizeof(buffer));

    for(j=0, ptr = (netlist_t*)0x82293c0; ptr->sub != NULL ; ptr++, j++){

    subptr = ptr->sub;
    Com_sprintf(line, sizeof(line), "netField_t entityStateFields_%i[] =\n{\n", j);
    Q_strncat(buffer, sizeof(buffer), line);

    for(i = 0; i < ptr->size; i++, subptr++)
    {
        Com_sprintf(line, sizeof(line), "\t{ NETF( %s ), %i, %i, %i},\n", subptr->name, subptr->offset, subptr->bits, subptr->zero);
        Q_strncat(buffer, sizeof(buffer), line);
    }

    Com_sprintf(line, sizeof(line), "};\n\n\n");
    Q_strncat(buffer, sizeof(buffer), line);

    }

    FS_WriteFile("array.txt", buffer, strlen(buffer));
    Com_Quit_f();
}



typedef struct{
    char *name;
    int offset;
    int bits;
    int unknown;
}netlistPlayer_t;


void GetDeltaPlayerArray(){

    char buffer[1024*1024];
    char line[128];
    netlistPlayer_t *ptr;
    int j;

    Com_Memset(buffer, 0, sizeof(buffer));

    for(j=0, ptr = (netlistPlayer_t*)0x82283c0; ptr->name != NULL; ptr++, j++){

    Com_sprintf(line, sizeof(line), "\tint\t\t%s; %i\n", ptr->name, ptr->offset);
    Q_strncat(buffer, sizeof(buffer), line);
    Com_Printf(CON_CHANNEL_SCRIPT,"%s\n", line);

    }

    FS_WriteFile("array.txt", buffer, strlen(buffer));
    Com_Quit_f();
}



typedef struct __attribute__((packed)){
    byte mov1;
    byte mov2;
    byte mov3;
    char* string;
    byte call1;
    byte call2;
    byte call3;
    byte call4;
    byte call5;
    byte mov11;
    byte mov12;
    short* index;
}stringindexcmd_t;


void StringIndexArray(){

    char buffer[1024*1024];
    char line[128];
    int j;
    stringindexcmd_t* ptr;


    Com_Memset(buffer, 0, sizeof(buffer));

    for(j=0, ptr = (stringindexcmd_t*)0x80a396e; ptr->mov3 != 0x90 ; ptr++, j++){

    Com_Printf(CON_CHANNEL_SCRIPT,"String: %s\n", ptr->string);
    Com_sprintf(line, sizeof(line), "\tshort   %s;\n", ptr->string);
    Q_strncat(buffer, sizeof(buffer), line);
    }

    FS_WriteFile("array.txt", buffer, strlen(buffer));
    Com_Quit_f();
}


void GetPlayerFieldArray(){

    char buffer[1024*1024];
    char line[128];
    scrClientFields_t *ptr;
    int j;

    Com_Memset(buffer, 0, sizeof(buffer));

    for(j=0, ptr = (scrClientFields_t*)0x8215780; ptr->name != NULL; ptr++, j++){

    Com_sprintf(line, sizeof(line), "\tScr_AddPlayerField(%s, %d, %d, %p, %p)\n", ptr->name, ptr->val1, ptr->val1, ptr->setfun, ptr->getfun);
    Q_strncat(buffer, sizeof(buffer), line);
    Com_Printf(CON_CHANNEL_SCRIPT,"%s\n", line);

    }

    FS_WriteFile("array.txt", buffer, strlen(buffer));
    Com_Quit_f();
}
*/
/*#include <string.h>
void GetEntityFieldArray()
{

    char buffer[1024 * 1024];
    char line[128];
    ent_field_t *ptr;
    int j;

    Com_Memset(buffer, 0, sizeof(buffer));

    Com_sprintf(line, sizeof(line), "entity_fields_t entityField[]");
    Q_strncat(buffer, sizeof(buffer), line);
    Com_Printf(CON_CHANNEL_SCRIPT,"%s\n", line);

    for (j = 0, ptr = (ent_field_t *)0x82202a0; ptr->name != NULL; ptr++, j++)
    {

        Com_sprintf(line, sizeof(line), "\t{ \"%s\", %d, %d, (void*)%p) },\n", ptr->name, ptr->val1, ptr->val2, ptr->setfun);
        Q_strncat(buffer, sizeof(buffer), line);
        Com_Printf(CON_CHANNEL_SCRIPT,"%s\n", line);
    }

    Com_sprintf(line, sizeof(line), "};\n");
    Q_strncat(buffer, sizeof(buffer), line);
    Com_Printf(CON_CHANNEL_SCRIPT,"%s\n", line);

    FS_WriteFile("array.txt", buffer, strlen(buffer));
    Com_Quit_f();
}
*/
/*
void GetHuffmanArray(){

    char buffer[1024*1024];
    char line[128];
    int j;
    byte* data;
    int *number;

    Com_Memset(buffer, 0, sizeof(buffer));



    if(0 < FS_ReadFile("cod_mac-bin.iwd", (void**)&data)){


        data += 0x387080;

        number = (int*)data;

        for(j=0; j < 256 ; j++){

        Com_sprintf(line, sizeof(line), "\t%d,\t\t\t//%d\n", number[j],j);
        Q_strncat(buffer, sizeof(buffer), line);

    }

    FS_WriteFile("array.txt", buffer, strlen(buffer));
    }
    Com_Quit_f();
}
*/
/*
int GetArraySize(int aHandle)
{
    int size = scrVarGlob.variables[aHandle].value.typeSize.size;
    return size;
}*/

/* only for debug */
/*
__regparm3 void VM_Notify_Hook(int entid, int constString, VariableValue *arguments)
{
    Com_Printf(CON_CHANNEL_SCRIPT,"^2Notify Entitynum: %d, EventString: %s\n", entid, SL_ConvertToString(constString));
    VM_Notify(entid, constString, arguments);
}*/

void Scr_InitSystem()
{
    scrVarPub.timeArrayId = AllocObject();
    scrVarPub.pauseArrayId = Scr_AllocArray();
    scrVarPub.levelId = AllocObject();
    scrVarPub.animId = AllocObject();
    scrVarPub.time = 0;
    g_script_error_level = -1;
}

void Scr_ClearArguments()
{
    while (scrVmPub.outparamcount)
    {
        RemoveRefToValue(scrVmPub.top->type, scrVmPub.top->u);
        --scrVmPub.top;
        --scrVmPub.outparamcount;
    }
}

void Scr_NotifyInternal(int varNum, int constString, int numArgs)
{
    VariableValue *curArg;
    int z;
    int ctype;

    Scr_ClearArguments();
    curArg = scrVmPub.top - numArgs;
    z = scrVmPub.inparamcount - numArgs;
    if (varNum)
    {
        ctype = curArg->type;
        curArg->type = 8;
        scrVmPub.inparamcount = 0;
        VM_Notify(varNum, constString, scrVmPub.top);
        curArg->type = ctype;
    }
    while (scrVmPub.top != curArg)
    {
        RemoveRefToValue(scrVmPub.top->type, scrVmPub.top->u);
        --scrVmPub.top;
    }
    scrVmPub.inparamcount = z;
}

void Scr_NotifyLevel(int constString, unsigned int numArgs)
{
    Scr_NotifyInternal(scrVarPub.levelId, constString, numArgs);
}

void Scr_NotifyNum(int entityNum, unsigned int entType, unsigned int constString, unsigned int numArgs)
{
    int entVarNum;

    entVarNum = FindEntityId(entityNum, entType);

    Scr_NotifyInternal(entVarNum, constString, numArgs);
}

void Scr_Notify(gentity_t *ent, unsigned short constString, unsigned int numArgs)
{
    Scr_NotifyNum(ent->s.number, 0, constString, numArgs);
}

void RuntimeError_Debug(char *msg, char *pos, int a4)
{
    int i;

    Com_Printf(CON_CHANNEL_SCRIPT,"\n^1******* script runtime error *******\n%s: ", msg);
    Scr_PrintPrevCodePos(0, pos, a4);
    if (scrVmPub.function_count)
    {
        for (i = scrVmPub.function_count - 1; i > 0; --i)
        {
            Com_Printf(CON_CHANNEL_SCRIPT,"^1called from:\n");
            Scr_PrintPrevCodePos(0, scrVmPub.function_frame_start[i].fs.pos, scrVmPub.function_frame_start[i].fs.localId == 0);
        }
        Com_Printf(CON_CHANNEL_SCRIPT,"^1started from:\n");
        Scr_PrintPrevCodePos(0, scrVmPub.function_frame_start[0].fs.pos, 1);
    }
    Com_Printf(CON_CHANNEL_SCRIPT,"^1************************************\n");
}

void RuntimeError(char *pos, int arg4, char *message, char *a4)
{
    int errtype;

    if (!scrVarPub.developer && !scrVmPub.terminal_error)
    {
        return;
    }

    if (scrVmPub.debugCode)
    {
        Com_Printf(CON_CHANNEL_SCRIPT,"%s\n", message);
        if (!scrVmPub.terminal_error)
        {
            return;
        }
    }
    else
    {
        RuntimeError_Debug(message, pos, arg4);
        if (!scrVmPub.abort_on_error && !scrVmPub.terminal_error)
        {
            return;
        }
    }

    if (scrVmPub.terminal_error)
    {
        errtype = 5;
    }
    else
    {
        errtype = 4;
    }

    if (a4)
    {
        Com_Error(errtype, "script runtime error\n(see console for details)\n%s\n%s", message, a4);
    }
    else
    {
        Com_Error(errtype, "script runtime error\n(see console for details)\n%s", message);
    }
}

qboolean Scr_ScriptRuntimecheckInfiniteLoop()
{
    int now = Sys_Milliseconds();

    if (now - scrVmGlob.starttime > 6000)
    {
        Cbuf_AddText("wait 50;map_rotate\n");
        return qtrue;
        //CPU is just busy
    }
    return qfalse;
}

gentity_t *VM_GetGEntityForNum(scr_entref_t num)
{
    if (HIWORD(num))
    {
        Scr_Error("Not an entity");
        return NULL;
    }

    return &g_entities[LOWORD(num)];
}

gclient_t *VM_GetGClientForEntity(gentity_t *ent)
{
    return ent->client;
}

gclient_t *VM_GetGClientForEntityNumber(scr_entref_t num)
{
    return VM_GetGClientForEntity(VM_GetGEntityForNum(num));
}

client_t *VM_GetClientForEntityNumber(scr_entref_t num)
{
    return &svs.clients[num];
}

/*
void __cdecl sub_51D1F0()
{
  if ( !scrVarPub.evaluate && !unk_1509088 )
  {
    if ( scrVarPub.developer && scrVmGlob.loading )
    {
      scrVmPub.terminal_error = 1;
    }
    if ( scrVmPub.function_count || scrVmPub.debugCode )
    {
      longjmp(g_script_error[g_script_error_level], -1);
    }
    Sys_Error("%s", scrVarPub.error_message);
  }
  if ( scrVmPub.terminal_error )
  {
    Sys_Error("%s", scrVarPub.error_message);
  }
}

void Scr_Error(const char *error)
{
  static char errormsg[1024];
  if ( !scrVarPub.error_message )
  {
    Q_strncpyz(errormsg, error, sizeof(errormsg));
    scrVarPub.error_message = errormsg;
  }
  sub_51D1F0();
}
*/
int Scr_GetInt(unsigned int paramnum)
{
    mvabuf;
    VariableValue *var;

    if (paramnum >= scrVmPub.outparamcount)
    {
        Scr_Error(va("parameter %d does not exist", paramnum + 1));
        return 0;
    }

    var = &scrVmPub.top[-paramnum];
    if (var->type == 6)
    {
        return var->u.intValue;
    }
    scrVarPub.error_index = paramnum + 1;
    Scr_Error(va("type %s is not an int", var_typename[var->type]));
    return 0;
}

unsigned int Scr_GetObject(unsigned int paramnum)
{
    mvabuf;
    VariableValue *var;

    if (paramnum >= scrVmPub.outparamcount)
    {
        Scr_Error(va("parameter %d does not exist", paramnum + 1));
        return 0;
    }

    var = &scrVmPub.top[-paramnum];
    if (var->type == 1)
    {
        return var->u.pointerValue;
    }
    scrVarPub.error_index = paramnum + 1;
    Scr_Error(va("type %s is not an object", var_typename[var->type]));
    return 0;
}


//Use with Scr_Exce(Ent)Thread
int Scr_GetFunc(unsigned int paramnum)
{
    mvabuf;
    VariableValue *var;

    if (paramnum >= scrVmPub.outparamcount)
    {
        Scr_Error(va("parameter %d does not exist", paramnum + 1));
        return -1;
    }

    var = &scrVmPub.top[-paramnum];
    if (var->type == 9)
    {
        int vmRomAddress = var->u.codePosValue - scrVarPub.programBuffer;
        return vmRomAddress;
    }
    scrVarPub.error_index = paramnum + 1;
    Scr_Error(va("type %s is not an function", var_typename[var->type]));
    return -1;
}



unsigned int Scr_GetArrayId(unsigned int paramnum, VariableValue** v, int maxvariables)
{
    unsigned int ptr = Scr_GetObject(paramnum);
    VariableValueInternal *var;

    unsigned int hash_id = 0;
    int i = 0;

    do
    {
        if(hash_id == 0)
        {
            hash_id = scrVarGlob.variableList[ptr + 1].hash.u.prevSibling;
            if(hash_id == 0)
            {
                return 0;
            }
        }else{
            hash_id = scrVarGlob.variableList[SCR_VARHIGH + var->hash.u.prevSibling].hash.id;
        }
        var = &scrVarGlob.variableList[SCR_VARHIGH + hash_id];

        int type = var->w.type & 0x1f;

        v[i]->type = type;
        v[i]->u = var->u.u;

        ++i;
    }
    while ( var->hash.u.prevSibling && scrVarGlob.variableList[SCR_VARHIGH + var->hash.u.prevSibling].hash.id && i < maxvariables);

    return 0;//GetArraySize(ptr);
}

void __cdecl Scr_TerminalError(const char *error/*, scriptInstance_t inst*/)
{
/*
  Scr_DumpScriptThreads(inst);
  Scr_DumpScriptVariablesDefault(inst);
  scrVmPub.terminal_error = 1;
  Scr_Error(inst, error, 0);
*/
  Scr_DumpScriptThreads( );
  Scr_DumpScriptVariablesDefault( );
  scrVmPub.terminal_error = 1;
  Scr_Error(error);

}



//Fix for weird GNU-GCC ABI
#if defined( __GNUC__ ) && !defined( __MINGW32__ )
static inline __attribute__((always_inline)) VariableValue __cdecl GetEntityFieldValueReal(unsigned int classnum, int entnum, int offset);

//For GCC
void __cdecl GetEntityFieldValue(unsigned int classnum, int entnum, int offset)
{
    VariableValue r;
    r = GetEntityFieldValueReal(classnum, entnum, offset);
    asm volatile (
        "mov 0x4(%%eax), %%edx\n"
        "mov 0x0(%%eax), %%eax\n"
        ::"eax" (&r)
    );
}

static inline __attribute__((always_inline)) VariableValue __cdecl GetEntityFieldValueReal(unsigned int classnum, int entnum, int offset)
#else
//For MSVC & MinGW
VariableValue __cdecl GetEntityFieldValue(unsigned int classnum, int entnum, int offset)
#endif
{
  VariableValue result;

  assert ( !scrVmPub.inparamcount );
  assert(!scrVmPub.outparamcount);

  scrVmPub.top = scrVmGlob.eval_stack - 1;
  scrVmGlob.eval_stack[0].type = 0;
  Scr_GetObjectField(classnum, entnum, offset);

  assert(!scrVmPub.inparamcount || scrVmPub.inparamcount == 1);
  assert(!scrVmPub.outparamcount);
  assert(scrVmPub.top - scrVmPub.inparamcount == scrVmGlob.eval_stack - 1);

  scrVmPub.inparamcount = 0;

  result.u.intValue = scrVmGlob.eval_stack[0].u.intValue;
  result.type = scrVmGlob.eval_stack[0].type;

  return result;
}

void __cdecl Scr_LocalizationError(int iParm, const char *pszErrorMessage)
{
  Scr_ParamError(iParm, pszErrorMessage);
}

void __cdecl Scr_ValidateLocalizedStringRef(int parmIndex, const char *token, int tokenLen)
{
  int charIter;

  assert( token != 0 );
  assert( tokenLen >= 0 );

  if ( tokenLen > 1 )
  {
    for ( charIter = 0; charIter < tokenLen; ++charIter )
    {
      if ( !isalnum(token[charIter]) && token[charIter] != '_' )
      {
        Scr_ParamError(parmIndex, va("Illegal localized string reference: %s must contain only alpha-numeric characters and underscores", token));
      }
    }
  }
}


void __cdecl Scr_ConstructMessageString(int firstParmIndex, int lastParmIndex, const char *errorContext, char *string, unsigned int stringLimit)
{
  unsigned int charIndex;
  unsigned int tokenLen;
  int type;
  gentity_t *ent;
  int parmIndex;
  char *token;
  unsigned int stringLen;

  stringLen = 0;
  for ( parmIndex = firstParmIndex; parmIndex <= lastParmIndex; ++parmIndex )
  {
    type = Scr_GetType(parmIndex);
    if ( type == 3 )
    {
      token = (char *)Scr_GetIString(parmIndex);
      tokenLen = strlen(token);
      Scr_ValidateLocalizedStringRef(parmIndex, token, tokenLen);
      if ( stringLen + tokenLen + 1 >= stringLimit )
      {
        Scr_ParamError(parmIndex, va("%s is too long. Max length is %i\n", errorContext, stringLimit));
      }
      if ( stringLen )
      {
        string[stringLen++] = 20;
      }
    }
    else if ( type != 1 || Scr_GetPointerType(parmIndex) != 19 )
    {
      token = (char *)Scr_GetString(parmIndex);
      tokenLen = strlen(token);
      for ( charIndex = 0; charIndex < tokenLen; ++charIndex )
      {
        if ( token[charIndex] == 20 || token[charIndex] == 21 || token[charIndex] == 22 )
        {
          Scr_ParamError(parmIndex, va("bad escape character (%i) present in string", token[charIndex]));
        }
        if ( isalpha(token[charIndex]) )
        {
          if ( loc_warnings->boolean )
          {
            if ( loc_warningsAsErrors->boolean )
            {
              Scr_LocalizationError(parmIndex, va("non-localized %s strings are not allowed to have letters in them: \"%s\"", errorContext, token));
            }
            else
            {
              Com_PrintWarning(CON_CHANNEL_PLAYERWEAP,
                "WARNING: Non-localized %s string is not allowed to have letters in it. Must be changed over to a localized string: \"%s\"\n",
                errorContext, token);
            }
          }
          break;
        }
      }
      if ( stringLen + tokenLen + 1 >= stringLimit )
      {
        Scr_ParamError(parmIndex, va("%s is too long. Max length is %i\n", errorContext, stringLimit));
      }
      if ( tokenLen )
      {
        string[stringLen++] = 21;
      }
    }
    else
    {
      ent = Scr_GetEntity(parmIndex);
      if ( !ent->client )
      {
        Scr_ParamError(parmIndex, "Entity is not a player");
      }
      token = va("%s^7", CS_DisplayName(&ent->client->sess.cs, 3));
      tokenLen = strlen(token);
      if ( stringLen + tokenLen + 1 >= stringLimit )
      {
        Scr_ParamError(parmIndex, va("%s is too long. Max length is %i\n", errorContext, stringLimit));
      }
      if ( tokenLen )
      {
        string[stringLen++] = 21;
      }
    }
    for ( charIndex = 0; charIndex < tokenLen; ++charIndex )
    {
      if ( token[charIndex] != 20 && token[charIndex] != 21 && token[charIndex] != 22 )
      {
        string[stringLen] = token[charIndex];
      }
      else
      {
        string[stringLen] = '.';
      }
      ++stringLen;
    }
  }
  string[stringLen] = 0;
}


void Scr_YYACError(const char* fmt, ...)
{
    va_list argptr;
    char com_errorMessage[4096];

    va_start(argptr, fmt);
    Q_vsnprintf(com_errorMessage, sizeof(com_errorMessage), fmt, argptr);
    va_end(argptr);

    Com_Error(ERR_SCRIPT, "%s", com_errorMessage);
}


void __cdecl Scr_ClearOutParams()
{
  while ( scrVmPub.outparamcount )
  {
    RemoveRefToValue(scrVmPub.top->type, scrVmPub.top->u);
    --scrVmPub.top;
    --scrVmPub.outparamcount;
  }
}

void __cdecl IncInParam()
{

  assert(((scrVmPub.top >= scrVmGlob.eval_stack - 1) && (scrVmPub.top <= scrVmGlob.eval_stack)) || ((scrVmPub.top >= scrVmPub.stack) && (scrVmPub.top <= scrVmPub.maxstack)));

  Scr_ClearOutParams( );

  if ( scrVmPub.top == scrVmPub.maxstack )
  {
    Sys_Error("Internal script stack overflow");
  }
  ++scrVmPub.top;
  ++scrVmPub.inparamcount;
  assert(((scrVmPub.top >= scrVmGlob.eval_stack) && (scrVmPub.top <= scrVmGlob.eval_stack + 1)) || ((scrVmPub.top >= scrVmPub.stack) && (scrVmPub.top <= scrVmPub.maxstack)));

}


void __cdecl Scr_AddString(const char *value)
{
  assert(value != NULL);

  IncInParam( );
  scrVmPub.top->type = VAR_STRING;
  scrVmPub.top->u.stringValue = SL_GetString(value, 0);
}

void __cdecl Scr_AddInt(int value)
{
  IncInParam( );
  scrVmPub.top->type = VAR_INTEGER;
  scrVmPub.top->u.intValue = value;
}

void __cdecl Scr_AddBool(bool value)
{
  assert(value == false || value == true);

  IncInParam( );
  scrVmPub.top->type = VAR_INTEGER;
  scrVmPub.top->u.intValue = value;
}

void __cdecl Scr_AddFloat(float value)
{
  IncInParam();
  scrVmPub.top->type = VAR_FLOAT;
  scrVmPub.top->u.floatValue = value;
}

void __cdecl Scr_AddAnim(struct scr_anim_s value)
{
  IncInParam();
  scrVmPub.top->type = VAR_ANIMATION;
  scrVmPub.top->u.codePosValue = value.linkPointer;
}

void __cdecl Scr_AddUndefined( )
{
  IncInParam();
  scrVmPub.top->type = VAR_UNDEFINED;
}

void __cdecl Scr_AddObject(unsigned int id)
{
  assert(id != 0);
  assert(Scr_GetObjectType( id ) != VAR_THREAD);
  assert(Scr_GetObjectType( id ) != VAR_NOTIFY_THREAD);
  assert(Scr_GetObjectType( id ) != VAR_TIME_THREAD);
  assert(Scr_GetObjectType( id ) != VAR_CHILD_THREAD);
  assert(Scr_GetObjectType( id ) != VAR_DEAD_THREAD);

  IncInParam();
  scrVmPub.top->type = VAR_POINTER;
  scrVmPub.top->u.intValue = id;
  AddRefToObject(id);
}

void __cdecl Scr_AddEntityNum(int entnum, unsigned int classnum)
{
  unsigned int entId;
  const char *varUsagePos;

  varUsagePos = scrVarPub.varUsagePos;
  if ( !scrVarPub.varUsagePos )
  {
    scrVarPub.varUsagePos = "<script entity variable>";
  }
  entId = Scr_GetEntityId(entnum, classnum);
  Scr_AddObject(entId);
  scrVarPub.varUsagePos = varUsagePos;
}

void __cdecl Scr_AddStruct( )
{
  unsigned int id;

  id = AllocObject();
  Scr_AddObject(id );
  RemoveRefToObject(id);
}

void __cdecl Scr_AddIString(const char *value)
{
  assert(value != NULL);

  IncInParam( );
  scrVmPub.top->type = VAR_ISTRING;
  scrVmPub.top->u.stringValue = SL_GetString(value, 0);
}

void __cdecl Scr_AddConstString(unsigned int value)
{
  assert(value != 0);

  IncInParam( );
  scrVmPub.top->type = VAR_STRING;
  scrVmPub.top->u.stringValue = value;
  SL_AddRefToString(value);
}


/*
float *__cdecl Scr_AllocVector(const float *v)
{
  float *avec;

  avec = Scr_AllocVectorInternal( );
  VectorCopy(v, avec);
  return avec;
}
*/

void __cdecl Scr_AddVector(const float *value)
{
  IncInParam();
  scrVmPub.top->type = VAR_VECTOR;
  scrVmPub.top->u.vectorValue = Scr_AllocVector(value);
}

void __cdecl Scr_MakeArray( )
{
  IncInParam( );
  scrVmPub.top->type = VAR_POINTER;
  scrVmPub.top->u.intValue = Scr_AllocArray( );
}

void __cdecl Scr_AddArray( )
{
  unsigned int arraySize;
  unsigned int id;
  const char *varUsagePos;

  varUsagePos = scrVarPub.varUsagePos;
  if ( !scrVarPub.varUsagePos )
  {
    scrVarPub.varUsagePos = "<script array variable>";
  }

  assert(scrVmPub.inparamcount);

  --scrVmPub.top;
  --scrVmPub.inparamcount;

  assert(scrVmPub.top->type == VAR_POINTER);

  arraySize = GetArraySize( scrVmPub.top->u.stringValue);
  id = GetNewArrayVariable( scrVmPub.top->u.stringValue, arraySize);
  SetNewVariableValue( id, scrVmPub.top + 1);
  scrVarPub.varUsagePos = varUsagePos;
}

void __cdecl Scr_AddArrayStringIndexed(unsigned int stringValue)
{
  unsigned int id;

  assert(scrVmPub.inparamcount != 0);

  --scrVmPub.top;
  --scrVmPub.inparamcount;
  assert(scrVmPub.top->type == VAR_POINTER);

  id = GetNewVariable( scrVmPub.top->u.stringValue, stringValue);
  SetNewVariableValue( id, scrVmPub.top + 1);
}
