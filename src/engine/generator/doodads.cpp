
#include "generator.h"

void Grass(CGenerator *pGenerator)
{
	CDoodad *pGrass = new CDoodad();

	{//Shape #0
		CDoodadShape Shape;
		Shape.m_Width = 1;
		Shape.m_Index = 14;
		Shape.m_Height = 1;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #1
		CDoodadShape Shape;
		Shape.m_Width = 1;
		Shape.m_Index = 30;
		Shape.m_Height = 1;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #2
		CDoodadShape Shape;
		Shape.m_Width = 1;
		Shape.m_Index = 36;
		Shape.m_Height = 1;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #3
		CDoodadShape Shape;
		Shape.m_Width = 1;
		Shape.m_Index = 37;
		Shape.m_Height = 1;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #4
		CDoodadShape Shape;
		Shape.m_Width = 1;
		Shape.m_Index = 38;
		Shape.m_Height = 1;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #5
		CDoodadShape Shape;
		Shape.m_Width = 2;
		Shape.m_Index = 24;
		Shape.m_Height = 1;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #6
		CDoodadShape Shape;
		Shape.m_Width = 2;
		Shape.m_Index = 10;
		Shape.m_Height = 2;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #7
		CDoodadShape Shape;
		Shape.m_Width = 2;
		Shape.m_Index = 39;
		Shape.m_Height = 1;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #8
		CDoodadShape Shape;
		Shape.m_Width = 2;
		Shape.m_Index = 44;
		Shape.m_Height = 1;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #9
		CDoodadShape Shape;
		Shape.m_Width = 3;
		Shape.m_Index = 1;
		Shape.m_Height = 3;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #10
		CDoodadShape Shape;
		Shape.m_Width = 3;
		Shape.m_Index = 56;
		Shape.m_Height = 2;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #11
		CDoodadShape Shape;
		Shape.m_Width = 3;
		Shape.m_Index = 59;
		Shape.m_Height = 2;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #11
		CDoodadShape Shape;
		Shape.m_Width = 4;
		Shape.m_Index = 4;
		Shape.m_Height = 2;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #12
		CDoodadShape Shape;
		Shape.m_Width = 4;
		Shape.m_Index = 101;
		Shape.m_Height = 5;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #13
		CDoodadShape Shape;
		Shape.m_Width = 5;
		Shape.m_Index = 96;
		Shape.m_Height = 5;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #14
		CDoodadShape Shape;
		Shape.m_Width = 6;
		Shape.m_Index = 90;
		Shape.m_Height = 6;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #15
		CDoodadShape Shape;
		Shape.m_Width = 8;
		Shape.m_Index = 48;
		Shape.m_Height = 3;
		pGrass->m_DoodadShapes.add(Shape);
	}

	pGenerator->AddDoodad(pGrass, "grass_doodads", 0);
}

void Winter(CGenerator *pGenerator)
{
	CDoodad *pGrass = new CDoodad();

	{//Shape #0
		CDoodadShape Shape;
		Shape.m_Width = 3;
		Shape.m_Index = 6;
		Shape.m_Height = 6;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #1
		CDoodadShape Shape;
		Shape.m_Width = 3;
		Shape.m_Index = 9;
		Shape.m_Height = 5;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #2
		CDoodadShape Shape;
		Shape.m_Width = 3;
		Shape.m_Index = 12;
		Shape.m_Height = 4;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #3
		CDoodadShape Shape;
		Shape.m_Width = 3;
		Shape.m_Index = 16;
		Shape.m_Height = 5;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #4
		CDoodadShape Shape;
		Shape.m_Width = 3;
		Shape.m_Index = 19;
		Shape.m_Height = 3;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #5
		CDoodadShape Shape;
		Shape.m_Width = 6;
		Shape.m_Index = 96;
		Shape.m_Height = 4;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #6
		CDoodadShape Shape;
		Shape.m_Width = 3;
		Shape.m_Index = 102;
		Shape.m_Height = 4;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #7
		CDoodadShape Shape;
		Shape.m_Width = 3;
		Shape.m_Index = 89;
		Shape.m_Height = 2;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #8
		CDoodadShape Shape;
		Shape.m_Width = 3;
		Shape.m_Index = 92;
		Shape.m_Height = 2;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #9
		CDoodadShape Shape;
		Shape.m_Width = 1;
		Shape.m_Index = 141;
		Shape.m_Height = 1;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #10
		CDoodadShape Shape;
		Shape.m_Width = 1;
		Shape.m_Index = 125;
		Shape.m_Height = 1;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #11
		CDoodadShape Shape;
		Shape.m_Width = 1;
		Shape.m_Index = 126;
		Shape.m_Height = 1;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #11
		CDoodadShape Shape;
		Shape.m_Width = 1;
		Shape.m_Index = 127;
		Shape.m_Height = 1;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #12
		CDoodadShape Shape;
		Shape.m_Width = 1;
		Shape.m_Index = 168;
		Shape.m_Height = 3;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #13
		CDoodadShape Shape;
		Shape.m_Width = 1;
		Shape.m_Index = 216;
		Shape.m_Height = 3;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #14
		CDoodadShape Shape;
		Shape.m_Width = 1;
		Shape.m_Index = 63;
		Shape.m_Height = 2;
		pGrass->m_DoodadShapes.add(Shape);
	}

	{//Shape #15
		CDoodadShape Shape;
		Shape.m_Width = 1;
		Shape.m_Index = 95;
		Shape.m_Height = 2;
		pGrass->m_DoodadShapes.add(Shape);
	}

	pGenerator->AddDoodad(pGrass, "winter_doodads", 0);
}

void Desert(CGenerator *pGenerator)
{
	CDoodad *pDesert = new CDoodad();

	{//Shape #0
		CDoodadShape Shape;
		Shape.m_Index = 32;
		Shape.m_Width = 3;
		Shape.m_Height = 1;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #1
		CDoodadShape Shape;
		Shape.m_Index = 19;
		Shape.m_Width = 4;
		Shape.m_Height = 2;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #3
		CDoodadShape Shape;
		Shape.m_Index = 23;
		Shape.m_Width = 4;
		Shape.m_Height = 2;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #4
		CDoodadShape Shape;
		Shape.m_Index = 11;
		Shape.m_Width = 3;
		Shape.m_Height = 3;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #6
		CDoodadShape Shape;
		Shape.m_Index = 64;
		Shape.m_Width = 3;
		Shape.m_Height = 1;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #7
		CDoodadShape Shape;
		Shape.m_Index = 96;
		Shape.m_Width = 3;
		Shape.m_Height = 1;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #8
		CDoodadShape Shape;
		Shape.m_Index = 99;
		Shape.m_Width = 2;
		Shape.m_Height = 1;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #9
		CDoodadShape Shape;
		Shape.m_Index = 101;
		Shape.m_Width = 1;
		Shape.m_Height = 1;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #10
		CDoodadShape Shape;
		Shape.m_Index = 102;
		Shape.m_Width = 2;
		Shape.m_Height = 1;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #11
		CDoodadShape Shape;
		Shape.m_Index = 88;
		Shape.m_Width = 3;
		Shape.m_Height = 2;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #12
		CDoodadShape Shape;
		Shape.m_Index = 91;
		Shape.m_Width = 2;
		Shape.m_Height = 2;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #13
		CDoodadShape Shape;
		Shape.m_Index = 77;
		Shape.m_Width = 3;
		Shape.m_Height = 3;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #14
		CDoodadShape Shape;
		Shape.m_Index = 148;
		Shape.m_Width = 1;
		Shape.m_Height = 1;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #15
		CDoodadShape Shape;
		Shape.m_Index = 164;
		Shape.m_Width = 1;
		Shape.m_Height = 1;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #16
		CDoodadShape Shape;
		Shape.m_Index = 180;
		Shape.m_Width = 1;
		Shape.m_Height = 1;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #17
		CDoodadShape Shape;
		Shape.m_Index = 149;
		Shape.m_Width = 2;
		Shape.m_Height = 3;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #18
		CDoodadShape Shape;
		Shape.m_Index = 167;
		Shape.m_Width = 2;
		Shape.m_Height = 2;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #19
		CDoodadShape Shape;
		Shape.m_Index = 169;
		Shape.m_Width = 1;
		Shape.m_Height = 1;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #20
		CDoodadShape Shape;
		Shape.m_Index = 170;
		Shape.m_Width = 1;
		Shape.m_Height = 1;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #21
		CDoodadShape Shape;
		Shape.m_Index = 139;
		Shape.m_Width = 2;
		Shape.m_Height = 4;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #22
		CDoodadShape Shape;
		Shape.m_Index = 157;
		Shape.m_Width = 2;
		Shape.m_Height = 3;
		pDesert->m_DoodadShapes.add(Shape);
	}

	pGenerator->AddDoodad(pDesert, "desert_doodads", 0);
}

void Jungle(CGenerator *pGenerator)
{
	CDoodad *pDesert = new CDoodad();

	{//Shape #0
		CDoodadShape Shape;
		Shape.m_Index = 6;
		Shape.m_Width = 9;
		Shape.m_Height = 3;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #1
		CDoodadShape Shape;
		Shape.m_Index = 56;
		Shape.m_Width = 4;
		Shape.m_Height = 2;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #2
		CDoodadShape Shape;
		Shape.m_Index = 60;
		Shape.m_Width = 2;
		Shape.m_Height = 1;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #3
		CDoodadShape Shape;
		Shape.m_Index = 115;
		Shape.m_Width = 1;
		Shape.m_Height = 1;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #4
		CDoodadShape Shape;
		Shape.m_Index = 116;
		Shape.m_Width = 1;
		Shape.m_Height = 1;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #5
		CDoodadShape Shape;
		Shape.m_Index = 128;
		Shape.m_Width = 1;
		Shape.m_Height = 1;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #6
		CDoodadShape Shape;
		Shape.m_Index = 129;
		Shape.m_Width = 2;
		Shape.m_Height = 1;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #7
		CDoodadShape Shape;
		Shape.m_Index = 131;
		Shape.m_Width = 2;
		Shape.m_Height = 1;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #8
		CDoodadShape Shape;
		Shape.m_Index = 150;
		Shape.m_Width = 2;
		Shape.m_Height = 2;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #9
		CDoodadShape Shape;
		Shape.m_Index = 176;
		Shape.m_Width = 5;
		Shape.m_Height = 5;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #10
		CDoodadShape Shape;
		Shape.m_Index = 76;
		Shape.m_Width = 4;
		Shape.m_Height = 7;
		pDesert->m_DoodadShapes.add(Shape);
	}

	{//Shape #11
		CDoodadShape Shape;
		Shape.m_Index = 136;
		Shape.m_Width = 4;
		Shape.m_Height = 3;
		pDesert->m_DoodadShapes.add(Shape);
	}

	pGenerator->AddDoodad(pDesert, "jungle_doodads", 0);
}

void Tundra(CGenerator *pGenerator)
{
	CDoodad *pTundra = new CDoodad();

	{//Shape #0
		CDoodadShape Shape;
		Shape.m_Width = 1;
		Shape.m_Index = 14;
		Shape.m_Height = 1;
		pTundra->m_DoodadShapes.add(Shape);
	}

	{//Shape #1
		CDoodadShape Shape;
		Shape.m_Width = 1;
		Shape.m_Index = 30;
		Shape.m_Height = 1;
		pTundra->m_DoodadShapes.add(Shape);
	}

	{//Shape #2
		CDoodadShape Shape;
		Shape.m_Width = 1;
		Shape.m_Index = 36;
		Shape.m_Height = 1;
		pTundra->m_DoodadShapes.add(Shape);
	}

	{//Shape #3
		CDoodadShape Shape;
		Shape.m_Width = 1;
		Shape.m_Index = 37;
		Shape.m_Height = 1;
		pTundra->m_DoodadShapes.add(Shape);
	}

	{//Shape #4
		CDoodadShape Shape;
		Shape.m_Width = 1;
		Shape.m_Index = 38;
		Shape.m_Height = 1;
		pTundra->m_DoodadShapes.add(Shape);
	}

	{//Shape #5
		CDoodadShape Shape;
		Shape.m_Width = 2;
		Shape.m_Index = 24;
		Shape.m_Height = 1;
		pTundra->m_DoodadShapes.add(Shape);
	}

	{//Shape #6
		CDoodadShape Shape;
		Shape.m_Width = 2;
		Shape.m_Index = 10;
		Shape.m_Height = 2;
		pTundra->m_DoodadShapes.add(Shape);
	}

	{//Shape #8
		CDoodadShape Shape;
		Shape.m_Width = 2;
		Shape.m_Index = 44;
		Shape.m_Height = 1;
		pTundra->m_DoodadShapes.add(Shape);
	}

	{//Shape #10
		CDoodadShape Shape;
		Shape.m_Width = 3;
		Shape.m_Index = 56;
		Shape.m_Height = 2;
		pTundra->m_DoodadShapes.add(Shape);
	}

	{//Shape #11
		CDoodadShape Shape;
		Shape.m_Width = 3;
		Shape.m_Index = 59;
		Shape.m_Height = 2;
		pTundra->m_DoodadShapes.add(Shape);
	}

	{//Shape #11
		CDoodadShape Shape;
		Shape.m_Width = 4;
		Shape.m_Index = 4;
		Shape.m_Height = 2;
		pTundra->m_DoodadShapes.add(Shape);
	}

	{//Shape #15
		CDoodadShape Shape;
		Shape.m_Width = 8;
		Shape.m_Index = 48;
		Shape.m_Height = 3;
		pTundra->m_DoodadShapes.add(Shape);
	}

	pGenerator->AddDoodad(pTundra, "grass_doodads", 1);
}


void CGenerator::Doodads()
{
	Grass(this);
	Winter(this);
	Desert(this);
	Jungle(this);
	Tundra(this);
}