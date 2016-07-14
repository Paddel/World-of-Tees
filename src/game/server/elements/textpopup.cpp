
#include <base/stringseperation.h>
#include <engine/server.h>
#include <game/server/gamecontext.h>
#include <engine/textrender.h>

#include "textpopup.h"

char *str_create_line(char *pStr)
{
	int len = str_length(pStr);
	for(int i = 0; i < len; i++)
	{
		char c = pStr[i];
		if(c == '\n')
		{
			pStr[i] = 0;
			break;
		}
	}
	return pStr;
}

CPage::CPage(char *pText, int LineSkips, void *pInfo, CDecision aDecision[MAX_DECISIONS])
{
	str_copy(m_aText, pText, sizeof(m_aText));
	m_LineSkips = LineSkips;
	m_pInfo = pInfo;
	m_NumDecisions = 0;
	m_CurrentDecision = 0;

	if(aDecision)
	{
		mem_copy(m_aDecision, aDecision, sizeof(CDecision)*MAX_DECISIONS);
		for(int i = 0; i < MAX_DECISIONS; i++)
		{
			if(m_aDecision[i].m_Line == -1)
				break;
			m_NumDecisions = i+1;
		}
	}
}

CTextPopup::CTextPopup(CGameContext *pGameServer, int ClientID)
{
	m_pGameServer = pGameServer;
	m_pServer = pGameServer->Server();
	m_ClientID = ClientID;

	m_pPages.clear();
	m_PopupTime = 0;
	m_CurrentPage = 0;
}

void CTextPopup::Tick()
{
	if(Active() && m_PopupTime < Server()->Tick())
	{
		SendPopup();
		m_PopupTime = Server()->Tick()+Server()->TickSpeed()*0.75;
	}
}

void CTextPopup::AddText(char *pText, int LineSkips, void *pInfo, CDecision m_aDecision[MAX_DECISIONS])
{
	m_PopupTime = 0;

	m_pPages.add(new CPage(pText, LineSkips, pInfo, m_aDecision));

	if(m_pPages.size() == 1)
	{
		m_CurrentPage = 0;
		m_AppearTime = Server()->Tick()+Server()->TickSpeed()*0.75f;
	}
}

void CTextPopup::OnLeftClick()
{
	if(!Active())
		return;

	CPage *pPage = CurrentPage();
	if(!pPage || pPage->NumDecisions())
		return;

	m_PopupTime = 0;
	m_CurrentPage = clamp(m_CurrentPage-1, 0 , m_pPages.size()-1);
}

void CTextPopup::OnRightClick()
{
	if(!Active())
		return;

	CPage *pPage = CurrentPage();
	if(!pPage || pPage->NumDecisions())
		return;

	m_PopupTime = 0;
	int PrevPage = m_CurrentPage;
	m_CurrentPage = clamp(m_CurrentPage+1, 0 , m_pPages.size()-1);
	if(m_CurrentPage == PrevPage && m_AppearTime < Server()->Tick())
		Clear();
}

void CTextPopup::OnJump()
{
	if(!Active())
		return;

	CPage *pPage = CurrentPage();
	if(!pPage || pPage->NumDecisions() == 0)
		return;

	CDecision *pDecision = &pPage->m_aDecision[pPage->m_CurrentDecision];
	if(pDecision->m_Line != -1 && pDecision->m_Func)
		pDecision->m_Func(pPage->GetInfo(), m_ClientID);

	m_pPages.remove_index(m_CurrentPage);
	m_PopupTime = 0;

	CCharacter *pChr = GameServer()->GetPlayerChar(m_ClientID);
	if(pChr)
		pChr->Core()->m_Jumped = 1;

	if(m_pPages.size() == 0)
		Clear();
}

void CTextPopup::OnMoveRight()
{
	if(!Active())
		return;

	CPage *pPage = CurrentPage();
	if(!pPage || pPage->NumDecisions() == 0)
		return;

	pPage->m_CurrentDecision = clamp(pPage->m_CurrentDecision+1, 0 , pPage->NumDecisions()-1);
	m_PopupTime = 0;
}

void CTextPopup::OnMoveLeft()
{
	if(!Active())
		return;

	CPage *pPage = CurrentPage();
	if(!pPage || pPage->NumDecisions() == 0)
		return;

	pPage->m_CurrentDecision = clamp(pPage->m_CurrentDecision-1, 0 , pPage->NumDecisions()-1);
	m_PopupTime = 0;
}

void CTextPopup::Clear()
{
	m_pPages.clear();
	m_CurrentPage = 0;
	m_PopupTime = 0;

	CNetMsg_Sv_Motd Msg;
	Msg.m_pMessage = "";
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, m_ClientID);
}

bool CTextPopup::Active()
{
	return m_pPages.size() > 0;
}

CPage *CTextPopup::CurrentPage()
{
	if(m_CurrentPage < 0 || m_CurrentPage >= m_pPages.size())
		return NULL;
	return m_pPages[m_CurrentPage];
}

void CTextPopup::SendPopup()
{
	if(Active() == false || m_pPages.size() <= 0)
		return;
	CPage *pPage = CurrentPage();
	if(!pPage)
		return;

	char aText[512];
	CreatePopupText(aText, sizeof(aText), pPage);

	CNetMsg_Sv_Motd Msg;
	Msg.m_pMessage = aText;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, m_ClientID);
}

void CTextPopup::CreatePopupText(char *pStr, int Size, CPage *pPage)
{
	mem_zero(pStr, Size);
	str_fcat(pStr, Size, "Page %i/%i\n", m_CurrentPage+1, m_pPages.size());
	str_fcat(pStr, Size, "---------------------------------------------------\n");

	{
		char aText[MAX_TEXT_LENGTH];
		str_copy(aText, pPage->Text(), MAX_TEXT_LENGTH);
		char *pText = aText;
		const char *pLine = 0;
		int CurLine = 0;
		while(pText)
		{
			pLine = GetSepStr('\n', &pText);

			for(int i = 0; i < MAX_DECISIONS; i++)
			{
				if(pPage->m_aDecision[i].m_Line == CurLine)
				{
					if(pPage->m_CurrentDecision == i)
						str_fcat(pStr, Size, "-> ");
					else
						str_fcat(pStr, Size, "o ");
				}
			}

			str_fcat(pStr, Size, "%s\n", pLine);
			CurLine++;
		}
	}

	for(int i = 0; i < pPage->LineSkips(); i++)
		str_fcat(pStr, Size, "\n");

	str_fcat(pStr, Size, "---------------------------------------------------\n");
	if(pPage->NumDecisions())
	{
		str_fcat(pStr, Size, " Move right/left change selection\n");
		str_fcat(pStr, Size, " Jump to select\n");
	}
	else
	{
		str_fcat(pStr, Size, " Fire/Hook to turn over Page\n");
		str_fcat(pStr, Size, "\n");
	}
}