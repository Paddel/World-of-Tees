#ifndef GAME_GENERATOR_AUTO_MAP_H
#define GAME_GENERATOR_AUTO_MAP_H

#include <base/tl/array.h>

class IStorage;

class CGenAutoMapper
{
	struct CPosRule
	{
		int m_X;
		int m_Y;
		int m_Value;
		bool m_IndexValue;

		enum
		{
			EMPTY=0,
			FULL,
			WATER,
		};
	};

	struct CIndexRule
	{
		int m_ID;
		array<CPosRule> m_aRules;
		int m_Flag;
		int m_RandomValue;
		bool m_BaseTile;
	};

	struct CConfiguration
	{
		array<CIndexRule> m_aIndexRules;
		char m_aName[128];
	};

public:
	CGenAutoMapper(IStorage *pStorage);

	void Load(const char* pTileName);
	void Proceed(class CGenLayerTiles *pLayer, class CGenLayerTiles *pGameLayer, int ConfigID);
	void Proceed(class CGenLayerTiles *pLayer, int ConfigID);

	int ConfigNamesNum() { return m_lConfigs.size(); }
	const char* GetConfigName(int Index);

	const bool IsLoaded() { return m_FileLoaded; }
	IStorage *Storage() const { return m_pStorage; }
private:
	array<CConfiguration> m_lConfigs;
	IStorage *m_pStorage;
	bool m_FileLoaded;
};


#endif
