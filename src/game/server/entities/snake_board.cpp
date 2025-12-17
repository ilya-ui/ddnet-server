#include "snake_board.h"
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/gamemodes/DDRace.h>
#include <game/server/entities/character.h>
#include <engine/shared/config.h>

CSnakeBoard::CSnakeBoard(CGameWorld *pGameWorld, vec2 Pos, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_SNAKE, Pos) // Use Snake Type
{
    m_Pos = Pos;
    m_Owner = Owner;
    m_Active = true;
    m_GameOver = false;
    m_MoveTimer = 0;
    m_Speed = 10; // Ticks per move (50 ticks/sec -> 5 moves/sec)

    ResetGame();
    GameWorld()->InsertEntity(this);
}

void CSnakeBoard::ResetGame()
{
    m_SnakeBody.clear();
    m_SnakeBody.push_back({10, 10});
    m_SnakeBody.push_back({9, 10});
    m_SnakeBody.push_back({8, 10});
    m_Direction = 0;
    m_NextDirection = 0;
    m_GameOver = false;
    SpawnFood();
}

void CSnakeBoard::Reset()
{
    m_MarkedForDestroy = true;
}

void CSnakeBoard::SpawnFood()
{
    while(true)
    {
        int x = rand() % 20;
        int y = rand() % 20;
        bool Collision = false;
        for(const auto& Seg : m_SnakeBody)
        {
            if(Seg.x == x && Seg.y == y)
            {
                Collision = true;
                break;
            }
        }
        if(!Collision)
        {
            m_FoodPos = {x, y};
            break;
        }
    }
}

void CSnakeBoard::Tick()
{
    if(!m_Active) return;

    // Check Inputs from Owner
    CPlayer *pOwner = GameServer()->m_apPlayers[m_Owner];
    if(pOwner)
    {
        // We can access input from CPlayer or CCharacter
        // Let's look at the character's last input
        // Or directly GetLastPlayerInput from GameContext
        CNetObj_PlayerInput Input = GameServer()->GetLastPlayerInput(m_Owner);
        
        // Simple WASD logic based on Move/Jump/Hook might be hard.
        // Let's use Direction + Jump/Hook.
        // Direction X: -1, 0, 1
        // Jump: 1
        // Hook: 1
        
        // Using standard movement keys might interfere with player movement.
        // But since we are "playing snake", maybe that's fine.
        
        if(Input.m_Direction < 0 && m_Direction != 0) m_NextDirection = 2; // Left
        else if(Input.m_Direction > 0 && m_Direction != 2) m_NextDirection = 0; // Right
        else if(Input.m_Jump && m_Direction != 1) m_NextDirection = 3; // Up
        else if(Input.m_Hook && m_Direction != 3) m_NextDirection = 1; // Down
        
        // Hammer Interaction (Hit the board to reset/start)
        // We can check if any player hits near the board.
        // Simplified: If owner fires hammer near center.
         if (Input.m_Fire & 1 && pOwner->GetCharacter() && pOwner->GetCharacter()->GetActiveWeapon() == WEAPON_HAMMER)
         {
             if(length(pOwner->GetCharacter()->m_Pos - m_Pos) < 200.0f)
             {
                 if(m_GameOver) ResetGame();
             }
         }
    }
    else
    {
        // Owner left
        Reset();
        return;
    }

    if(m_GameOver) return;

    m_MoveTimer++;
    if(m_MoveTimer >= m_Speed)
    {
        m_MoveTimer = 0;
        MoveSnake();
    }
}

void CSnakeBoard::MoveSnake()
{
    m_Direction = m_NextDirection;
    SPoint Head = m_SnakeBody.front();
    
    if(m_Direction == 0) Head.x++;
    if(m_Direction == 1) Head.y++;
    if(m_Direction == 2) Head.x--;
    if(m_Direction == 3) Head.y--;

    // Wall Collision
    if(Head.x < 0 || Head.x >= 20 || Head.y < 0 || Head.y >= 20)
    {
        m_GameOver = true;
        // Send chat/broadcast?
        GameServer()->SendBroadcast("Game Over! Hit with Hammer to restart.", m_Owner);
        return;
    }

    // Self Collision
    for(const auto& Seg : m_SnakeBody)
    {
        if(Seg.x == Head.x && Seg.y == Head.y)
        {
            m_GameOver = true;
             GameServer()->SendBroadcast("Game Over! Hit with Hammer to restart.", m_Owner);
            return;
        }
    }

    m_SnakeBody.push_front(Head);

    if(Head.x == m_FoodPos.x && Head.y == m_FoodPos.y)
    {
        SpawnFood();
        // Speed up?
    }
    else
    {
        m_SnakeBody.pop_back();
    }
}

void CSnakeBoard::Snap(int SnappingClient)
{
    // Draw Board using Lasers
    // A 20x20 grid. Let's make each cell 32 units.
    float CellSize = 32.0f;
    float BoardWidth = 20 * CellSize;
    float BoardHeight = 20 * CellSize;
    vec2 TopLeft = m_Pos - vec2(BoardWidth/2, BoardHeight/2);

    // Draw Border (4 Lasers)
    // We need IDs. We can't really generate infinite IDs.
    // We can use SnapLaserObject which doesn't require a persistent Entity ID if we just want to send a visual.
    // Wait, GameServer()->SnapLaserObject takes an ID. It usually expects a VALID Entity ID.
    // If we use the same ID, it might glitch.
    // Use m_Id + Offset? m_Id is unique.
    
    // Actually, SnapLaserObject sends a LASER Object.
    // The client uses the ID to interpolate. If we change positions, we need consistent IDs.
    // Since the border is static, we can use fixed IDs relative to our Entity ID.
    // But we are limited by MAX_NETOBJ_LASERS potentially? No, just packet size.
    
    int Id = GetId(); // Base ID. Hopefully we don't collide with other entities. 
    // Actually, m_Id is allocated from a pool. We shouldn't use arbitrary offsets if they collide with real entities.
    // But this involves "Server-side rendering" hack.
    // A trick is to use IDs that are high numbers or just hope for the best? 
    // SAFEST: Just SNAP it. If ID conflict, one overrides other?
    // CNetObjHandler usually linearly adds them.
    
    // NOTE: CGameContext::SnapLaserObject calls SnapNewItem which allocates from the Snapshot buffer.
    // The ID passed to it is purely for the Client to track interpolation.
    // So we CAN generate "Pseudo-IDs" as long as they are consistent for this entity.
    // We can simulate IDs by hashing or just assuming we have a "virtual range".
    // Alternatively, we reuse our m_Id but vary the "Type" or something? No.
    
    // Let's use (m_Id * 100) + index. This is risky if ID is large.
    // Better: Helper function to get a pseudo ID.
    // But for now, let's try just drawing ONE laser for the border?
    // Drawing a box needs 4 lasers.
    
    int StartId = GetId() * 200; // Offset separate from other entities.
    
    // We need the CSnapContext.
    CSnapContext Context(GameServer()->GetClientVersion(SnappingClient), GameServer()->Server()->IsSixup(SnappingClient), SnappingClient);

    vec2 TR = TopLeft + vec2(BoardWidth, 0);
    vec2 BL = TopLeft + vec2(0, BoardHeight);
    vec2 BR = TopLeft + vec2(BoardWidth, BoardHeight);

    GameServer()->SnapLaserObject(Context, StartId++, TR, TopLeft, Server()->Tick());
    GameServer()->SnapLaserObject(Context, StartId++, BR, TR, Server()->Tick());
    GameServer()->SnapLaserObject(Context, StartId++, BL, BR, Server()->Tick());
    GameServer()->SnapLaserObject(Context, StartId++, TopLeft, BL, Server()->Tick());

    // Draw Food (Pickup - Heart)
    vec2 FoodWorldPos = TopLeft + vec2(m_FoodPos.x * CellSize + CellSize/2, m_FoodPos.y * CellSize + CellSize/2);
    GameServer()->SnapPickup(Context, StartId++, FoodWorldPos, POWERUP_HEALTH, 0, 0, 0);

    // Draw Snake (Pickups - Armor)
    for(const auto& Seg : m_SnakeBody)
    {
        vec2 SegPos = TopLeft + vec2(Seg.x * CellSize + CellSize/2, Seg.y * CellSize + CellSize/2);
        GameServer()->SnapPickup(Context, StartId++, SegPos, POWERUP_ARMOR, 0, 0, 0);
    }
}
