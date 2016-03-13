#pragma once

class CRemote
{
public:

	enum
	{
		REMOTETYPE_DOOR=0,
		REMOTETYPE_BOLDER,
	};

private:
	int m_Type;
	int m_ID;

public:
	CRemote(int Type, int ID) : m_Type(Type), m_ID(ID) {}

	int GetID() { return m_ID; }
	int GetType() { return m_Type; }

	virtual void Activate() = 0;
};