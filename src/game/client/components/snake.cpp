#include "snake.h"

#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/keys.h>
#include <engine/shared/config.h>
#include <game/client/gameclient.h>
#include <game/client/components/controls.h>
#include <generated/protocol.h>
#include <generated/client_data.h>

CSnakeGame::CSnakeGame()
{
	m_Active = false;
	m_Playing = false;
	m_GameOver = false;
	m_Score = 0;
	m_Direction = 0;
	m_NextDirection = 0;
    m_Timer = 0;
}

void CSnakeGame::OnConsoleInit()
{
	Console()->Register("snakegame", "", CFGFLAG_CLIENT, ConSnakeGame, this, "Start Snake Game window");
	Console()->Register("unsnakegame", "", CFGFLAG_CLIENT, ConUnSnakeGame, this, "Remove Snake Game window");
}

void CSnakeGame::ConSnakeGame(IConsole::IResult *pResult, void *pUserData)
{
	CSnakeGame *pSelf = (CSnakeGame *)pUserData;
	pSelf->m_Active = true;
	pSelf->m_WindowPos = pSelf->GameClient()->m_LocalCharacterPos;
	pSelf->m_WindowPos.y -= 100.0f; // Spawn slightly above player
	pSelf->ResetGame();
}

void CSnakeGame::ConUnSnakeGame(IConsole::IResult *pResult, void *pUserData)
{
	CSnakeGame *pSelf = (CSnakeGame *)pUserData;
	pSelf->m_Active = false;
	pSelf->m_Playing = false;
}

void CSnakeGame::ResetGame()
{
	m_Playing = false;
	m_GameOver = false;
	m_Score = 0;
	m_Direction = 0;
	m_NextDirection = 0;
	m_SnakeBody.clear();
	m_SnakeBody.push_back({10, 10});
	m_SnakeBody.push_back({9, 10});
	m_SnakeBody.push_back({8, 10});
	SpawnFood();
}

void CSnakeGame::StartGame()
{
	if(m_GameOver)
		ResetGame();
	
	m_Playing = true;
	m_LastUpdateTick = time_get();
}

void CSnakeGame::StopGame()
{
	m_Playing = false;
}

void CSnakeGame::SpawnFood()
{
	bool Valid = false;
	while(!Valid)
	{
		m_FoodPos.x = rand() % 20;
		m_FoodPos.y = rand() % 20;
		Valid = true;
		for(const auto &Seg : m_SnakeBody)
		{
			if(Seg.x == m_FoodPos.x && Seg.y == m_FoodPos.y)
			{
				Valid = false;
				break;
			}
		}
	}
}

void CSnakeGame::UpdateGame()
{
	if(!m_Playing || m_GameOver)
		return;

    // Use a fixed tick rate (e.g., 10 updates per second)
	if(time_get() - m_LastUpdateTick < time_freq() / 10)
		return;

	m_LastUpdateTick = time_get();
	m_Direction = m_NextDirection;

	SSnakeSegment Head = m_SnakeBody[0];
	if(m_Direction == 0) Head.x++;
	else if(m_Direction == 1) Head.y++;
	else if(m_Direction == 2) Head.x--;
	else if(m_Direction == 3) Head.y--;

	// Wall collision
	if(Head.x < 0 || Head.x >= 20 || Head.y < 0 || Head.y >= 20)
	{
		m_GameOver = true;
		m_Playing = false;
		return;
	}

	// Self collision
	for(const auto &Seg : m_SnakeBody)
	{
		if(Head.x == Seg.x && Head.y == Seg.y)
		{
			m_GameOver = true;
			m_Playing = false;
			return;
		}
	}

	m_SnakeBody.insert(m_SnakeBody.begin(), Head);

	if(Head.x == m_FoodPos.x && Head.y == m_FoodPos.y)
	{
		m_Score++;
		SpawnFood();
	}
	else
	{
		m_SnakeBody.pop_back();
	}
}

void CSnakeGame::OnRender()
{
	if(!m_Active)
		return;

	// Draw Window Background
	float CellSize = 10.0f;
	float BoardSize = 20 * CellSize;
	float Padding = 10.0f;
	float WindowWidth = BoardSize + Padding * 2;
	float WindowHeight = BoardSize + Padding * 2 + 30.0f; // Extra space for header

	vec2 Center = m_WindowPos;
    
    // Simple logic to keep movement controls relative to the game window
    // We'll check player input relative to the camera for snake control
    // But since this is a "world" object, maybe we should just use absolute keys?
    // Let's use local player input for simplicity of "playing"

	Graphics()->BlendNormal();
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.8f);
    IGraphics::CQuadItem QuadItem(Center.x - WindowWidth / 2, Center.y - WindowHeight / 2, WindowWidth, WindowHeight);
    Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	// Draw Board Background
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.1f, 0.1f, 0.1f, 1.0f);
    IGraphics::CQuadItem BoardQuad(Center.x - BoardSize / 2, Center.y - BoardSize / 2 + 15.0f, BoardSize, BoardSize);
    Graphics()->QuadsDrawTL(&BoardQuad, 1);
	Graphics()->QuadsEnd();

	// Draw Snake
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 1.0f, 0.0f, 1.0f);
	for(const auto &Seg : m_SnakeBody)
	{
		float X = Center.x - BoardSize / 2 + Seg.x * CellSize;
		float Y = Center.y - BoardSize / 2 + 15.0f + Seg.y * CellSize;
        IGraphics::CQuadItem SnakeQuad(X, Y, CellSize - 1, CellSize - 1);
        Graphics()->QuadsDrawTL(&SnakeQuad, 1);
	}
	Graphics()->QuadsEnd();

	// Draw Food
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
	float FoodX = Center.x - BoardSize / 2 + m_FoodPos.x * CellSize;
	float FoodY = Center.y - BoardSize / 2 + 15.0f + m_FoodPos.y * CellSize;
    IGraphics::CQuadItem FoodQuad(FoodX, FoodY, CellSize - 1, CellSize - 1);
    Graphics()->QuadsDrawTL(&FoodQuad, 1);
	Graphics()->QuadsEnd();

	// Draw Text
	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "Score: %d", m_Score);
	TextRender()->Text(Center.x - WindowWidth / 2 + Padding, Center.y - WindowHeight / 2 + Padding, 10.0f, aBuf);

	if(m_GameOver)
	{
		TextRender()->TextColor(1.0f, 0.0f, 0.0f, 1.0f);
		TextRender()->Text(Center.x - 40, Center.y, 20.0f, "GAME OVER");
		TextRender()->Text(Center.x - 50, Center.y + 20, 10.0f, "Hit with Hammer to Restart");
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
	else if(!m_Playing)
	{
		TextRender()->Text(Center.x - 50, Center.y, 10.0f, "Hit with Hammer to Start");
	}

    // Interaction Logic: Hammer Hit
    if(GameClient()->m_Snap.m_pLocalCharacter)
    {
        // Check for hammer hit
        // We look at the local player's input fire state and weapon
        // If they assume we are "hitting" the window if the cursor is near it or if the player is close?
        // Let's assume if the player hits the window within a radius.
        
        // Actually, checking "hammer hit" means we need to detect the fire event.
        // CGameClient has m_Controls.m_InputData which has the fire state.
        // We can detect the rising edge of fire while holding hammer.
        
        bool Fired = false;
        // Basic edge detection could be done if we stored last state, but for now let's just use the current input state
        // and a cooldown or check if we haven't processed it yet.
        // Better: Check active weapon and if pLocalCharacter->m_AttackTick just changed?
        // Or just use the input directly.

        static int s_LastFire = 0;
        int CurrentFire = GameClient()->m_Controls.m_aInputData[g_Config.m_ClDummy].m_Fire;
        
        if(CurrentFire && !s_LastFire)
        {
             // Fired this tick
             if(GameClient()->m_Snap.m_pLocalCharacter->m_Weapon == WEAPON_HAMMER)
             {
                 vec2 PlayerPos = GameClient()->m_LocalCharacterPos;
                 if(distance(PlayerPos, m_WindowPos) < 100.0f) // Close enough
                 {
                     if(m_GameOver) ResetGame();
                     
                     if(m_Playing) StopGame();
                     else StartGame();
                 }
             }
        }
        s_LastFire = CurrentFire;

        // Input Logic for Snake
        if(m_Playing)
        {
             // We can use the player's movement keys to control the snake
             // But we need to make sure we don't move the player? 
             // That's hard client-side only without affecting gameplay.
             // Maybe we just read the inputs.
             // If the player is "playing snake", maybe we can suppress their movement inputs being sent to server?
             // But that might desync or look weird.
             // User just said "you approach it and hit with hammer to start snake game".
             // It implies you stop playing the main game.
             
             // Let's hook into the input inputs.
             // CControls::m_InputDirectionLeft/Right etc.
             
             // We'll just read `g_Config.m_ClDummy` inputs.
             
             // We need to map WASD / Arrows from the OS input, but standard way is `m_pInput`.
             // But simpler is to look at what `CControls` or `CGameClient` sees.
             
             // Let's use `GameClient()->m_Controls.m_InputData`
            
            // To prevent movement, we might need to modify CControls or just accept the player moves around while playing snake.
            // Let's assume player moves around is fine for now, or maybe the user wants to be "locked" in.
            // "Чтобы выйти тоже ударить" implies a mode switch.
            
            // Checking local inputs is better.
            // But for now let's just check the net input direction.
            // This usually reflects the keys pressed.
             
            if(GameClient()->m_Controls.m_aInputData[g_Config.m_ClDummy].m_Direction == -1 && m_Direction != 0) m_NextDirection = 2; // Left
            if(GameClient()->m_Controls.m_aInputData[g_Config.m_ClDummy].m_Direction == 1 && m_Direction != 2) m_NextDirection = 0; // Right
            // Jump/Hook for Up/Down? Or just aim?
            // Usually Up/Down are not separate inputs in standard tee movement (Jump is Up).
            // But maybe we can use KeyBinder or just check `Input()->KeyIsPressed(...)`.
            
            // Let's use `Input()->KeyIsPressed` for arrow keys/WASD if possible, but that requires `IInput`.
            // `CGameClient` has `Input()`.
             
            // We need to be careful not to use keys that aren't bound.
            // But checking standard Arrow keys is probably safe for a "minigame".
            
            if(Input()->KeyIsPressed(KEY_LEFT) || Input()->KeyIsPressed(KEY_A)) { if(m_Direction != 0) m_NextDirection = 2; }
            if(Input()->KeyIsPressed(KEY_RIGHT) || Input()->KeyIsPressed(KEY_D)) { if(m_Direction != 2) m_NextDirection = 0; }
            if(Input()->KeyIsPressed(KEY_UP) || Input()->KeyIsPressed(KEY_W)) { if(m_Direction != 1) m_NextDirection = 3; }
            if(Input()->KeyIsPressed(KEY_DOWN) || Input()->KeyIsPressed(KEY_S)) { if(m_Direction != 3) m_NextDirection = 1; }
        }
        
        UpdateGame();
    }
}

void CSnakeGame::OnReset()
{
	m_Active = false;
	ResetGame();
}
