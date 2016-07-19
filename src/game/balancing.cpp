
#include "balancing.h"

int Character_NeededExperience(int Level)
{
	return 1000+(Level-1)*500;
}

int Character_MaxMana(int Level)
{
	return (Level-1)*20;
}

int Character_MaxHealth(int Level)
{
	return 170+(Level-1)*22;
}

int Town_Ticket_Costs(int Ticket)
{
	switch(Ticket)
	{
	case 1: return 0;
	case 2:	return 350;
	}
	return 0;
}

int Town_Ticket_Level(int Ticket)
{
	switch(Ticket)
	{
	case 1: return 1;
	case 2:	return 2;
	}
	return 0;
}