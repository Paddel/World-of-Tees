
#include <game/server/gamecontext.h>

#include "questionhandler.h"

CQuestion::CQuestion(void *pOwner, int Type, pClientFunc pYesFunc, pClientFunc pNoFunc, pClientFunc pNoAnswerFunc, int Time, char *pText, char *pReason)
{
	m_pOwner = pOwner;
	m_Type = Type;
	m_pYesFunc = pYesFunc;
	m_pNoFunc = pNoFunc;
	m_pNoAnswerFunc = *pNoAnswerFunc;
	m_Time = Time;
	str_copy(m_aText, pText, sizeof(m_aText));
	str_copy(m_aReason, pReason, sizeof(m_aReason));
}

CQuestionHandler::CQuestionHandler(CGameContext *pGameServer, int Owner)
{
	m_pGameServer = pGameServer;
	m_pServer = pGameServer->Server();
	m_Owner = Owner;
}

void CQuestionHandler::Ask(void *pOwner, int Type, pClientFunc pYesFunc, pClientFunc pNoFunc, char *pText, char *pReason)
{
	Ask(pOwner, Type, pYesFunc, pNoFunc, NULL, -1, pText, pReason);
}

void CQuestionHandler::Ask(void *pOwner, int Type, pClientFunc pYesFunc, pClientFunc pNoFunc, pClientFunc pNoAnswerFunc, int Time, char *pText, char *pReason)
{
	for(int i = 0; i < m_pQuestions.size(); i++)
	{
		if(m_pQuestions[i]->GetType() == Type)
			return;
	}
	m_pQuestions.add( new CQuestion(pOwner, Type, pYesFunc, pNoFunc, pNoAnswerFunc, Time, pText, pReason) );

	if(m_pQuestions.size() == 1)
		Update();
}

void CQuestionHandler::AnswerYes()
{
	if(!m_pQuestions.size())
		return;

	void *pOwner = m_pQuestions[0]->GetOwner();
	pClientFunc Func = m_pQuestions[0]->m_pYesFunc;
	CloseQuestion();

	if(Func)
		Func(pOwner, m_Owner);
}

void CQuestionHandler::AnswerNo()
{
	if(!m_pQuestions.size())
		return;

	void *pOwner = m_pQuestions[0]->GetOwner();
	pClientFunc Func = m_pQuestions[0]->m_pNoFunc;
	CloseQuestion();

	if(Func)
		Func(pOwner, m_Owner);
}

void CQuestionHandler::CloseQuestion()
{
	if(!m_pQuestions.size())
		return;

	m_pQuestions.remove_index(0);
	Update();
}

void CQuestionHandler::Update()
{
	CNetMsg_Sv_VoteSet Msg;
	if(m_pQuestions.size())
	{//AskNext Question
		int Time = m_pQuestions[0]->GetTime();
		Msg.m_Timeout = Time==-1?1:Time;
		Msg.m_pDescription = m_pQuestions[0]->GetText();
		Msg.m_pReason = m_pQuestions[0]->GetReason();
	}
	else
	{
		Msg.m_Timeout = 0;
		Msg.m_pDescription = "";
		Msg.m_pReason = "";
	}
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, m_Owner);

	m_TimeCounter = Server()->Tick()+Server()->TickSpeed();
}

void CQuestionHandler::EndQuestion(int Type)
{
	for(int i = 0; i < m_pQuestions.size(); i++)
	{
		if(m_pQuestions[i]->GetType() == Type)
		{
			m_pQuestions.remove_index(i);
			if(i == 0)
				Update();
			break;
		}
	}
}

void CQuestionHandler::Tick()
{
	if(!m_pQuestions.size())
		return;

	if(m_pQuestions[0]->GetTime() != -1 && m_TimeCounter < Server()->Tick())
	{
		m_pQuestions[0]->ReduceTime();

		if(m_pQuestions[0]->GetTime() == 0)
		{
			void *pOwner = m_pQuestions[0]->GetOwner();
			pClientFunc Func = m_pQuestions[0]->m_pNoAnswerFunc;
			CloseQuestion();
			if(Func)
				Func(pOwner, m_Owner);
		}

		m_TimeCounter = Server()->Tick()+Server()->TickSpeed();
	}

	//remove dis :D
	int Total = 16;
	int Yes = (sin(Server()->Tick()*0.1f+pi)+1)*8;
	int No = (sin(Server()->Tick()*0.1f)+1)*8;
	CNetMsg_Sv_VoteStatus Msg = {0};
	Msg.m_Total = Total;
	Msg.m_Yes = Yes;
	Msg.m_No = No;
	Msg.m_Pass = 1;

	if(m_Last != No)
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, m_Owner);

	m_Last = No;
}