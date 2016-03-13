
#include "generator.h"

void GrassMin(CGenerator *pGenerator)
{
	CMinimalisticShape NewShape;
	NewShape.m_Start = 41;
	NewShape.m_Tile = 42;
	NewShape.m_Space = 0;
	pGenerator->AddMinimalisticShape(NewShape, "grass_doodads");
}

void WinterMin(CGenerator *pGenerator)
{
	CMinimalisticShape NewShape;
	NewShape.m_Start = 229;
	NewShape.m_Tile = 226;
	NewShape.m_Space = 0;
	pGenerator->AddMinimalisticShape(NewShape, "winter_main");
}

void DesertMin(CGenerator *pGenerator)
{
	CMinimalisticShape NewShape;
	NewShape.m_Start = 181;
	NewShape.m_Tile = 182;
	NewShape.m_Space = 0;
	pGenerator->AddMinimalisticShape(NewShape, "desert_main");
}

void JungleMin(CGenerator *pGenerator)
{
	CMinimalisticShape NewShape;
	NewShape.m_Start = 112;
	NewShape.m_Tile = 113;
	NewShape.m_Space = 1;
	pGenerator->AddMinimalisticShape(NewShape, "jungle_doodads");
}

void CGenerator::MinimalisticShapes()
{
	GrassMin(this);
	WinterMin(this);
	DesertMin(this);
	JungleMin(this);
}