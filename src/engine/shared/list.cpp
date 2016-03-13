
#include <base/system.h>

#include "list.h"

CList::CList()
{
	//m_pNextItem = new CListEnd();
}

CList *CList::ListGet(int Index)
{
	if(Index <= 0)
		return this;
	else
		return ListGet(Index-1);
}

void CList::ListAdd(CList *pNew)
{
	m_pNextItem->ListAdd(pNew);
}

void CList::ListRemove(int Index)
{
	if(Index == 1)
		m_pNextItem = m_pNextItem->ListGetNext();
	else
		ListRemove(Index-1);
}

int CList::ListLength()
{
	return m_pNextItem->ListLength()+1;
}

CList *CListEnd::ListGet(int Index)
{
	return 0x0;
}

void CListEnd::ListAdd(CList *pNew)
{
	m_pNextItem = pNew;
}

int CListEnd::ListLength()
{
	return 0;
}