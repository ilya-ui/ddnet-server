/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
#include "gamecontext.h"

#include <engine/antibot.h>
#include <engine/shared/config.h>

#include <game/server/entities/cannon.h>
#include <game/server/entities/character.h>
#include <game/server/entities/ferriswheel.h>
#include <game/server/entities/helicopter.h>
#include <game/server/entities/laserclock.h>
#include <game/server/entities/laserdoor.h>
#include <game/server/entities/lasertetris.h>
#include <game/server/entities/tank.h>
#include <game/server/entities/turret.h>
#include <game/server/entities/ufo.h>
#include <game/server/entities/vodkabottle.h>
#include <game/server/entities/snake.h>
#include <game/server/entities/snake_board.h>
#include <game/server/entities/spider.h>
#include <game/server/gamemodes/DDRace.h>
#include <game/server/player.h>
#include <game/server/save.h>
#include <game/server/teams.h>

void CGameContext::ConGoLeft(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Tiles = pResult->NumArguments() == 1 ? pResult->GetInteger(0) : 1;

	if(!CheckClientId(pResult->m_ClientId))
		return;
	pSelf->MoveCharacter(pResult->m_ClientId, -1 * Tiles, 0);
}

void CGameContext::ConGoRight(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Tiles = pResult->NumArguments() == 1 ? pResult->GetInteger(0) : 1;

	if(!CheckClientId(pResult->m_ClientId))
		return;
	pSelf->MoveCharacter(pResult->m_ClientId, Tiles, 0);
}

void CGameContext::ConGoDown(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Tiles = pResult->NumArguments() == 1 ? pResult->GetInteger(0) : 1;

	if(!CheckClientId(pResult->m_ClientId))
		return;
	pSelf->MoveCharacter(pResult->m_ClientId, 0, Tiles);
}

void CGameContext::ConGoUp(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Tiles = pResult->NumArguments() == 1 ? pResult->GetInteger(0) : 1;

	if(!CheckClientId(pResult->m_ClientId))
		return;
	pSelf->MoveCharacter(pResult->m_ClientId, 0, -1 * Tiles);
}

void CGameContext::ConMove(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;
	pSelf->MoveCharacter(pResult->m_ClientId, pResult->GetInteger(0),
		pResult->GetInteger(1));
}

void CGameContext::ConMoveRaw(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;
	pSelf->MoveCharacter(pResult->m_ClientId, pResult->GetInteger(0),
		pResult->GetInteger(1), true);
}

void CGameContext::MoveCharacter(int ClientId, int X, int Y, bool Raw)
{
	CCharacter *pChr = GetPlayerChar(ClientId);

	if(!pChr)
		return;

	pChr->Move(vec2((Raw ? 1 : 32) * X, (Raw ? 1 : 32) * Y));
	pChr->ResetVelocity();
	pChr->m_DDRaceState = ERaceState::CHEATED;
}

void CGameContext::ConKillPlayer(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;
	int Victim = pResult->GetVictim();

	if(pSelf->m_apPlayers[Victim])
	{
		pSelf->m_apPlayers[Victim]->KillCharacter(WEAPON_GAME);
		char aBuf[512];
		if(pResult->NumArguments() == 2)
			str_format(aBuf, sizeof(aBuf), "%s was killed by authorized player (%s)",
				pSelf->Server()->ClientName(Victim),
				pResult->GetString(1));
		else
			str_format(aBuf, sizeof(aBuf), "%s was killed by authorized player",
				pSelf->Server()->ClientName(Victim));
		pSelf->SendChat(-1, TEAM_ALL, aBuf);
	}
}

void CGameContext::ConNinja(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_NINJA, false);
}

void CGameContext::ConUnNinja(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_NINJA, true);
}

void CGameContext::ConTelekinesis(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;
	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientId);
	if(pPlayer && pChr)
	{
		pPlayer->m_HasTelekinesis = true;
		pChr->GiveNinja();
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "telekinesis", "Telekinesis enabled! Click on players to grab them.");
	}
}

void CGameContext::ConUnTelekinesis(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;
	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientId);
	if(pPlayer)
	{
		// Release any grabbed player
		if(pPlayer->m_TelekinesisVictim >= 0)
		{
			CPlayer *pVictimPlayer = pSelf->m_apPlayers[pPlayer->m_TelekinesisVictim];
			if(pVictimPlayer && pVictimPlayer->GetCharacter())
			{
				pVictimPlayer->GetCharacter()->UnFreeze();
			}
			pPlayer->m_TelekinesisVictim = -1;
		}
		pPlayer->m_HasTelekinesis = false;
		if(pChr)
		{
			pChr->RemoveNinja();
		}
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "telekinesis", "Telekinesis disabled.");
	}
}

void CGameContext::ConPaint(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;
	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(pPlayer)
	{
		pPlayer->m_PaintMode = true;
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "paint", "Paint mode enabled! Hold fire to draw lasers.");
	}
}

void CGameContext::ConUnPaint(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;
	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(pPlayer)
	{
		pPlayer->m_PaintMode = false;
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "paint", "Paint mode disabled.");
	}
}

void CGameContext::ConClearPaint(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ClearPaintLasers();
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "paint", "All paint lasers cleared!");
}

void CGameContext::ConFight(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	// Count active players
	int NumPlayers = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(pSelf->m_apPlayers[i] && pSelf->GetPlayerChar(i))
			NumPlayers++;
	}

	if(NumPlayers < 2)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fight", "Need at least 2 players to start fight!");
		return;
	}

	// Start fight event
	pSelf->m_FightActive = true;
	pSelf->m_FightStartTick = pSelf->Server()->Tick();

	// Reset participants
	for(int i = 0; i < MAX_CLIENTS; i++)
		pSelf->m_aFightParticipants[i] = false;

	// Teleport all players to spawn and freeze them
	pSelf->SendChat(-1, TEAM_ALL, "*** FIGHT EVENT STARTED! ***");
	pSelf->SendChat(-1, TEAM_ALL, "All players frozen for 3 seconds...");

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CPlayer *pPlayer = pSelf->m_apPlayers[i];
		CCharacter *pChr = pSelf->GetPlayerChar(i);
		if(pPlayer && pChr)
		{
			pSelf->m_aFightParticipants[i] = true;
			// Respawn player at spawn
			pPlayer->KillCharacter(WEAPON_GAME, false);
			pPlayer->Respawn();
		}
	}
}

void CGameContext::ConZombieEvent(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_ZombieEvent.Start();
}

void CGameContext::CheckFightWinner()
{
	if(!m_FightActive)
		return;

	// Check for frozen players and move them to spectator
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_aFightParticipants[i] && m_apPlayers[i])
		{
			CCharacter *pChr = GetPlayerChar(i);
			// If player is frozen (any type of freeze), eliminate them
			if(pChr && pChr->IsAlive())
			{
				bool IsFrozen = pChr->m_FreezeTime > 0 || pChr->Core()->m_DeepFrozen || pChr->Core()->m_LiveFrozen;
				if(IsFrozen)
				{
					// Move to spectator
					char aBuf[128];
					str_format(aBuf, sizeof(aBuf), "%s was eliminated!", Server()->ClientName(i));
					SendChat(-1, TEAM_ALL, aBuf);
					m_aFightParticipants[i] = false; // No longer a participant
					m_apPlayers[i]->SetTeam(TEAM_SPECTATORS, false);
				}
			}
			else if(!pChr)
			{
				// Player died or disconnected
				m_aFightParticipants[i] = false;
			}
		}
	}

	// Count alive participants
	int AliveCount = 0;
	int LastAliveId = -1;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_aFightParticipants[i])
		{
			CCharacter *pChr = GetPlayerChar(i);
			if(pChr && pChr->IsAlive())
			{
				AliveCount++;
				LastAliveId = i;
			}
		}
	}

	// Check if we have a winner
	if(AliveCount <= 1 && LastAliveId >= 0)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "*** %s WON THE FIGHT! ***", Server()->ClientName(LastAliveId));
		SendChat(-1, TEAM_ALL, aBuf);
		m_FightActive = false;

		// Unfreeze all spectating players
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(m_aFightParticipants[i] && m_apPlayers[i])
			{
				m_apPlayers[i]->Respawn();
			}
		}
	}
	else if(AliveCount == 0)
	{
		SendChat(-1, TEAM_ALL, "*** FIGHT ENDED - NO WINNER! ***");
		m_FightActive = false;
	}
}

void CGameContext::ClearPaintLasers()
{
	// Remove all paint laser entities
	CEntity *pEnt = m_World.FindFirst(CGameWorld::ENTTYPE_LASER);
	while(pEnt)
	{
		CEntity *pNext = pEnt->TypeNext();
		// Check if it's a paint laser by checking if it has no energy (paint lasers are static)
		pEnt->Reset();
		pEnt = pNext;
	}
}

void CGameContext::SaveJailByAddr(int ClientId)
{
	if(!m_apPlayers[ClientId] || !m_apPlayers[ClientId]->m_InJail)
		return;

	const NETADDR *pAddr = Server()->ClientAddr(ClientId);
	int SecondsLeft = (m_apPlayers[ClientId]->m_JailEndTick - Server()->Tick()) / Server()->TickSpeed();
	if(SecondsLeft <= 0)
		return;

	// Check if already exists, update it
	for(int i = 0; i < m_NumJailEntries; i++)
	{
		if(net_addr_comp_noport(&m_aJailEntries[i].m_Addr, pAddr) == 0)
		{
			m_aJailEntries[i].m_SecondsLeft = SecondsLeft;
			return;
		}
	}

	// Add new entry
	if(m_NumJailEntries < MAX_CLIENTS)
	{
		m_aJailEntries[m_NumJailEntries].m_Addr = *pAddr;
		m_aJailEntries[m_NumJailEntries].m_SecondsLeft = SecondsLeft;
		m_NumJailEntries++;
	}
}

int CGameContext::GetJailByAddr(int ClientId)
{
	const NETADDR *pAddr = Server()->ClientAddr(ClientId);

	for(int i = 0; i < m_NumJailEntries; i++)
	{
		if(net_addr_comp_noport(&m_aJailEntries[i].m_Addr, pAddr) == 0)
		{
			int SecondsLeft = m_aJailEntries[i].m_SecondsLeft;
			// Remove entry
			m_aJailEntries[i] = m_aJailEntries[m_NumJailEntries - 1];
			m_NumJailEntries--;
			return SecondsLeft;
		}
	}
	return 0;
}

void CGameContext::ConSetJail(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientId);
	if(pChr)
	{
		pSelf->m_JailPos = pChr->GetPos();
		pSelf->m_JailSet = true;
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "jail", "Jail position set at your current location!");
	}
}

void CGameContext::ConJail(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	if(!pSelf->m_JailSet)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "jail", "Jail position not set! Use 'setjail' first.");
		return;
	}

	int Victim = pResult->GetVictim();
	int Minutes = pResult->NumArguments() > 1 ? pResult->GetInteger(1) : 5; // Default 5 minutes

	if(Minutes < 1)
		Minutes = 1;
	if(Minutes > 60)
		Minutes = 60;

	CPlayer *pPlayer = pSelf->m_apPlayers[Victim];
	if(!pPlayer)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "jail", "Player not found!");
		return;
	}

	pPlayer->m_InJail = true;
	pPlayer->m_JailEndTick = pSelf->Server()->Tick() + Minutes * 60 * pSelf->Server()->TickSpeed();

	// Teleport to jail
	CCharacter *pChr = pPlayer->GetCharacter();
	if(pChr)
	{
		pSelf->Teleport(pChr, pSelf->m_JailPos);
		pChr->Freeze(Minutes * 60); // Freeze for the duration
	}

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%s was jailed for %d minute(s)!", pSelf->Server()->ClientName(Victim), Minutes);
	pSelf->SendChat(-1, TEAM_ALL, aBuf);
}

void CGameContext::ConUnjail(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	int Victim = pResult->GetVictim();
	CPlayer *pPlayer = pSelf->m_apPlayers[Victim];
	if(!pPlayer)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "jail", "Player not found!");
		return;
	}

	pPlayer->m_InJail = false;
	pPlayer->m_JailEndTick = 0;

	CCharacter *pChr = pPlayer->GetCharacter();
	if(pChr)
	{
		pChr->UnFreeze();
	}

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%s was released from jail!", pSelf->Server()->ClientName(Victim));
	pSelf->SendChat(-1, TEAM_ALL, aBuf);
}

void CGameContext::ConHelicopter(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	
	// Get target ID - use provided ID or caller's ID
	int TargetId = pResult->NumArguments() > 0 ? pResult->GetVictim() : pResult->m_ClientId;
	if(!CheckClientId(TargetId))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[TargetId];
	CCharacter *pChr = pSelf->GetPlayerChar(TargetId);
	if(!pPlayer || !pChr)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "helicopter", "Target needs to be alive!");
		return;
	}

	// If already in helicopter, exit
	if(pPlayer->m_InHelicopter && pPlayer->m_pHelicopter)
	{
		pPlayer->m_pHelicopter->RemovePilot();
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "helicopter", "Exited helicopter!");
		return;
	}

	// Spawn helicopter and enter immediately
	vec2 Pos = pChr->GetPos();
	CHelicopter *pHeli = new CHelicopter(&pSelf->m_World, Pos, TargetId);
	pHeli->SetPilot(TargetId);

	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "helicopter", "Helicopter! Move cursor to fly. Press K to exit.");
}

void CGameContext::ConUfo(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	
	// Get target ID - use provided ID or caller's ID
	int TargetId = pResult->NumArguments() > 0 ? pResult->GetVictim() : pResult->m_ClientId;
	if(!CheckClientId(TargetId))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[TargetId];
	CCharacter *pChr = pSelf->GetPlayerChar(TargetId);
	if(!pPlayer || !pChr)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ufo", "Target needs to be alive!");
		return;
	}

	// If already in UFO, exit
	if(pPlayer->m_InUfo && pPlayer->m_pUfo)
	{
		pPlayer->m_pUfo->RemovePilot();
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ufo", "Exited UFO!");
		return;
	}

	// Spawn UFO and enter immediately
	vec2 Pos = pChr->GetPos();
	CUfo *pUfo = new CUfo(&pSelf->m_World, Pos, TargetId);
	pUfo->SetPilot(TargetId);

	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ufo", "UFO! Move cursor to fly, LMB to abduct players below. Press K to exit.");
}

void CGameContext::ConClearVehicles(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	
	int Count = 0;
	
	// Remove all helicopters
	CEntity *pEnt = pSelf->m_World.FindFirst(CGameWorld::ENTTYPE_HELICOPTER);
	while(pEnt)
	{
		CEntity *pNext = pEnt->TypeNext();
		pEnt->Reset();
		Count++;
		pEnt = pNext;
	}
	
	// Remove all UFOs
	pEnt = pSelf->m_World.FindFirst(CGameWorld::ENTTYPE_UFO);
	while(pEnt)
	{
		CEntity *pNext = pEnt->TypeNext();
		pEnt->Reset();
		Count++;
		pEnt = pNext;
	}
	
	// Remove all tanks
	pEnt = pSelf->m_World.FindFirst(CGameWorld::ENTTYPE_TANK);
	while(pEnt)
	{
		CEntity *pNext = pEnt->TypeNext();
		pEnt->Reset();
		Count++;
		pEnt = pNext;
	}
	
	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "Removed %d vehicles.", Count);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "vehicles", aBuf);
}

void CGameContext::ConTank(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	
	// Get target ID - use provided ID or caller's ID
	int TargetId = pResult->NumArguments() > 0 ? pResult->GetVictim() : pResult->m_ClientId;
	if(!CheckClientId(TargetId))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[TargetId];
	CCharacter *pChr = pSelf->GetPlayerChar(TargetId);
	if(!pPlayer || !pChr)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tank", "Target needs to be alive!");
		return;
	}

	// If already in tank, exit
	if(pPlayer->m_InTank && pPlayer->m_pTank)
	{
		pPlayer->m_pTank->RemovePilot();
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tank", "Exited tank!");
		return;
	}

	// Spawn tank and enter immediately
	vec2 Pos = pChr->GetPos();
	CTank *pTank = new CTank(&pSelf->m_World, Pos, TargetId);
	pTank->SetPilot(TargetId);

	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tank", "Tank! A/D to move, aim with cursor, LMB to fire grenades. Press K to exit.");
}

void CGameContext::ConVodka(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	
	// Get target ID - use provided ID or caller's ID
	int TargetId = pResult->NumArguments() > 0 ? pResult->GetVictim() : pResult->m_ClientId;
	if(!CheckClientId(TargetId))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[TargetId];
	CCharacter *pChr = pSelf->GetPlayerChar(TargetId);
	if(!pPlayer || !pChr)
		return;

	pPlayer->m_HasVodka = true;
	pSelf->SendChatTarget(TargetId, "You got vodka! LMB to drink, F3 to drop.");
}

void CGameContext::ConDropVodka(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	
	// Get target ID - use provided ID or caller's ID
	int TargetId = pResult->NumArguments() > 0 ? pResult->GetVictim() : pResult->m_ClientId;
	if(!CheckClientId(TargetId))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[TargetId];
	CCharacter *pChr = pSelf->GetPlayerChar(TargetId);
	if(!pPlayer || !pChr)
		return;

	if(!pPlayer->m_HasVodka)
	{
		pSelf->SendChatTarget(TargetId, "You don't have vodka!");
		return;
	}

	// Drop vodka bottle
	pPlayer->m_HasVodka = false;
	new CVodkaBottle(&pSelf->m_World, pChr->GetPos());
	pSelf->SendChatTarget(TargetId, "You dropped the vodka bottle.");
}

void CGameContext::ConSober(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	
	// Get target ID - use provided ID or caller's ID
	int TargetId = pResult->NumArguments() > 0 ? pResult->GetVictim() : pResult->m_ClientId;
	if(!CheckClientId(TargetId))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[TargetId];
	if(!pPlayer)
		return;

	pPlayer->m_HasVodka = false;
	pPlayer->m_DrunkLevel = 0;
	pSelf->SendChatTarget(TargetId, "You are now sober.");
}

void CGameContext::ConCannon(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	
	// Get target ID - use provided ID or caller's ID
	int TargetId = pResult->NumArguments() > 0 ? pResult->GetVictim() : pResult->m_ClientId;
	if(!CheckClientId(TargetId))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[TargetId];
	CCharacter *pChr = pSelf->GetPlayerChar(TargetId);
	if(!pPlayer || !pChr)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "cannon", "Target needs to be alive!");
		return;
	}

	// If already has cannon, remove it
	if(pPlayer->m_HasCannon && pPlayer->m_pCannon)
	{
		pPlayer->m_pCannon->Reset();
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "cannon", "Cannon removed!");
		return;
	}

	// Spawn cannon
	vec2 Pos = pChr->GetPos();
	CCannon *pCannon = new CCannon(&pSelf->m_World, Pos, TargetId);
	pCannon->SetOwner(TargetId);

	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "cannon", "CANNON! RMB to suck players, LMB to fire them! Use /uncannon to remove.");
}

void CGameContext::ConUnCannon(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	
	// Get target ID - use provided ID or caller's ID
	int TargetId = pResult->NumArguments() > 0 ? pResult->GetVictim() : pResult->m_ClientId;
	if(!CheckClientId(TargetId))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[TargetId];
	if(!pPlayer)
		return;

	if(pPlayer->m_HasCannon && pPlayer->m_pCannon)
	{
		pPlayer->m_pCannon->Reset();
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "cannon", "Cannon removed!");
	}
	else
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "cannon", "Target doesn't have a cannon!");
	}
}

void CGameContext::ConSpider(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	
	// Get target ID - use provided ID or caller's ID
	int TargetId = pResult->NumArguments() > 0 ? pResult->GetVictim() : pResult->m_ClientId;
	if(!CheckClientId(TargetId))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[TargetId];
	CCharacter *pChr = pSelf->GetPlayerChar(TargetId);
	if(!pPlayer || !pChr)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "spider", "Target needs to be alive!");
		return;
	}

	// Spawn spider (hammer to enter/exit)
	vec2 Pos = pChr->GetPos() + vec2(80, 0);
	new CSpider(&pSelf->m_World, Pos, TargetId);

	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "spider", "Spider spawned! Hammer it to enter/exit.");
}

void CGameContext::ConBigHammer(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	
	// Get target ID - use provided ID or caller's ID
	int TargetId = pResult->NumArguments() > 0 ? pResult->GetVictim() : pResult->m_ClientId;
	if(!CheckClientId(TargetId))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[TargetId];
	if(!pPlayer)
		return;

	pPlayer->m_HasBigHammer = true;
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "bighammer", "BigHammer enabled! Hammer now creates a giant laser effect.");
}

void CGameContext::ConUnBigHammer(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	
	// Get target ID - use provided ID or caller's ID
	int TargetId = pResult->NumArguments() > 0 ? pResult->GetVictim() : pResult->m_ClientId;
	if(!CheckClientId(TargetId))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[TargetId];
	if(!pPlayer)
		return;

	pPlayer->m_HasBigHammer = false;
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "bighammer", "BigHammer disabled.");
}

void CGameContext::ConEars(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(!pPlayer)
		return;

	pPlayer->m_HasEars = !pPlayer->m_HasEars;
	if(pPlayer->m_HasEars)
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ears", "Cat ears enabled! Meow~");
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ears", "Cat ears disabled.");
}

void CGameContext::ConSnake(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int ClientId = pResult->m_ClientId;
	CCharacter *pChr = pSelf->GetPlayerChar(ClientId);
	if(!pChr || !pChr->IsAlive())
		return;

	// Remove existing board if any
	CSnakeBoard *pSnake = (CSnakeBoard *)pSelf->m_World.FindFirst(CGameWorld::ENTTYPE_SNAKE);
	
	new CSnakeBoard(&pSelf->m_World, pChr->m_Pos + vec2(0, -200), ClientId);
	pSelf->SendChatTarget(ClientId, "Snake Game Started! Hit the board with Hammer to Reset.");
}

void CGameContext::ConUnSnake(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CSnakeBoard *pSnake = (CSnakeBoard *)pSelf->m_World.FindFirst(CGameWorld::ENTTYPE_SNAKE);
    int Count = 0;
	while(pSnake)
	{
		pSnake->Reset(); // Mark for destroy
        Count++;
		pSnake = (CSnakeBoard *)pSnake->TypeNext();
	}
	
    char aBuf[128];
    str_format(aBuf, sizeof(aBuf), "Removed %d Snake Boards.", Count);
	pSelf->SendChatTarget(pResult->m_ClientId, aBuf);
}


void CGameContext::ConFerrisWheel(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientId);
	if(!pChr)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ferriswheel", "You need to be alive!");
		return;
	}

	vec2 Pos = pChr->GetPos();
	new CFerrisWheel(&pSelf->m_World, Pos);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ferriswheel", "Ferris wheel spawned! Hammer seats to enter/exit.");
}

void CGameContext::ConTurret(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientId);
	if(!pChr)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "turret", "You need to be alive!");
		return;
	}

	vec2 Pos = pChr->GetPos();
	new CTurret(&pSelf->m_World, Pos, pResult->m_ClientId);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "turret", "Turret spawned! Hammer it to enter, aim with mouse, LMB to fire lasers.");
}

void CGameContext::ConClock(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientId);
	if(!pChr)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "clock", "You need to be alive!");
		return;
	}

	vec2 Pos = pChr->GetPos() + vec2(50, -100);
	new CLaserClock(&pSelf->m_World, Pos, pResult->m_ClientId);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "clock", "Laser clock spawned! Shows real time HH:MM:SS.");
}

void CGameContext::ConDoor(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientId);
	if(!pPlayer || !pChr)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "door", "You need to be alive!");
		return;
	}

	vec2 CursorPos = pChr->GetPos() + vec2(pChr->Core()->m_Input.m_TargetX, pChr->Core()->m_Input.m_TargetY);

	if(!pPlayer->m_CreatingDoor)
	{
		pPlayer->m_CreatingDoor = true;
		pPlayer->m_DoorPoint1 = CursorPos;
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "door", "First point set! Use /door again to set second point.");
	}
	else
	{
		pPlayer->m_CreatingDoor = false;
		new CLaserDoor(&pSelf->m_World, pPlayer->m_DoorPoint1, CursorPos, pResult->m_ClientId);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "door", "Door created! Hammer it to open/close (only you can).");
	}
}

void CGameContext::ConClearDoors(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Count = 0;
	CEntity *pEnt = pSelf->m_World.FindFirst(CGameWorld::ENTTYPE_LASER);
	while(pEnt)
	{
		CEntity *pNext = pEnt->TypeNext();
		CLaserDoor *pDoor = dynamic_cast<CLaserDoor *>(pEnt);
		if(pDoor)
		{
			pDoor->Reset();
			Count++;
		}
		pEnt = pNext;
	}
	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "Removed %d door(s).", Count);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "door", aBuf);
}

void CGameContext::ConEndlessHook(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;
	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientId);
	if(pChr)
	{
		pChr->SetEndlessHook(true);
	}
}

void CGameContext::ConUnEndlessHook(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;
	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientId);
	if(pChr)
	{
		pChr->SetEndlessHook(false);
	}
}

void CGameContext::ConSuper(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	// If ID provided, give super to that player
	int TargetId = pResult->m_ClientId;
	if(pResult->NumArguments() >= 1)
	{
		TargetId = pResult->GetInteger(0);
		if(TargetId < 0 || TargetId >= MAX_CLIENTS || !pSelf->m_apPlayers[TargetId])
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "super", "Invalid player ID");
			return;
		}
	}

	CCharacter *pChr = pSelf->GetPlayerChar(TargetId);
	if(pChr && !pChr->IsSuper())
	{
		pChr->SetSuper(true);
		pChr->UnFreeze();
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "Super enabled for %s", pSelf->Server()->ClientName(TargetId));
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "super", aBuf);
	}
}

void CGameContext::ConUnSuper(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	// If ID provided, remove super from that player
	int TargetId = pResult->m_ClientId;
	if(pResult->NumArguments() >= 1)
	{
		TargetId = pResult->GetInteger(0);
		if(TargetId < 0 || TargetId >= MAX_CLIENTS || !pSelf->m_apPlayers[TargetId])
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "super", "Invalid player ID");
			return;
		}
	}

	CCharacter *pChr = pSelf->GetPlayerChar(TargetId);
	if(pChr && pChr->IsSuper())
	{
		pChr->SetSuper(false);
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "Super disabled for %s", pSelf->Server()->ClientName(TargetId));
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "super", aBuf);
	}
}

void CGameContext::ConToggleInvincible(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientId);
	if(pChr)
		pChr->SetInvincible(pResult->NumArguments() == 0 ? !pChr->Core()->m_Invincible : pResult->GetInteger(0));
}

void CGameContext::ConSolo(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;
	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientId);
	if(pChr)
		pChr->SetSolo(true);
}

void CGameContext::ConUnSolo(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;
	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientId);
	if(pChr)
		pChr->SetSolo(false);
}

void CGameContext::ConFreeze(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;
	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientId);
	if(pChr)
		pChr->Freeze();
}

void CGameContext::ConUnFreeze(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;
	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientId);
	if(pChr)
		pChr->UnFreeze();
}

void CGameContext::ConDeep(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;
	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientId);
	if(pChr)
		pChr->SetDeepFrozen(true);
}

void CGameContext::ConUnDeep(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;
	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientId);
	if(pChr)
	{
		pChr->SetDeepFrozen(false);
		pChr->UnFreeze();
	}
}

void CGameContext::ConLiveFreeze(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;
	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientId);
	if(pChr)
		pChr->SetLiveFrozen(true);
}

void CGameContext::ConUnLiveFreeze(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;
	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientId);
	if(pChr)
		pChr->SetLiveFrozen(false);
}

void CGameContext::ConShotgun(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_SHOTGUN, false);
}

void CGameContext::ConGrenade(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_GRENADE, false);
}

void CGameContext::ConLaser(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_LASER, false);
}

void CGameContext::ConJetpack(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientId);
	if(pChr)
		pChr->SetJetpack(true);
}

void CGameContext::ConEndlessJump(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientId);
	if(pChr)
		pChr->SetEndlessJump(true);
}

void CGameContext::ConSetJumps(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientId);
	if(pChr)
		pChr->SetJumps(pResult->GetInteger(0));
}

void CGameContext::ConWeapons(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, -1, false);
}

void CGameContext::ConUnShotgun(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_SHOTGUN, true);
}

void CGameContext::ConUnGrenade(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_GRENADE, true);
}

void CGameContext::ConUnLaser(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_LASER, true);
}

void CGameContext::ConUnJetpack(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientId);
	if(pChr)
		pChr->SetJetpack(false);
}

void CGameContext::ConUnEndlessJump(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientId);
	if(pChr)
		pChr->SetEndlessJump(false);
}

void CGameContext::ConUnWeapons(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, -1, true);
}

void CGameContext::ConAddWeapon(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, pResult->GetInteger(0), false);
}

void CGameContext::ConRemoveWeapon(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, pResult->GetInteger(0), true);
}

void CGameContext::ModifyWeapons(IConsole::IResult *pResult, void *pUserData,
	int Weapon, bool Remove)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CCharacter *pChr = GetPlayerChar(pResult->m_ClientId);
	if(!pChr)
		return;

	if(std::clamp(Weapon, -1, NUM_WEAPONS - 1) != Weapon)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
			"invalid weapon id");
		return;
	}

	if(Weapon == -1)
	{
		pChr->GiveWeapon(WEAPON_SHOTGUN, Remove);
		pChr->GiveWeapon(WEAPON_GRENADE, Remove);
		pChr->GiveWeapon(WEAPON_LASER, Remove);
	}
	else
	{
		pChr->GiveWeapon(Weapon, Remove);
	}

	pChr->m_DDRaceState = ERaceState::CHEATED;
}

void CGameContext::Teleport(CCharacter *pChr, vec2 Pos)
{
	pChr->SetPosition(Pos);
	pChr->m_Pos = Pos;
	pChr->m_PrevPos = Pos;
	pChr->m_DDRaceState = ERaceState::CHEATED;
}

void CGameContext::ConToTeleporter(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	unsigned int TeleTo = pResult->GetInteger(0);

	if(!pSelf->Collision()->TeleOuts(TeleTo - 1).empty())
	{
		CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientId);
		if(pChr)
		{
			int TeleOut = pSelf->m_World.m_Core.RandomOr0(pSelf->Collision()->TeleOuts(TeleTo - 1).size());
			pSelf->Teleport(pChr, pSelf->Collision()->TeleOuts(TeleTo - 1)[TeleOut]);
		}
	}
}

void CGameContext::ConToCheckTeleporter(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	unsigned int TeleTo = pResult->GetInteger(0);

	if(!pSelf->Collision()->TeleCheckOuts(TeleTo - 1).empty())
	{
		CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientId);
		if(pChr)
		{
			int TeleOut = pSelf->m_World.m_Core.RandomOr0(pSelf->Collision()->TeleCheckOuts(TeleTo - 1).size());
			pSelf->Teleport(pChr, pSelf->Collision()->TeleCheckOuts(TeleTo - 1)[TeleOut]);
			pChr->m_TeleCheckpoint = TeleTo;
		}
	}
}

void CGameContext::ConTeleport(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Tele = pResult->NumArguments() == 2 ? pResult->GetInteger(0) : pResult->m_ClientId;
	int TeleTo = pResult->NumArguments() ? pResult->GetInteger(pResult->NumArguments() - 1) : pResult->m_ClientId;
	int AuthLevel = pSelf->Server()->GetAuthedState(pResult->m_ClientId);

	if(Tele != pResult->m_ClientId && AuthLevel < g_Config.m_SvTeleOthersAuthLevel)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tele", "you aren't allowed to tele others");
		return;
	}

	CCharacter *pChr = pSelf->GetPlayerChar(Tele);
	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];

	if(pChr && pPlayer && pSelf->GetPlayerChar(TeleTo))
	{
		// default to view pos when character is not available
		vec2 Pos = pPlayer->m_ViewPos;
		if(pResult->NumArguments() == 0 && !pPlayer->IsPaused() && pChr->IsAlive())
		{
			vec2 Target = vec2(pChr->Core()->m_Input.m_TargetX, pChr->Core()->m_Input.m_TargetY);
			Pos = pPlayer->m_CameraInfo.ConvertTargetToWorld(pChr->GetPos(), Target);
		}
		pSelf->Teleport(pChr, Pos);
		pChr->ResetJumps();
		pChr->UnFreeze();
		pChr->SetVelocity(vec2(0, 0));
	}
}

void CGameContext::ConKill(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;
	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];

	if(!pPlayer || (pPlayer->m_LastKill && pPlayer->m_LastKill + pSelf->Server()->TickSpeed() * g_Config.m_SvKillDelay > pSelf->Server()->Tick()))
		return;

	// If in vehicle, exit instead of killing
	if(pPlayer->m_InHelicopter && pPlayer->m_pHelicopter)
	{
		pPlayer->m_pHelicopter->RemovePilot();
		pSelf->SendChatTarget(pResult->m_ClientId, "Exited helicopter!");
		pPlayer->m_LastKill = pSelf->Server()->Tick();
		return;
	}
	if(pPlayer->m_InUfo && pPlayer->m_pUfo)
	{
		pPlayer->m_pUfo->RemovePilot();
		pSelf->SendChatTarget(pResult->m_ClientId, "Exited UFO!");
		pPlayer->m_LastKill = pSelf->Server()->Tick();
		return;
	}
	if(pPlayer->m_InTank && pPlayer->m_pTank)
	{
		pPlayer->m_pTank->RemovePilot();
		pSelf->SendChatTarget(pResult->m_ClientId, "Exited tank!");
		pPlayer->m_LastKill = pSelf->Server()->Tick();
		return;
	}

	pPlayer->m_LastKill = pSelf->Server()->Tick();
	pPlayer->KillCharacter(WEAPON_SELF);
}

void CGameContext::ConForcePause(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();
	int Seconds = 0;
	if(pResult->NumArguments() > 1)
		Seconds = std::clamp(pResult->GetInteger(1), 0, 360);

	CPlayer *pPlayer = pSelf->m_apPlayers[Victim];
	if(!pPlayer)
		return;

	pPlayer->ForcePause(Seconds);
}

void CGameContext::ConModerate(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	bool HadModerator = pSelf->PlayerModerating();

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	pPlayer->m_Moderating = !pPlayer->m_Moderating;

	if(!HadModerator && pPlayer->m_Moderating)
		pSelf->SendChat(-1, TEAM_ALL, "Server kick/spec votes will now be actively moderated.", 0);

	if(!pSelf->PlayerModerating())
		pSelf->SendChat(-1, TEAM_ALL, "Server kick/spec votes are no longer actively moderated.", 0);

	if(pPlayer->m_Moderating)
		pSelf->SendChatTarget(pResult->m_ClientId, "Active moderator mode enabled for you.");
	else
		pSelf->SendChatTarget(pResult->m_ClientId, "Active moderator mode disabled for you.");
}

void CGameContext::ConSetDDRTeam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	auto *pController = pSelf->m_pController;

	if(g_Config.m_SvTeam == SV_TEAM_FORBIDDEN || g_Config.m_SvTeam == SV_TEAM_FORCED_SOLO)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join",
			"Teams are disabled");
		return;
	}

	int Target = pResult->GetVictim();
	int Team = pResult->GetInteger(1);

	if(!pController->Teams().IsValidTeamNumber(Team))
		return;

	CCharacter *pChr = pSelf->GetPlayerChar(Target);

	if((pSelf->GetDDRaceTeam(Target) && pController->Teams().GetDDRaceState(pSelf->m_apPlayers[Target]) == ERaceState::STARTED) || (pChr && pController->Teams().IsPractice(pChr->Team())))
		pSelf->m_apPlayers[Target]->KillCharacter(WEAPON_GAME);

	pController->Teams().SetForceCharacterTeam(Target, Team);
	pController->Teams().SetTeamLock(Team, true);
}

void CGameContext::ConUninvite(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	auto *pController = pSelf->m_pController;

	pController->Teams().SetClientInvited(pResult->GetInteger(1), pResult->GetVictim(), false);
}

void CGameContext::ConVoteNo(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	pSelf->ForceVote(pResult->m_ClientId, false);
}

void CGameContext::ConDrySave(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];

	if(!pPlayer || !pSelf->Server()->IsRconAuthedAdmin(pResult->m_ClientId))
		return;

	CSaveTeam SavedTeam;
	int Team = pSelf->GetDDRaceTeam(pResult->m_ClientId);
	ESaveResult Result = SavedTeam.Save(pSelf, Team, true);
	if(CSaveTeam::HandleSaveError(Result, pResult->m_ClientId, pSelf))
		return;

	char aTimestamp[32];
	str_timestamp(aTimestamp, sizeof(aTimestamp));
	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "%s_%s_%s.save", pSelf->Server()->GetMapName(), aTimestamp, pSelf->Server()->GetAuthName(pResult->m_ClientId));
	IOHANDLE File = pSelf->Storage()->OpenFile(aBuf, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!File)
		return;

	int Len = str_length(SavedTeam.GetString());
	io_write(File, SavedTeam.GetString(), Len);
	io_close(File);
}

void CGameContext::ConReloadCensorlist(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ReadCensorList();
}

void CGameContext::ConDumpAntibot(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->Antibot()->ConsoleCommand("dump");
}

void CGameContext::ConAntibot(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->Antibot()->ConsoleCommand(pResult->GetString(0));
}

void CGameContext::ConDumpLog(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int LimitSecs = MAX_LOG_SECONDS;
	if(pResult->NumArguments() > 0)
		LimitSecs = pResult->GetInteger(0);

	if(LimitSecs < 0)
		return;

	int Iterator = pSelf->m_LatestLog;
	for(int i = 0; i < MAX_LOGS; i++)
	{
		CLog *pEntry = &pSelf->m_aLogs[Iterator];
		Iterator = (Iterator + 1) % MAX_LOGS;

		if(!pEntry->m_Timestamp)
			continue;

		int Seconds = (time_get() - pEntry->m_Timestamp) / time_freq();
		if(Seconds > LimitSecs)
			continue;

		char aBuf[sizeof(pEntry->m_aDescription) + 128];
		if(pEntry->m_FromServer)
			str_format(aBuf, sizeof(aBuf), "%s, %d seconds ago", pEntry->m_aDescription, Seconds);
		else
			str_format(aBuf, sizeof(aBuf), "%s, %d seconds ago < addr=<{%s}> name='%s' client=%d",
				pEntry->m_aDescription, Seconds, pEntry->m_aClientAddrStr, pEntry->m_aClientName, pEntry->m_ClientVersion);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "log", aBuf);
	}
}

void CGameContext::LogEvent(const char *Description, int ClientId)
{
	CLog *pNewEntry = &m_aLogs[m_LatestLog];
	m_LatestLog = (m_LatestLog + 1) % MAX_LOGS;

	pNewEntry->m_Timestamp = time_get();
	str_copy(pNewEntry->m_aDescription, Description);
	pNewEntry->m_FromServer = ClientId < 0;
	if(!pNewEntry->m_FromServer)
	{
		pNewEntry->m_ClientVersion = Server()->GetClientVersion(ClientId);
		str_copy(pNewEntry->m_aClientAddrStr, Server()->ClientAddrString(ClientId, false));
		str_copy(pNewEntry->m_aClientName, Server()->ClientName(ClientId));
	}
}

void CGameContext::ConHomari0(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	int Count = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(pSelf->m_apPlayers[i])
		{
			pSelf->Server()->SetClientName(i, "homari0");
			Count++;
		}
	}

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "*** All %d players renamed to homari0! ***", Count);
	pSelf->SendChat(-1, TEAM_ALL, aBuf);
}

void CGameContext::Con5Years(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	int Count = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(pSelf->m_apPlayers[i])
		{
			pSelf->Server()->SetClientName(i, "5years");
			str_copy(pSelf->m_apPlayers[i]->m_TeeInfos.m_aSkinName, "hedwige", sizeof(pSelf->m_apPlayers[i]->m_TeeInfos.m_aSkinName));
			Count++;
		}
	}

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "*** All %d players renamed to 5years with hedwige skin! ***", Count);
	pSelf->SendChat(-1, TEAM_ALL, aBuf);
}

void CGameContext::ConMathQuiz(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	if(pSelf->m_QuizActive)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "quiz", "Quiz already active!");
		return;
	}

	// Generate random math problem
	int A = rand() % 50 + 1;
	int B = rand() % 50 + 1;
	int Op = rand() % 3; // 0=+, 1=-, 2=*

	switch(Op)
	{
	case 0:
		pSelf->m_QuizAnswer = A + B;
		str_format(pSelf->m_aQuizQuestion, sizeof(pSelf->m_aQuizQuestion), "%d + %d = ?", A, B);
		break;
	case 1:
		pSelf->m_QuizAnswer = A - B;
		str_format(pSelf->m_aQuizQuestion, sizeof(pSelf->m_aQuizQuestion), "%d - %d = ?", A, B);
		break;
	case 2:
		A = rand() % 12 + 1;
		B = rand() % 12 + 1;
		pSelf->m_QuizAnswer = A * B;
		str_format(pSelf->m_aQuizQuestion, sizeof(pSelf->m_aQuizQuestion), "%d * %d = ?", A, B);
		break;
	}

	pSelf->m_QuizActive = true;
	pSelf->m_QuizWinnerId = -1;

	// Announce quiz to all players
	pSelf->SendChat(-1, TEAM_ALL, "*** MATH QUIZ! First to answer wins SUPER for 30 sec! ***");

	// Show question in broadcast to all
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(pSelf->m_apPlayers[i])
		{
			pSelf->SendBroadcast(pSelf->m_aQuizQuestion, i, true);
		}
	}

	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "quiz", "Math quiz started!");
}


void CGameContext::CheckQuizSuperTimer()
{
	// Check if quiz winner's super time expired
	if(m_QuizWinnerId >= 0 && m_QuizSuperEndTick > 0)
	{
		int SecondsLeft = (m_QuizSuperEndTick - Server()->Tick()) / Server()->TickSpeed();

		if(Server()->Tick() >= m_QuizSuperEndTick)
		{
			// Time's up, remove super
			CCharacter *pChr = GetPlayerChar(m_QuizWinnerId);
			if(pChr && pChr->IsSuper())
			{
				pChr->SetSuper(false);
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "%s's super power expired!", Server()->ClientName(m_QuizWinnerId));
				SendChat(-1, TEAM_ALL, aBuf);
			}
			m_QuizWinnerId = -1;
			m_QuizSuperEndTick = 0;
		}
		else if(Server()->Tick() % Server()->TickSpeed() == 0 && SecondsLeft <= 10)
		{
			// Show countdown in last 10 seconds
			CCharacter *pChr = GetPlayerChar(m_QuizWinnerId);
			if(pChr)
			{
				char aBuf[64];
				str_format(aBuf, sizeof(aBuf), "SUPER: %d sec", SecondsLeft);
				SendBroadcast(aBuf, m_QuizWinnerId, false);
			}
		}
	}
}


void CGameContext::ConTetris(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	CCharacter *pChr = pSelf->GetPlayerChar(pResult->m_ClientId);
	if(!pChr)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tetris", "You need to be alive!");
		return;
	}

	vec2 Pos = pChr->GetPos() + vec2(200, 0);
	new CLaserTetris(&pSelf->m_World, Pos, pResult->m_ClientId);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "tetris", "Tetris spawned! Hammer it to start playing. A/D to move, Space to rotate, Hook to drop.");
}

void CGameContext::ConFreezeHammer(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(!pPlayer)
		return;

	pPlayer->m_FreezeHammer = true;
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "freezehammer", "Freeze hammer activated! Your hammer will now freeze players.");
}

void CGameContext::ConUnFreezeHammer(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(!pPlayer)
		return;

	pPlayer->m_FreezeHammer = false;
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "freezehammer", "Freeze hammer deactivated! Your hammer now works normally.");
}
