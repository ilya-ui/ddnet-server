#ifndef GAME_SERVER_ENTITIES_SNAKE_BOARD_H
#define GAME_SERVER_ENTITIES_SNAKE_BOARD_H

#include <game/server/entity.h>
#include <deque>

class CSnakeBoard : public CEntity
{
public:
	CSnakeBoard(CGameWorld *pGameWorld, vec2 Pos, int Owner);

	void Reset() override;
	void Tick() override;
	void Snap(int SnappingClient) override;

private:
	int m_Owner;
	int m_Timer;
	int m_MoveTimer;
    int m_Speed;
	bool m_Active;
    bool m_GameOver;

    struct SPoint { int x, y; };
    std::deque<SPoint> m_SnakeBody;
    SPoint m_FoodPos;
    int m_Direction; // 0=Right, 1=Down, 2=Left, 3=Up
    int m_NextDirection;

    void SpawnFood();
    void MoveSnake();
    void ResetGame();
};

#endif
