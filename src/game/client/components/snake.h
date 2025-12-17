#ifndef GAME_CLIENT_COMPONENTS_SNAKE_H
#define GAME_CLIENT_COMPONENTS_SNAKE_H

#include <game/client/component.h>
#include <engine/console.h>
#include <base/vmath.h>
#include <vector>

class CSnakeGame : public CComponent
{
	struct SSnakeSegment
	{
		int x, y;
	};

	bool m_Active;
	vec2 m_WindowPos;
	bool m_Playing;
	bool m_GameOver;
	
	std::vector<SSnakeSegment> m_SnakeBody;
	SSnakeSegment m_FoodPos;
	int m_Direction; // 0=Right, 1=Down, 2=Left, 3=Up
	int m_NextDirection;
	int64_t m_LastUpdateTick;
	int m_Score;
    int m_Timer;

	static void ConSnakeGame(IConsole::IResult *pResult, void *pUserData);
	static void ConUnSnakeGame(IConsole::IResult *pResult, void *pUserData);

	void StartGame();
	void StopGame();
	void ResetGame();
	void SpawnFood();
	void UpdateGame();

public:
	CSnakeGame();
	virtual int Sizeof() const override { return sizeof(*this); }

	virtual void OnConsoleInit() override;
	virtual void OnRender() override;
	virtual void OnReset() override;
};

#endif
