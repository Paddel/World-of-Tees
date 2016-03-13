#pragma once

class CList
{
protected:
	CList *m_pNextItem;

public:
	CList();

	virtual CList *ListGetNext() { return m_pNextItem; }
	virtual CList *ListGet(int Index);
	virtual void ListAdd(CList *pNew);
	virtual void ListRemove(int Index);
	virtual int ListLength();
};

class CListEnd : CList
{
public:
	virtual CList *ListGetNext() { return new CListEnd(); }
	virtual CList *ListGet(int Index);
	virtual void ListAdd(CList *pNew);
	virtual void ListRemove(int Index) {}
	virtual int ListLength();
};