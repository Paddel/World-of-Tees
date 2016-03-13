
#include "generator.h"

void BackColor(CGenerator *pGenerator)
{
	CGenEnvelope *pBackColor = new CGenEnvelope(4);
	pBackColor->m_Synchronized = true;

	{//Point #0
		CEnvPoint Point;
		Point.m_Time = 0;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 133;
		Point.m_aValues[1] = 133;
		Point.m_aValues[2] = 1025;
		Point.m_aValues[3] = 1024;
		pBackColor->m_lPoints.add(Point);
	}

	{//Point #1
		CEnvPoint Point;
		Point.m_Time = 319235;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 302;
		Point.m_aValues[1] = 312;
		Point.m_aValues[2] = 1024;
		Point.m_aValues[3] = 1025;
		pBackColor->m_lPoints.add(Point);
	}

	{//Point #2
		CEnvPoint Point;
		Point.m_Time = 378990;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 993;
		Point.m_aValues[1] = 351;
		Point.m_aValues[2] = 147;
		Point.m_aValues[3] = 1024;
		pBackColor->m_lPoints.add(Point);
	}

	{//Point #3
		CEnvPoint Point;
		Point.m_Time = 459423;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 436;
		Point.m_aValues[1] = 504;
		Point.m_aValues[2] = 1024;
		Point.m_aValues[3] = 1024;
		pBackColor->m_lPoints.add(Point);
	}

	{//Point #4
		CEnvPoint Point;
		Point.m_Time = 720000;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 563;
		Point.m_aValues[1] = 563;
		Point.m_aValues[2] = 1024;
		Point.m_aValues[3] = 1024;
		pBackColor->m_lPoints.add(Point);
	}

	{//Point #5
		CEnvPoint Point;
		Point.m_Time = 1031338;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 469;
		Point.m_aValues[1] = 496;
		Point.m_aValues[2] = 1024;
		Point.m_aValues[3] = 1024;
		pBackColor->m_lPoints.add(Point);
	}

	{//Point #6
		CEnvPoint Point;
		Point.m_Time = 1084408;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 714;
		Point.m_aValues[1] = 227;
		Point.m_aValues[2] = 49;
		Point.m_aValues[3] = 1024;
		pBackColor->m_lPoints.add(Point);
	}

	{//Point #7
		CEnvPoint Point;
		Point.m_Time = 1133819;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 253;
		Point.m_aValues[1] = 230;
		Point.m_aValues[2] = 1024;
		Point.m_aValues[3] = 1024;
		pBackColor->m_lPoints.add(Point);
	}

	{//Point #8
		CEnvPoint Point;
		Point.m_Time = 1440000;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 133;
		Point.m_aValues[1] = 133;
		Point.m_aValues[2] = 1024;
		Point.m_aValues[3] = 1024;
		pBackColor->m_lPoints.add(Point);
	}

	pGenerator->AddEnvelopeTemplate(pBackColor, "BackColor");
}

void Aquarell(CGenerator *pGenerator)
{
	CGenEnvelope *pAquarell = new CGenEnvelope(4);
	pAquarell->m_Synchronized = true;

	{//Point #0
		CEnvPoint Point;
		Point.m_Time = 0;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 1024;
		Point.m_aValues[1] = 1024;
		Point.m_aValues[2] = 1025;
		Point.m_aValues[3] = 1024;
		pAquarell->m_lPoints.add(Point);
	}

	{//Point #1
		CEnvPoint Point;
		Point.m_Time = 319235;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 1024;
		Point.m_aValues[1] = 1024;
		Point.m_aValues[2] = 1024;
		Point.m_aValues[3] = 1025;
		pAquarell->m_lPoints.add(Point);
	}

	{//Point #2
		CEnvPoint Point;
		Point.m_Time = 378990;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 1024;
		Point.m_aValues[1] = 1024;
		Point.m_aValues[2] = 1024;
		Point.m_aValues[3] = 269;
		pAquarell->m_lPoints.add(Point);
	}

	{//Point #3
		CEnvPoint Point;
		Point.m_Time = 459423;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 1024;
		Point.m_aValues[1] = 1024;
		Point.m_aValues[2] = 1024;
		Point.m_aValues[3] = 1024;
		pAquarell->m_lPoints.add(Point);
	}

	{//Point #4
		CEnvPoint Point;
		Point.m_Time = 720000;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 1024;
		Point.m_aValues[1] = 1024;
		Point.m_aValues[2] = 1024;
		Point.m_aValues[3] = 1024;
		pAquarell->m_lPoints.add(Point);
	}

	{//Point #5
		CEnvPoint Point;
		Point.m_Time = 1031338;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 1024;
		Point.m_aValues[1] = 1024;
		Point.m_aValues[2] = 1024;
		Point.m_aValues[3] = 1024;
		pAquarell->m_lPoints.add(Point);
	}

	{//Point #6
		CEnvPoint Point;
		Point.m_Time = 1084408;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 1024;
		Point.m_aValues[1] = 1024;
		Point.m_aValues[2] = 1024;
		Point.m_aValues[3] = 246;
		pAquarell->m_lPoints.add(Point);
	}

	{//Point #7
		CEnvPoint Point;
		Point.m_Time = 1133819;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 1024;
		Point.m_aValues[1] = 1024;
		Point.m_aValues[2] = 1024;
		Point.m_aValues[3] = 1024;
		pAquarell->m_lPoints.add(Point);
	}

	{//Point #8
		CEnvPoint Point;
		Point.m_Time = 1440000;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 1024;
		Point.m_aValues[1] = 1024;
		Point.m_aValues[2] = 1024;
		Point.m_aValues[3] = 1024;
		pAquarell->m_lPoints.add(Point);
	}

	pGenerator->AddEnvelopeTemplate(pAquarell, "Aquarell");
}

void SunColor(CGenerator *pGenerator)
{
	CGenEnvelope *pSunColor = new CGenEnvelope(4);
	pSunColor->m_Synchronized = true;

	{//Point #0
		CEnvPoint Point;
		Point.m_Time = 0;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 0;
		Point.m_aValues[1] = 0;
		Point.m_aValues[2] = 0;
		Point.m_aValues[3] = 11;
		pSunColor->m_lPoints.add(Point);
	}

	{//Point #1
		CEnvPoint Point;
		Point.m_Time = 319235;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 0;
		Point.m_aValues[1] = 0;
		Point.m_aValues[2] = 0;
		Point.m_aValues[3] = 11;
		pSunColor->m_lPoints.add(Point);
	}

	{//Point #2
		CEnvPoint Point;
		Point.m_Time = 378990;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 993;
		Point.m_aValues[1] = 351;
		Point.m_aValues[2] = 0;
		Point.m_aValues[3] = 1024;
		pSunColor->m_lPoints.add(Point);
	}

	{//Point #3
		CEnvPoint Point;
		Point.m_Time = 459423;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 0;
		Point.m_aValues[1] = 0;
		Point.m_aValues[2] = 0;
		Point.m_aValues[3] = 11;
		pSunColor->m_lPoints.add(Point);
	}

	{//Point #4
		CEnvPoint Point;
		Point.m_Time = 1031338;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 0;
		Point.m_aValues[1] = 0;
		Point.m_aValues[2] = 0;
		Point.m_aValues[3] = 11;
		pSunColor->m_lPoints.add(Point);
	}

	{//Point #5
		CEnvPoint Point;
		Point.m_Time = 1084408;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 982;
		Point.m_aValues[1] = 320;
		Point.m_aValues[2] = 0;
		Point.m_aValues[3] = 1024;
		pSunColor->m_lPoints.add(Point);
	}

	{//Point #6
		CEnvPoint Point;
		Point.m_Time = 1133819;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 0;
		Point.m_aValues[1] = 0;
		Point.m_aValues[2] = 0;
		Point.m_aValues[3] = 11;
		pSunColor->m_lPoints.add(Point);
	}

	{//Point #7
		CEnvPoint Point;
		Point.m_Time = 1440000;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 0;
		Point.m_aValues[1] = 0;
		Point.m_aValues[2] = 0;
		Point.m_aValues[3] = 11;
		pSunColor->m_lPoints.add(Point);
	}

	pGenerator->AddEnvelopeTemplate(pSunColor, "SunColor");
}

void Stars(CGenerator *pGenerator)
{
	CGenEnvelope *pStars = new CGenEnvelope(4);
	pStars->m_Synchronized = true;

	{//Point #0
		CEnvPoint Point;
		Point.m_Time = 0;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 1024;
		Point.m_aValues[1] = 1024;
		Point.m_aValues[2] = 1024;
		Point.m_aValues[3] = 768;
		pStars->m_lPoints.add(Point);
	}

	{//Point #1
		CEnvPoint Point;
		Point.m_Time = 204534;
		Point.m_Curvetype = 3;
		Point.m_aValues[0] = 1024;
		Point.m_aValues[1] = 1024;
		Point.m_aValues[2] = 1024;
		Point.m_aValues[3] = 768;
		pStars->m_lPoints.add(Point);
	}

	{//Point #2
		CEnvPoint Point;
		Point.m_Time = 489986;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 1024;
		Point.m_aValues[1] = 1024;
		Point.m_aValues[2] = 1024;
		Point.m_aValues[3] = 11;
		pStars->m_lPoints.add(Point);
	}

	{//Point #3
		CEnvPoint Point;
		Point.m_Time = 919000;
		Point.m_Curvetype = 2;
		Point.m_aValues[0] = 1024;
		Point.m_aValues[1] = 1024;
		Point.m_aValues[2] = 1024;
		Point.m_aValues[3] = 11;
		pStars->m_lPoints.add(Point);
	}

	{//Point #4
		CEnvPoint Point;
		Point.m_Time = 1278620;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 1024;
		Point.m_aValues[1] = 1024;
		Point.m_aValues[2] = 1024;
		Point.m_aValues[3] = 768;
		pStars->m_lPoints.add(Point);
	}

	{//Point #5
		CEnvPoint Point;
		Point.m_Time = 1440000;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 1024;
		Point.m_aValues[1] = 1024;
		Point.m_aValues[2] = 1024;
		Point.m_aValues[3] = 768;
		pStars->m_lPoints.add(Point);
	}

	pGenerator->AddEnvelopeTemplate(pStars, "Stars");
}

void PlanetRotation(CGenerator *pGenerator)
{
	CGenEnvelope *pPlanetRotation = new CGenEnvelope(3);
	pPlanetRotation->m_Synchronized = true;

	{//Point #0
		CEnvPoint Point;
		Point.m_Time = 0;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 0;
		Point.m_aValues[1] = 0;
		Point.m_aValues[2] = 0;
		Point.m_aValues[3] = 0;
		pPlanetRotation->m_lPoints.add(Point);
	}

	{//Point #1
		CEnvPoint Point;
		Point.m_Time = 1440000;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 0;
		Point.m_aValues[1] = 0;
		Point.m_aValues[2] = 368640;
		Point.m_aValues[3] = 0;
		pPlanetRotation->m_lPoints.add(Point);
	}

	pGenerator->AddEnvelopeTemplate(pPlanetRotation, "PlanetRotation");
}

void Clouds(CGenerator *pGenerator)
{
	CGenEnvelope *pClouds = new CGenEnvelope(4);
	pClouds->m_Synchronized = true;

	{//Point #0
		CEnvPoint Point;
		Point.m_Time = 0;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 1024;
		Point.m_aValues[1] = 1024;
		Point.m_aValues[2] = 1024;
		Point.m_aValues[3] = 11;
		pClouds->m_lPoints.add(Point);
	}

	{//Point #1
		CEnvPoint Point;
		Point.m_Time = 343132;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 1024;
		Point.m_aValues[1] = 1024;
		Point.m_aValues[2] = 1024;
		Point.m_aValues[3] = 11;
		pClouds->m_lPoints.add(Point);
	}

	{//Point #2
		CEnvPoint Point;
		Point.m_Time = 457613;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 1024;
		Point.m_aValues[1] = 1024;
		Point.m_aValues[2] = 1024;
		Point.m_aValues[3] = 768;
		pClouds->m_lPoints.add(Point);
	}

	{//Point #3
		CEnvPoint Point;
		Point.m_Time = 910000;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 1024;
		Point.m_aValues[1] = 1024;
		Point.m_aValues[2] = 1024;
		Point.m_aValues[3] = 768;
		pClouds->m_lPoints.add(Point);
	}

	{//Point #4
		CEnvPoint Point;
		Point.m_Time = 1117669;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 1024;
		Point.m_aValues[1] = 1024;
		Point.m_aValues[2] = 1024;
		Point.m_aValues[3] = 11;
		pClouds->m_lPoints.add(Point);
	}

	{//Point #5
		CEnvPoint Point;
		Point.m_Time = 1440000;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 1024;
		Point.m_aValues[1] = 1024;
		Point.m_aValues[2] = 1024;
		Point.m_aValues[3] = 11;
		pClouds->m_lPoints.add(Point);
	}

	pGenerator->AddEnvelopeTemplate(pClouds, "Clouds");
}

void Darkening(CGenerator *pGenerator)
{
	CGenEnvelope *pDarkening = new CGenEnvelope(4);
	pDarkening->m_Synchronized = true;

	{//Point #0
		CEnvPoint Point;
		Point.m_Time = 0;
		Point.m_Curvetype = 4;
		Point.m_aValues[0] = 1;
		Point.m_aValues[1] = 1;
		Point.m_aValues[2] = 1;
		Point.m_aValues[3] = 379;
		pDarkening->m_lPoints.add(Point);
	}

	{//Point #1
		CEnvPoint Point;
		Point.m_Time = 720000;
		Point.m_Curvetype = 4;
		Point.m_aValues[0] = 1;
		Point.m_aValues[1] = 1;
		Point.m_aValues[2] = 1;
		Point.m_aValues[3] = 11;
		pDarkening->m_lPoints.add(Point);
	}

	{//Point #2
		CEnvPoint Point;
		Point.m_Time = 1440000;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 1;
		Point.m_aValues[1] = 1;
		Point.m_aValues[2] = 1;
		Point.m_aValues[3] = 379;
		pDarkening->m_lPoints.add(Point);
	}

	pGenerator->AddEnvelopeTemplate(pDarkening, "Darkening");
}

void Water(CGenerator *pGenerator)
{
	CGenEnvelope *pWater = new CGenEnvelope(3);
	pWater->m_Synchronized = true;

	{//Point #0
		CEnvPoint Point;
		Point.m_Time = 0;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 0;
		Point.m_aValues[1] = 0;
		Point.m_aValues[2] = 0;
		Point.m_aValues[3] = 0;
		pWater->m_lPoints.add(Point);
	}

	{//Point #1
		CEnvPoint Point;
		Point.m_Time = 1380;
		Point.m_Curvetype = 1;
		Point.m_aValues[0] = 32768;
		Point.m_aValues[1] = 0;
		Point.m_aValues[2] = 0;
		Point.m_aValues[3] = 0;
		pWater->m_lPoints.add(Point);
	}

	pGenerator->AddEnvelopeTemplate(pWater, "Water");
}

void Snow(CGenerator *pGenerator)
{
	CGenEnvelope *pSnow = new CGenEnvelope(3);
	pSnow->m_Synchronized = true;

	const int NumPoints = 50;
	for (int i = 0; i < 50; i++)
	{
		float f = i / (float)(NumPoints-1);
		CEnvPoint Point;
		Point.m_Time = (int)(230000.0f * f);
		Point.m_Curvetype = 4;
		Point.m_aValues[0] = 0;
		Point.m_aValues[1] = (int)(11059200.0f * f);
		Point.m_aValues[2] = i % 2 ? -65000 : 65000;
		Point.m_aValues[3] = 0;
		pSnow->m_lPoints.add(Point);
	}

	pGenerator->AddEnvelopeTemplate(pSnow, "Snow");
}
void CGenerator::EnvelopeTemplates()
{
	BackColor(this);
	Aquarell(this);
	SunColor(this);
	Stars(this);
	PlanetRotation(this);
	Clouds(this);
	Darkening(this);
	Water(this);
	Snow(this);
}