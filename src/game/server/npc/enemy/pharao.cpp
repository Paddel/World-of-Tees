
#include <game/server/gamecontext.h>

#include "pharao.h"

CPharao::CPharao(CGameContext *pGameServer, CMap *pMap, CNpcHolder *pNpcHolder, vec2 Pos)
		: CEnemyNpc(pGameServer, pMap, pNpcHolder, Pos, NPC_PHARAO)
{
}