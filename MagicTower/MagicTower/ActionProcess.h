#ifndef ACTION_PROCESS_H
#define ACTION_PROCESS_H

#include "commonTools.h"
#include "Character.h"
#include "CreatureManager.h"

typedef eNotification (Character::*LPActionFunc)(Character*,DWORD);

struct Action
{
	eNotification m_eNotify;
	Character* m_pCast;
	Character* m_pTarget;
	DWORD m_dwTime;
	Action()
	{
		m_eNotify = eNotify_Success;
		m_pCast = NULL;
		m_pTarget = NULL;
		m_dwTime = 0;
	}
	Action(eNotification notify,Character* cast,Character* target,DWORD time)
	{
		m_eNotify = notify;
		m_pCast = cast;
		m_pTarget = target;
		m_dwTime = time;
	}
	void Clear()
	{
		m_eNotify = eNotify_Success;
		m_pCast = NULL;
		m_pTarget = NULL;
		m_dwTime = 0;
	}
};

enum eActionState
{
	eActionState_PickAction = 0,
	eActionState_Process,
	eActionState_End,
};

class ActionProcess : public Singleton<ActionProcess>
{
public:
	ActionProcess()
	{
		m_lAction.clear();
		m_eActionState = eActionState_PickAction;
		m_iCurAction.Clear();
		m_nLeftTime = -1;
	}
	~ActionProcess(){}

	void PushAction(eNotification notify,Character* cast,Character* target,DWORD time)
	{
		if(!cast || !target)
			return;
		m_lAction.push_back(Action(notify,cast,target,time));
	}
	void PopAction(){m_lAction.pop_front();}
	void TimeUp(DWORD leftTime)
	{
		m_nLeftTime = leftTime;
	}
	void Update()
	{
		if (m_eActionState == eActionState_PickAction)
		{
			if(!m_lAction.empty())
			{
				Action action = m_lAction.front();
				switch (action.m_eNotify)
				{
				case eNotify_TowardToAttacker:
					{
						action.m_pCast->TowardToAttacker(eNotify_TowardToAttacker,action.m_pTarget,action.m_dwTime);
						if(action.m_dwTime == 0)
							m_eActionState = eActionState_End;
						else
							m_eActionState = eActionState_Process;
					}
					break;
				case eNotify_Crit:
					{
						action.m_pCast->Crit(eNotify_Crit,action.m_pTarget,action.m_dwTime);
						m_eActionState = eActionState_Process;
					}
					break;
				case eNotify_Attack:
					{
						action.m_pCast->Attack(eNotify_Attack,action.m_pTarget,0);
						action.m_pTarget->Attacked(eNotify_Attack,action.m_pCast,action.m_dwTime);

						m_eActionState = eActionState_Process;
					}
					break;
				case eNotify_AttackDefend:
					{
						action.m_pCast->Attack(eNotify_Attack,action.m_pTarget,0);
						action.m_pTarget->Defend(eNotify_Attack,action.m_pCast,action.m_dwTime);

						m_eActionState = eActionState_Process;
					}
					break;
				case eNotify_Dead:
					{
						action.m_pTarget->Dead(eNotify_Dead,action.m_pCast,action.m_dwTime);
						m_eActionState = eActionState_Process;
					}
					break;
				case eNotify_FinishAttack:
					{
						m_eActionState = eActionState_End;
					}
					break;
				default:
					break;
				}

				
				m_iCurAction = action;
			}
		}
		else if (m_eActionState == eActionState_Process)
		{
			Action action = m_lAction.front();
			switch (action.m_eNotify)
			{
			case eNotify_Crit:
				{
					int nTempColor = (int)(255*m_nLeftTime/1000);
					DWORD dwCritColor = (nTempColor + (nTempColor << 8)) | 0xFFFF0000;
					action.m_pCast->ChangeColor(dwCritColor);
				}
				break;
			default:
				break;
			}
			if(m_nLeftTime==0)
			{
				m_nLeftTime = -1;
				m_eActionState = eActionState_End;
			}
		}
		else if (m_eActionState == eActionState_End)
		{
			Action action = m_lAction.front();
			switch (action.m_eNotify)
			{
			case eNotify_TowardToAttacker:
				{
					action.m_pCast->GetCharacterState() = eCharacterState_Stand;
				}
				break;
			case eNotify_Crit:
				{
					action.m_pCast->ChangeColor(0xFFFFFFFF);
				}
				break;
			case eNotify_Attack:
				{
					action.m_pCast->ResetFrame();
					action.m_pTarget->ResetFrame();
					action.m_pTarget->GetHP() -= action.m_pCast->GetPreHurt();
					action.m_pTarget->GetCharacterState() = eCharacterState_Stand;
				}
				break;
			case eNotify_AttackDefend:
				{
					action.m_pCast->ResetFrame();
					action.m_pTarget->ResetFrame();
					action.m_pTarget->GetCharacterState() = eCharacterState_Stand;
				}
				break;
			case eNotify_Dead:
				{
					action.m_pTarget->GetDead() = true;
				}
				break;
			case eNotify_FinishAttack:
				{
					action.m_pCast->SetFinish(true);
				}
				break;
			default:
				break;
			}
			m_iCurAction.Clear();
			m_eActionState = eActionState_PickAction;
			PopAction();
		}


	}
private:
	std::list<Action> m_lAction;
	Action m_iCurAction;
	eActionState m_eActionState;
	int m_nLeftTime; 
};

#endif