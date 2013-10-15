#include "Character.h"
#include "App.h"
#include "MapManager.h"
#include "TexManager.h"
#include "CreatureManager.h"
#include "UI.h"
#include "ActionProcess.h"
#include "ConfigManager.h"


Character::Character(void)
{
	m_pAnimation = NULL;
	m_mCharTex.clear();
	m_nMapLevel = 0;
	m_nID = 0;
	m_nNum = 0;
	m_iBlock.xpos = 0;
	m_iBlock.ypos = 0;
	m_iBlock.attri = 0;
	m_bCanMove = true;
	m_nMoveAbility = 0;
	m_bFinishAct = false;
	m_nCamp = eCamp_Neutral;
	m_eCharState = eCharacterState_Stand;
	m_eActionStage = eActionStage_WaitStage;
	m_nActionTime = 0;
	m_eAttackRange = (eAttackRange)(g_RandomInt(0,eAttackRange_Arrow));
	m_vDefaultSkillList.clear();
	m_vNewSkillList.clear();
}

Character::~Character(void)
{
	gSafeDelete(m_pAnimation);
	gSafeDelete(m_pItemSpr);
}

void Character::Init(int _Level,int _ID,int _Num,int _Action,Block _block)
{
	if(m_pAnimation)
		gSafeDelete(m_pAnimation);

	m_mCharTex = TexManager::sInstance().GetTex(_ID);
	//初始化失败
	if(m_mCharTex.empty())
		return;

	//右向图片是左向的对称图
	m_pAnimation = new hgeAnimation(m_mCharTex[eActionTex_Walk],1,8,0,(GetTexDir((eDirection)(_Action%4+1))-1)*FLOAT_PIC_SQUARE_WIDTH+6*FLOAT_PIC_SQUARE_WIDTH,FLOAT_PIC_SQUARE_WIDTH,FLOAT_PIC_SQUARE_WIDTH);
	m_nMapLevel = _Level;
	m_nID = _ID;
	m_nNum = _Num;
	m_iBlock = _block;
	m_iOrigBlock = _block;
	m_fStartX = m_fXPos = (MAP_RECT-FLOAT_PIC_SQUARE_WIDTH)/2+MAP_OFF_X +MAP_RECT*m_iBlock.xpos;
	m_fStartY = m_fYPos = MAP_OFF_Y+MAP_RECT*m_iBlock.ypos;
	m_nLeftDistance = 0;
	m_eCurDir = (eDirection)(_Action%4+1);
	m_eOrigDir = m_eCurDir;
	m_eCharState = eCharacterState_Stand;
	//为初始化的人物所在地图块设置属性
	Map* theMap = MapManager::sInstance().GetMap(m_nMapLevel);
	theMap->SetBlockOccupied(_block.xpos,_block.ypos);

	//初始化单位属性(读配置+计算)
	//测试，直接赋值
	char name[50];
	sprintf(name,"%d号小兵",m_nNum);
	m_strName = name;
	m_strKind = ConfigManager::sInstance().GetCreatureInfo().find(_ID)->second.m_strKind;
	m_eAttackType = eAttackType_Normal;
	m_nAttack = 5;
	m_nDefend = 2;
	m_fCrit = 0.8f;
	m_fDodge = 0.2f;
	m_nHP = 10;
	m_nHPMax = 10;
	m_nMP = 10;
	m_nMPMax = 10;
	m_nLevel = 1;
	m_nExp = 0;
	m_nExpTotal = 100;
	m_nPreHurt = 0;
	m_bDead = false;
	m_bCounter = true;
	m_vBuffData.clear();

	m_vDefaultSkillList = ConfigManager::sInstance().GetCreatureSkill(m_strKind);
	m_nCastSkill = -1;

	m_nUseItem = -1;
	m_pItemSpr = new hgeSprite(0,0,0,32,32);
	m_fItemRiseHeight = 0.0f;
	m_nDrawItem = 0;
	m_fDrawItemX = 0.0f;
	m_fDrawItemY = 0.0;
	m_pItemTarget = NULL;

	m_pAnimation->SetMode(HGEANIM_LOOP|HGEANIM_FWD);
	m_pAnimation->Play();
}

eDirection Character::GetTexDir(eDirection dir)
{
	eDirection tempDir = dir;
	if(tempDir == eDirection_Right)
		tempDir = eDirection_Left;

	return tempDir;
}

void Character::Render()
{
	if(m_pAnimation)
	{
		if(/*m_eCharState == eCharacterState_Defense && m_eAttackState == eAttackState_Defending && */m_eCurDir == eDirection_Right)	//由于源图片缺少向右的动作，故需要y轴对称绘制
			m_pAnimation->RenderSymmetry(m_fXPos,m_fYPos,1);
		else
			m_pAnimation->Render(m_fXPos,m_fYPos);
	}

	if (m_nDrawItem && m_pItemSpr)
	{
		m_pItemSpr->Render(m_fDrawItemX + 10,m_fDrawItemY + m_fItemRiseHeight - 20);
	}
}

void Character::Update(float delta)
{
	if(m_eCharState == eCharacterState_Stand/* || (m_eCharState==eCharacterState_Fight && m_eAttackState==eAttackState_Waiting)*/)
		return;

	if (m_nDrawItem!=0)
	{
		//物品上升阶段
		if(m_nDrawItem == 1)
		{
			m_fItemRiseHeight -= delta*80;
			if(m_fItemRiseHeight <=  -40.0f)
			{
				m_fDrawItemX = m_pItemTarget->GetRealX();
				m_fDrawItemY = m_pItemTarget->GetRealY();
				m_nDrawItem = 2;
			}
		}
		//物品下降阶段
		else if (m_nDrawItem == 2)
		{
			m_fItemRiseHeight += delta*80;
			if (m_fItemRiseHeight >= 0.0f)
			{
				m_nDrawItem = 0;
				ActionProcess::sInstance().TimeUp(0);
			}
		}
	}

	if(m_nActionTime > 0)
	{
		m_nActionTime -= (int)(delta*1000);
		ActionProcess::sInstance().TimeUp(m_nActionTime);
		if(m_nActionTime <= 0)
		{
			m_nActionTime = 0;
			ActionProcess::sInstance().TimeUp(0);
		}
	}

	if (m_eCharState == eCharacterState_Walk)
	{
		//禁止超过地图边界
		if ((m_iBlock.xpos<=0 && m_eCurDir==eDirection_Left)
			|| (m_iBlock.xpos>=MAP_WIDTH_NUM-1 && m_eCurDir==eDirection_Right)
			|| (m_iBlock.ypos<=0 && m_eCurDir==eDirection_Up)
			|| (m_iBlock.ypos>=MAP_LENGTH_NUM-1 && m_eCurDir==eDirection_Down))
		{
			m_eCharState = eCharacterState_Stand;
//			m_bFinishAct = true;
			m_nLeftDistance = 0;
			m_lPathDir.clear();
			return;
		}

		//前方如果无法通过则停止
		Map* theMap = MapManager::sInstance().GetCurrentMap();
		if (m_eCurDir==eDirection_Up && !IsCanCross((theMap->GetBlock(m_iBlock.xpos,m_iBlock.ypos-1)->attri))
			|| m_eCurDir==eDirection_Down && !IsCanCross((theMap->GetBlock(m_iBlock.xpos,m_iBlock.ypos+1)->attri))
			||m_eCurDir==eDirection_Left && !IsCanCross((theMap->GetBlock(m_iBlock.xpos-1,m_iBlock.ypos)->attri))
			||m_eCurDir==eDirection_Right && !IsCanCross((theMap->GetBlock(m_iBlock.xpos+1,m_iBlock.ypos)->attri)))
		{
			m_eCharState = eCharacterState_Stand;
//			m_bFinishAct = true;
			m_nLeftDistance = 0;
			m_lPathDir.clear();
			return;
		}

		if(abs(m_fXPos-m_fStartX) >= MAP_RECT || abs(m_fYPos-m_fStartY) >= MAP_RECT)
		{
			m_nLeftDistance -= 1;

			//更新以前地图块的属性，以及达到的新的地图块的属性
			Block* oldBlock = theMap->GetBlock(m_iBlock.xpos,m_iBlock.ypos);
			if(oldBlock!=NULL)
				setOccupied((oldBlock->attri),0);
			Block* newBlock = NULL;
			//更新地图位置，并且矫正偏移
			if (m_eCurDir == eDirection_Right)
			{
				newBlock = theMap->GetBlock(m_iBlock.xpos+1,m_iBlock.ypos);
				m_iBlock = (newBlock==NULL)?m_iBlock:(*newBlock);
				m_fXPos = (MAP_RECT-FLOAT_PIC_SQUARE_WIDTH)/2+MAP_OFF_X +MAP_RECT*m_iBlock.xpos;
			}
			else if(m_eCurDir == eDirection_Left)
			{
				newBlock = theMap->GetBlock(m_iBlock.xpos-1,m_iBlock.ypos);
				m_iBlock = (newBlock==NULL)?m_iBlock:(*newBlock);
				m_fXPos = (MAP_RECT-FLOAT_PIC_SQUARE_WIDTH)/2+MAP_OFF_X +MAP_RECT*m_iBlock.xpos;
			}
			else if(m_eCurDir == eDirection_Up)
			{
				newBlock = theMap->GetBlock(m_iBlock.xpos,m_iBlock.ypos-1);
				m_iBlock = (newBlock==NULL)?m_iBlock:(*newBlock);
				m_fYPos = MAP_OFF_Y+MAP_RECT*m_iBlock.ypos;
			}
			else if(m_eCurDir == eDirection_Down)
			{
				newBlock = theMap->GetBlock(m_iBlock.xpos,m_iBlock.ypos+1);
				m_iBlock = (newBlock==NULL)?m_iBlock:(*newBlock);
				m_fYPos = MAP_OFF_Y+MAP_RECT*m_iBlock.ypos;
			}
			theMap->SetBlockOccupied(m_iBlock.xpos,m_iBlock.ypos);
			m_fStartX = m_fXPos;
			m_fStartY = m_fYPos;

			//弹出下一个方向
			if(!m_lPathDir.empty())
			{	
				m_eCurDir = m_lPathDir.front();
				m_lPathDir.pop_front();
				m_pAnimation->ResetFrames(0,2*(GetTexDir(m_eCurDir)-1)*FLOAT_PIC_SQUARE_WIDTH,
					FLOAT_PIC_SQUARE_WIDTH,FLOAT_PIC_SQUARE_WIDTH,2,8,true);
			}

			if(m_nLeftDistance == 0)
			{
				m_eCharState = eCharacterState_Stand;
				//m_bFinishAct = true;
				m_eActionStage = eActionStage_HandleStage;
				m_lPathDir.clear();

				//友方单位打开操作界面
				if (m_nCamp == eCamp_Friend)
				{
// 					UIWindow* commandWindow = UISystem::sInstance().GetWindow(eWindowID_Command);
// 					if(commandWindow)
// 					{
// 						commandWindow->SetShow(true);
// 						commandWindow->SetBindChar(this);
// 					}
					UIWindow* commandWindow = UISystem::sInstance().PopUpWindow(eWindowID_Command);
					if(commandWindow)
						commandWindow->SetBindChar(this);
				}
			}
		}

		if(m_nLeftDistance > 0)
		{
			if(m_eCurDir == eDirection_Up)
				m_fYPos -= delta*120;
			else if(m_eCurDir == eDirection_Down)
				m_fYPos += delta*120;
			else if(m_eCurDir == eDirection_Left)
				m_fXPos -= delta*120;
			else if(m_eCurDir == eDirection_Right)
				m_fXPos += delta*120;
		}
	}
/*	else if (m_eCharState == eCharacterState_Fight)
	{
		if(m_eAttackState == eAttackState_Attacking)
		{
			if (m_pAnimation->GetFrame() == 3)
			{
				m_pAnimation->SetTexture(m_mCharTex[eCharacterState_Walk]);
				m_pAnimation->ResetFrames(0,(m_eCurDir-1)*FLOAT_PIC_SQUARE_HEIGHT,
					FLOAT_PIC_SQUARE_WIDTH,FLOAT_PIC_SQUARE_HEIGHT,4,8,false);
				m_pAnimation->SetMode(HGEANIM_LOOP|HGEANIM_FWD);
			}
		}
		else if (m_eAttackState == eAttackState_Criting)
		{
			if(m_dwRecordTime == 0)
				m_dwRecordTime = GetTickCount();
			DWORD currentTime = GetTickCount();
			//为加深暴击视觉效果，单位颜色慢慢变红
			int nTempColor = 255-(int)(255*(currentTime-m_dwRecordTime)/1000);
			DWORD dwCritColor = (nTempColor + (nTempColor << 8)) | 0xFFFF0000;
			m_pAnimation->SetColor(dwCritColor);
			//1秒后再通知攻击者
			if(currentTime >= m_dwRecordTime + 1000)
			{
				if (m_pAnimation->GetFrame() == 0)
				{
					m_pAnimation->SetColor(0xFFFFFFFF);
					Attack();
				}
				m_dwRecordTime = 0;
			}
		}
	}
	else if (m_eCharState == eCharacterState_Defense)
	{
		if (m_eAttackState == eAttackState_Ready)
		{
			DWORD currentTime = GetTickCount();
			if(m_dwRecordTime == 0)
				m_dwRecordTime = GetTickCount();
			//0.2秒后再通知攻击者
			if(currentTime >= m_dwRecordTime + 200)
			{
				if (m_pAnimation->GetFrame() == 0)
				{
					m_eCharState = eCharacterState_Stand;
					CreatureManager::sInstance().Notify(m_nNum,m_nSrc,eNotify_ReadyToBeAttacked,0);
				}
				m_dwRecordTime = 0;
			}
		}
		else if (m_eAttackState == eAttackState_Defending)
		{
			DWORD currentTime = GetTickCount();
			if(m_dwRecordTime == 0)
				m_dwRecordTime = GetTickCount();
			//0.5秒后再通知攻击者
			if(currentTime >= m_dwRecordTime + 500)
			{
				if (m_pAnimation->GetFrame() == 0)
				{
					Character* cast = CreatureManager::sInstance().GetCreature(m_nSrc);
					//是否发动反击且可以攻击到源
					if(m_bCounter && CanHitTarget(cast))
					{
						Counter();			
					}
					else
					{
						m_pAnimation->SetTexture(m_mCharTex[eCharacterState_Walk]);
						m_pAnimation->ResetFrames(0,(m_eCurDir-1)*FLOAT_PIC_SQUARE_HEIGHT,
							FLOAT_PIC_SQUARE_WIDTH,FLOAT_PIC_SQUARE_HEIGHT,4,8,false);
						m_pAnimation->SetMode(HGEANIM_LOOP|HGEANIM_FWD);
						m_eCharState = eCharacterState_Stand;
						//通知攻击者结束行动，这里通知是为了让被攻击者动作可以播完
						CreatureManager::sInstance().Notify(m_nNum,m_nSrc,eNotify_FinishAttack,0);
					}
				}
				m_dwRecordTime = 0;
			}
		}
		else if (m_eAttackState == eAttackState_Attackeding)
		{
			DWORD currentTime = GetTickCount();
			if(m_dwRecordTime == 0)
				m_dwRecordTime = GetTickCount();
			//0.5秒后再通知攻击者
			if(currentTime >= m_dwRecordTime + 500)
			{
				if (m_pAnimation->GetFrame() == 0)
				{
					Character* cast = CreatureManager::sInstance().GetCreature(m_nSrc);
					if(GetHP() <= 0)
					{
						//死亡
						Dead();
					}
					//是否发动反击且可以攻击到源
					else if(m_bCounter && CanHitTarget(cast))
					{
						Counter();			
					}
					else
					{
						m_pAnimation->SetTexture(m_mCharTex[eCharacterState_Walk]);
						m_pAnimation->ResetFrames(0,(m_eCurDir-1)*FLOAT_PIC_SQUARE_HEIGHT,
							FLOAT_PIC_SQUARE_WIDTH,FLOAT_PIC_SQUARE_HEIGHT,4,8,false);
						m_pAnimation->SetMode(HGEANIM_LOOP|HGEANIM_FWD);
						m_eCharState = eCharacterState_Stand;
						//通知攻击者结束行动，这里通知是为了让被攻击者动作可以播完
						CreatureManager::sInstance().Notify(m_nNum,m_nSrc,eNotify_FinishAttack,0);
					}
				}
				m_dwRecordTime = 0;
			}
		}
	}
	else if (m_eCharState == eCharacterState_Dead)
	{
		if(m_eAttackState == eAttackState_Dead)
		{
			if (m_pAnimation->GetFrame() == 3)
			{
				//角色死亡，准备移除
				m_bDead = true;

				//通知攻击者结束行动，这里通知是为了让被攻击者动作可以播完
				CreatureManager::sInstance().Notify(m_nNum,m_nSrc,eNotify_FinishAttack,0);
			}
		}
	}
*/
	if(m_pAnimation)
	{
// 		char test[50];
// 		if(lastFrame != m_ani->GetFrame())
// 		{
// 			sprintf(test,"%d",m_ani->GetFrame());
// 			OutputDebugString(test);
// 			lastFrame = m_ani->GetFrame();
// 		}
// 		if ((m_ani->GetFrame() >= 15 && m_MoveDir==UP))
// 			m_ani->SetFrame((m_MoveDir-1)*4);
// 		else if(m_ani->GetFrame() >= (m_MoveDir*4))
// 			m_ani->SetFrame((m_MoveDir-1)*4);
		
		m_pAnimation->Update(App::sInstance().GetHGE()->Timer_GetDelta());
	}
}

void Character::ResetFrame()
{
	if(m_nHP < 0.3*(float)m_nHPMax)
	{
		m_pAnimation->SetTexture(m_mCharTex[eActionTex_Dead]);
		m_pAnimation->ResetFrames(0,9*FLOAT_PIC_SQUARE_WIDTH,
			FLOAT_PIC_SQUARE_WIDTH,FLOAT_PIC_SQUARE_WIDTH,2,8,true);
	}
	else
	{
		m_pAnimation->SetTexture(m_mCharTex[eActionTex_Stand]);
		m_pAnimation->ResetFrames(0,(GetTexDir(m_eCurDir)-1)*FLOAT_PIC_SQUARE_WIDTH + 6*FLOAT_PIC_SQUARE_WIDTH,
			FLOAT_PIC_SQUARE_WIDTH,FLOAT_PIC_SQUARE_WIDTH,1,8,true);
	}
	m_pAnimation->SetMode(HGEANIM_LOOP|HGEANIM_FWD);
	m_pAnimation->Resume();
	m_eCharState = eCharacterState_Stand;
}

void Character::SetFinish(bool _finish)
{
	m_bFinishAct = _finish;
	if (m_bFinishAct)
	{
		m_pAnimation->SetColor(0xFF59636C);
		m_iOrigBlock.xpos = m_iBlock.xpos;
		m_iOrigBlock.ypos = m_iBlock.ypos;
		m_eOrigDir = m_eCurDir;
		ResetFrame();

		//行动完后检查是否触发剧情
		//检查是否接触触发
		MapManager::sInstance().GetCurrentMap()->IsTriggerTouch(m_nNum);
		//检查是否移动触发
		MapManager::sInstance().GetCurrentMap()->IsTriggerLocation(m_nNum);
	}
}

void Character::CancelMove()
{
	Map* currentMap = MapManager::sInstance().GetCurrentMap();
	//取消现在格子的占据状态
	Block* cannelBlock = currentMap->GetBlock(m_iBlock.xpos,m_iBlock.ypos);
	if(cannelBlock)
		setOccupied(cannelBlock->attri,0);
	m_iBlock.xpos = m_iOrigBlock.xpos;
	m_iBlock.ypos = m_iOrigBlock.ypos;
	m_eCurDir = m_eOrigDir;
	if(m_nHP < 0.3*(float)m_nHPMax)
		m_pAnimation->ResetFrames(0,9*FLOAT_PIC_SQUARE_WIDTH,FLOAT_PIC_SQUARE_WIDTH,FLOAT_PIC_SQUARE_WIDTH,2,8,true);
	else
		m_pAnimation->ResetFrames(0,(GetTexDir(m_eCurDir)-1)*FLOAT_PIC_SQUARE_WIDTH + 6*FLOAT_PIC_SQUARE_WIDTH,
		FLOAT_PIC_SQUARE_WIDTH,FLOAT_PIC_SQUARE_WIDTH,1,8,true);

	//原格子占据状态
	cannelBlock = currentMap->GetBlock(m_iBlock.xpos,m_iBlock.ypos);
	if(cannelBlock)
		setOccupied(cannelBlock->attri,1);

	m_fXPos = (MAP_RECT-FLOAT_PIC_SQUARE_WIDTH)/2+MAP_OFF_X +MAP_RECT*m_iBlock.xpos;
	m_fYPos = MAP_OFF_Y+MAP_RECT*m_iBlock.ypos;
	m_fStartX = m_fXPos;
	m_fStartY = m_fYPos;
}

void	Character::SetMoveAbility(int _ability,Map* map)
{
		m_nMoveAbility = _ability;
// 		if (map)
// 			CreateMoveRange(map);
}

std::vector<Block*> Character::CreateRange(Map* map,int _range,bool bAllBlockInclude)
{
	std::vector<Block*> range;
	if (map)
	{
		int mapWidth = 0,mapLength = 0;
		map->GetWidthLength(mapWidth,mapLength);
		int offX = 0,offY = 0;
		for (int i=m_iBlock.xpos-_range;i<=m_iBlock.xpos+_range;i++)
		{
			if(i >= 0 && i< mapWidth)
			{
				for (int j=m_iBlock.ypos-_range;j<=m_iBlock.ypos+_range;j++)
				{
					if (j >= 0 && j < mapLength)
					{
						offX = abs(i - m_iBlock.xpos);
						offY = abs(j - m_iBlock.ypos);
						if(offX + offY > _range)
							continue;
						if(!bAllBlockInclude && (i==m_iBlock.xpos && j==m_iBlock.ypos))
							continue;
						if (!map->GetBlockOccupied(i,j) || bAllBlockInclude)
						{
							range.push_back(map->GetBlock(i,j));
						}
					}
				}
			}
		}
	}
	return range;
}

eErrorCode Character::Move(int tarX,int tarY)
{
	//单位不可移动
	if (!m_bCanMove) 
	{
		return eErrorCode_CannotMove;
	}

	//当前移动未结束，不可寻路
	if(m_eCharState != eCharacterState_Stand)
	{
		return eErrorCode_NotStandState; 
	}

	MapManager::sInstance().GetCurrentMap()->SetSpecificRange(CreateRange(MapManager::sInstance().GetCurrentMap(),m_nMoveAbility));
	vector<Block*> path = MapManager::sInstance().GetCurrentMap()->FindPath(m_iBlock.xpos,m_iBlock.ypos,tarX,tarY);
	if(!path.empty())
	{
		//将移动保存起来
		eDirection dir = eDirection_None;
		Block* current = path.front();
		Block* next = NULL;
		for (vector<Block*>::iterator it=(++path.begin());it!=path.end();it++)
		{
			next = *it;
			if(next != NULL)
			{
				if(next->xpos == current->xpos)
				{
					if(next->ypos == current->ypos+1)
						Move(eDirection_Down);
					else if(next->ypos == current->ypos-1)
						Move(eDirection_Up);
				}
				else if(next->ypos == current->ypos)
				{
					if(next->xpos == current->xpos+1)
						Move(eDirection_Right);
					else if(next->xpos == current->xpos-1)
						Move(eDirection_Left);
				}
			}
			current = next;
		}
	}
	//没有找到路径，需要结束行动
	else
	{
// 		//找不到路径不应该结束行动
// 		//敌方由于AI算法未健全故可以找不到路径，为了测试方便则直接结束行动
// 		if(m_nCamp == eCamp_Enemy)
// 			m_bFinishAct = true;
		return eErrorCode_NoPath;
	}
	return eErrorCode_Success;
}

//每调用一次move，将会朝着该方向移动一格子
void Character::Move(eDirection dir)
{
	//单位不可移动
	if (!m_bCanMove)
	{
		//m_bFinishAct = true;
		return;
	}

	//只有在stand 或者 walk 时可以移动
	if (m_eCharState != eCharacterState_Stand && m_eCharState != eCharacterState_Walk)
	{
		//m_bFinishAct = true;
		return;
	}

	//无效方向
	if(dir <= eDirection_None || dir > eDirection_End)
	{
		//m_bFinishAct = true;
		return;
	}

	m_lPathDir.push_back(dir);
	m_nLeftDistance++;

	if(m_eCharState == eCharacterState_Stand)
	{
		m_eCurDir = m_lPathDir.front();
		m_pAnimation->SetTexture(m_mCharTex[eCharacterState_Walk]);
		m_pAnimation->ResetFrames(0,2*(GetTexDir(m_eCurDir)-1)*FLOAT_PIC_SQUARE_WIDTH,
			FLOAT_PIC_SQUARE_WIDTH,FLOAT_PIC_SQUARE_WIDTH,2,8,true);
		m_pAnimation->SetMode(HGEANIM_LOOP|HGEANIM_FWD);
		m_lPathDir.pop_front();
		m_eCharState = eCharacterState_Walk;
	}
}

// int Character::TowardToAttacker(int src)
// {
// 	if (m_eCharState == eCharacterState_Stand)
// 	{
// 		m_nSrc = src;
// 		m_eCharState = eCharacterState_Defense;
// 		m_eAttackState = eAttackState_Ready;
// 		Block& block = CreatureManager::sInstance().GetCreature(src)->GetBlock();
// 		if (block.xpos < m_iBlock.xpos)
// 			m_eCurDir = eDirection_Left;
// 		else if(block.xpos > m_iBlock.xpos)
// 			m_eCurDir = eDirection_Right;
// 		else if(block.ypos < m_iBlock.ypos)
// 			m_eCurDir = eDirection_Up;
// 		else
// 			m_eCurDir = eDirection_Down;
// 		m_pAnimation->ResetFrames(0,(m_eCurDir-1)*FLOAT_PIC_SQUARE_HEIGHT,
// 			FLOAT_PIC_SQUARE_WIDTH,FLOAT_PIC_SQUARE_HEIGHT,1,8,false);
// 		m_pAnimation->SetMode(HGEANIM_LOOP|HGEANIM_FWD);
// 
// 		return eNotify_Success;
// 	}
// 
// 	return eNotify_CannotBeAttacked;
// }

void Character::TowardToAttacker(eNotification notify,Character* target,int time)
{
	if (m_eCharState == eCharacterState_Stand)
	{
		m_eNotify = notify;
		m_nActionTime = time;
//		m_dwRecordTime = time;
//		if(time != 0)
//			m_nSrc = target->GetNum();

		m_eCharState = eCharacterState_Fight;
//		m_eAttackState = eAttackState_Ready;
		Block& block = target->GetBlock();
		if (block.xpos < m_iBlock.xpos)
			m_eCurDir = eDirection_Left;
		else if(block.xpos > m_iBlock.xpos)
			m_eCurDir = eDirection_Right;
		else if(block.ypos < m_iBlock.ypos)
			m_eCurDir = eDirection_Up;
		else
			m_eCurDir = eDirection_Down;
		m_pAnimation->ResetFrames(0,(GetTexDir(m_eCurDir)-1)*FLOAT_PIC_SQUARE_WIDTH + 6*FLOAT_PIC_SQUARE_WIDTH,
			FLOAT_PIC_SQUARE_WIDTH,FLOAT_PIC_SQUARE_WIDTH,1,8,true);
		m_pAnimation->SetMode(HGEANIM_LOOP|HGEANIM_FWD);
	}
}

// void Character::Attack()
// {
// 	//攻击者和被攻击者的相关数值传递给管理器计算攻击结果
// 	CreatureManager::sInstance().CalculateResult(m_nNum,m_nTar);
// 
// 	//播放攻击动作
// 	m_pAnimation->SetTexture(m_mCharTex[eCharacterState_Fight]);
// 	m_pAnimation->ResetFrames(0,(m_eCurDir-1)*FLOAT_PIC_SQUARE_HEIGHT,
// 		FLOAT_PIC_SQUARE_HEIGHT,FLOAT_PIC_SQUARE_HEIGHT,4,8,false);
// 	m_pAnimation->SetMode(HGEANIM_FWD | HGEANIM_NOLOOP);
// 
// 	//设置攻击子状态
// 	m_eAttackState = eAttackState_Attacking;
// }

void Character::Attack(eNotification notify,Character* target,int time)
{
	//播放攻击动作
	m_pAnimation->SetTexture(m_mCharTex[eActionTex_Attack]);
	m_pAnimation->ResetFrames(0,4*(GetTexDir(m_eCurDir)-1)*FLOAT_PIC_SQUARE_HEIGHT,
		FLOAT_PIC_SQUARE_HEIGHT,FLOAT_PIC_SQUARE_HEIGHT,4,8,true);
	m_pAnimation->SetMode(HGEANIM_FWD | HGEANIM_NOLOOP);

	//设置攻击子状态
//	m_eAttackState = eAttackState_Attacking;
	m_eCharState = eCharacterState_Fight;

	m_nActionTime = time;
	m_eNotify = notify;
}

// void Character::Attacked()
// {
// 	//播放被攻击动作
// 	m_pAnimation->SetTexture(m_mCharTex[eCharacterState_Defense]);
// 	m_pAnimation->ResetFrames(0,3*FLOAT_PIC_SQUARE_WIDTH,
// 		FLOAT_PIC_SQUARE_WIDTH,FLOAT_PIC_SQUARE_WIDTH,1,8,false);
// 	m_pAnimation->SetMode(HGEANIM_FWD | HGEANIM_LOOP);
// 
// 
// 	//设置被攻击子状态
// 	m_eCharState = eCharacterState_Defense;
// 	m_eAttackState = eAttackState_Attackeding;
// }

void Character::Attacked(eNotification notify,Character* target,int time)
{
	//播放被攻击动作
	m_pAnimation->SetTexture(m_mCharTex[eActionTex_Attacked]);
	m_pAnimation->ResetFrames(0,3*FLOAT_PIC_SQUARE_WIDTH,
		FLOAT_PIC_SQUARE_WIDTH,FLOAT_PIC_SQUARE_WIDTH,1,8,true);
	m_pAnimation->SetMode(HGEANIM_FWD | HGEANIM_LOOP);


	//设置被攻击子状态
	m_eCharState = eCharacterState_Attacked;
//	m_eAttackState = eAttackState_Attackeding;

	m_nActionTime = time;
	m_eNotify = notify;
}

// void Character::Defend()
// {
// 	//播放被攻击动作
// 	int offset = 0;
// 	if(m_eCurDir == eDirection_Down)
// 		offset = 0;
// 	else if (m_eCurDir == eDirection_Up)
// 		offset = 1;
// 	else if(m_eCurDir == eDirection_Left || m_eCurDir == eDirection_Right)
// 		offset = 2;
// 	m_pAnimation->SetTexture(m_mCharTex[eCharacterState_Defense]);
// 	m_pAnimation->ResetFrames(0,offset*FLOAT_PIC_SQUARE_WIDTH,
// 		FLOAT_PIC_SQUARE_WIDTH,FLOAT_PIC_SQUARE_WIDTH,1,8,false);
// 	m_pAnimation->SetMode(HGEANIM_FWD | HGEANIM_LOOP);
// 
// 
// 	//设置被攻击子状态
// 	m_eCharState = eCharacterState_Defense;
// 	m_eAttackState = eAttackState_Defending;
// }

void Character::Defend(eNotification notify,Character* target,int time)
{
	//播放被攻击动作
	int offset = 0;
	m_pAnimation->SetTexture(m_mCharTex[eActionTex_Defend]);
	m_pAnimation->ResetFrames(0,(GetTexDir(m_eCurDir)-1)*FLOAT_PIC_SQUARE_WIDTH,
		FLOAT_PIC_SQUARE_WIDTH,FLOAT_PIC_SQUARE_WIDTH,1,8,true);
	m_pAnimation->SetMode(HGEANIM_FWD | HGEANIM_LOOP);


	//设置被攻击子状态
	m_eCharState = eCharacterState_Defense;
//	m_eAttackState = eAttackState_Defending;

	m_nActionTime = time;
	m_eNotify = notify;
}

// void Character::Counter()
// {
// 
// }

// void Character::Crit()
// {
// 	//播放攻击动作
// 	m_pAnimation->SetTexture(m_mCharTex[eCharacterState_Fight]);
// 	m_pAnimation->ResetFrames(0,(m_eCurDir-1)*FLOAT_PIC_SQUARE_HEIGHT,
// 		FLOAT_PIC_SQUARE_HEIGHT,FLOAT_PIC_SQUARE_HEIGHT,1,8,false);
// 	m_pAnimation->SetMode(HGEANIM_FWD | HGEANIM_LOOP);
// 
// 	//设置攻击子状态
// 	m_eAttackState = eAttackState_Criting;
// }

void Character::Crit(eNotification notify,Character* target,int time)
{
	//播放攻击动作
	m_pAnimation->SetTexture(m_mCharTex[eActionTex_Attack]);
	m_pAnimation->ResetFrames(0,4*(GetTexDir(m_eCurDir)-1)*FLOAT_PIC_SQUARE_HEIGHT,
		FLOAT_PIC_SQUARE_HEIGHT,FLOAT_PIC_SQUARE_HEIGHT,1,8,true);
	m_pAnimation->SetMode(HGEANIM_FWD | HGEANIM_LOOP);

	//设置攻击子状态
//	m_eAttackState = eAttackState_Criting;
	m_eCharState = eCharacterState_Fight;

	m_nActionTime = time;
	m_eNotify = notify;
}

// void Character::Dead()
// {
// 	//播放攻击动作
// 	m_pAnimation->SetTexture(m_mCharTex[eCharacterState_Dead]);
// 	m_pAnimation->ResetFrames(0,(m_eCurDir-1)*FLOAT_PIC_SQUARE_WIDTH,
// 		FLOAT_PIC_SQUARE_WIDTH,FLOAT_PIC_SQUARE_WIDTH,4,8,false);
// 	m_pAnimation->SetMode(HGEANIM_FWD | HGEANIM_NOLOOP);
// 
// 	//设置攻击子状态
// 	m_eAttackState = eAttackState_Dead;
// 	m_eCharState = eCharacterState_Dead;
// }

void Character::Dead(eNotification notify,Character* target,int time)
{
	//播放攻击动作
	m_pAnimation->SetTexture(m_mCharTex[eActionTex_Dead]);
	m_pAnimation->ResetFrames(0,9*FLOAT_PIC_SQUARE_WIDTH,
		FLOAT_PIC_SQUARE_WIDTH,FLOAT_PIC_SQUARE_WIDTH,2,8,true);
	m_pAnimation->SetMode(HGEANIM_FWD | HGEANIM_LOOP);

	//设置攻击子状态
//	m_eAttackState = eAttackState_Dead;
	m_eCharState = eCharacterState_Dead;

	m_nActionTime = time;
	m_eNotify = notify;
}

void Character::Healed(eNotification notify,Character* target,int time)
{
	//播放攻击动作
	m_pAnimation->SetTexture(m_mCharTex[eActionTex_LevelUp]);
	m_pAnimation->ResetFrames(0,4*FLOAT_PIC_SQUARE_WIDTH,
		FLOAT_PIC_SQUARE_WIDTH,FLOAT_PIC_SQUARE_WIDTH,1,8,true);
	m_pAnimation->SetMode(HGEANIM_FWD | HGEANIM_LOOP);

	m_eCharState = eCharacterState_Healed;

	m_nActionTime = time;
	m_eNotify = notify;
}

void Character::UseItem(eNotification notify,Character* target,int time)
{
	//物品图片从释放者身上飘起，落在使用者身上
	if(m_nUseItem == -1)
		return;
	Item item = ConfigManager::sInstance().GetItemInfo().find(m_nUseItem)->second;
	m_pItemSpr->SetTexture(TexManager::sInstance().GetItemTex(item.m_nID));
	m_nDrawItem = 1;
	m_fDrawItemX = m_fXPos;
	m_fDrawItemY = m_fYPos;
	m_pItemTarget = target;

	m_eCharState = eCharacterState_UseItem;

	m_nActionTime = time;
	m_eNotify = notify;
}

bool Character::CanHitTarget(Character* target)
{
	if(!target)
		return false;

	int tarX = 0,tarY = 0;
	VPair vPairPoint = CreatureManager::sInstance().GetRangePoint();
	for (MAttackRange::iterator mit=CreatureManager::sInstance().GetAttackRange().begin();mit!=CreatureManager::sInstance().GetAttackRange().end();mit++)
	{
		if(mit->first == m_eAttackRange)
		{
			for (vector<int>::iterator it=mit->second.begin();it!=mit->second.end();it++)
			{
				tarX = vPairPoint[*it].x + m_iBlock.xpos;
				tarY = vPairPoint[*it].y + m_iBlock.ypos;
				if(target->GetBlock().xpos == tarX && target->GetBlock().ypos == tarY)
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool Character::CanSkillHitTarget(Character* target)
{
	if(!target)
		return false;

	Block tarBlock = target->GetBlock();
	SkillInfo skill = ConfigManager::sInstance().GetSkillInfo().find(m_nCastSkill)->second;
	if(skill.m_nSkillType == eSkillType_Hurt && target->GetCamp() != eCamp_Enemy)
		return false;
	if((skill.m_nSkillType == eSkillType_Heal || skill.m_nSkillType == eSkillType_Buff) && target->GetCamp() != eCamp_Friend)
		return false;
	if(abs(tarBlock.xpos-m_iBlock.xpos) + abs(tarBlock.ypos-m_iBlock.ypos) <= skill.m_nCastRange)
		return true;
	return false;
}

std::vector<int>	Character::GetSkillList()
{
	std::vector<int> vSkillList;
	vSkillList.insert(vSkillList.begin(),m_vDefaultSkillList.begin(),m_vDefaultSkillList.end());
	vSkillList.insert(vSkillList.end(),m_vNewSkillList.begin(),m_vNewSkillList.end());
	return vSkillList;
}


/*根据属性类型，调整属性值
type : 解释
1 : 增加HP上限以及当前HP值
2：增加MP上限以及当前MP值
3：增加攻击力
4：增加防御力
5：增加暴击概率
6：增加闪避概率
*/
void Character::SetAttributeValue(int type,int value)
{
	switch(type)
	{
	case 1:
		{
			m_nHPMax += value;
			m_nHP += value;
		}
		break;
	case 2:
		{
			m_nMPMax += value;
			m_nMP += value;
		}
		break;
	case 3:
		m_nAttack += value;
		break;
	case 4:
		m_nDefend += value;
		break;
	case 5:
		m_fCrit += ((float)value)/1000;
		break;
	case 6:
		m_fDodge += ((float)value)/1000;
		break;
	}
}

void Character::RemoveBuff()
{
	for (std::vector<BuffData>::iterator it=m_vBuffData.begin();it!=m_vBuffData.end();)
	{
		//持续时间减1
		it->m_nLastTurns--;
		//移除时间为0的buff
		if (it->m_nLastTurns <= 0)
		{
			switch(it->m_nType)
			{
			case 1:
				m_nHPMax -= it->m_nValue;
				break;
			case 2:
				m_nMPMax -= it->m_nValue;
				break;
			case 3:
				m_nAttack -= it->m_nValue;
				break;
			case 4:
				m_nDefend -= it->m_nValue;
				break;
			case 5:
				m_fCrit -= ((float)it->m_nValue)/1000;
				break;
			case 6:
				m_fDodge -= ((float)it->m_nValue)/1000;
				break;
			default:
				break;
			}
			it = m_vBuffData.erase(it);
		}
		else
		{
			it++;
		}
	}
}

bool	Character::CanUseItem(Character* target)
{
	return true;
}

void	Character::ItemEffect(Item item)
{
	if (item.m_nType == 1)	//可使用物品
	{
		for (std::map<int,int>::iterator it=item.m_mEffect.begin();it!=item.m_mEffect.end();it++)
		{
			switch(it->first)
			{
			case 1:	//HP恢复
				{
					m_nHP += it->second;
					if(m_nHP >= m_nHPMax)
						m_nHP = m_nHPMax;
				}
				break;
			case 2:	//MP恢复
				{
					m_nMP += it->second;
					if(m_nMP >= m_nMPMax)
						m_nMP = m_nMPMax;
				}
				break;
			case 3:	//攻击永久加成
				{
					m_nAttack += it->second;
				}
				break;
			case 4:	//防御永久加成
				m_nDefend += it->second;
				break;
			case 5:	//暴击永久加成
				m_fCrit += it->second;
				break;
			case 6:	//闪避永久加成
				m_fDodge += it->second;
				break;
			case 7:	//攻击范围永久加成
				{
					switch(m_eAttackRange)
					{
					case eAttackRange_Cross:
						m_eAttackRange = eAttackRange_CrossEx;
					case eAttackRange_Box:
						m_eAttackRange = eAttackRange_BoxEx;
					case eAttackRange_BigCross:
						m_eAttackRange = eAttackRange_BigCrossEx;
					case eAttackRange_Arrow:
						m_eAttackRange = eAttackRange_ArrowEx;
					}
					//其他范围无法加成
				}
				break;
			case 8:	//移动力永久加成
				m_nMoveAbility += it->second;
				break;
			}
		}
	}
}

void Character::Talk(int head,const char* name,const char* word)
{
	CreatureManager::sInstance().AddAction(eAction_Talk,m_nNum,0,eDirection_None,head,name,word);
}

void Character::Turn(int dir,DWORD time)
{
	CreatureManager::sInstance().AddAction(eAction_Turn,m_nNum,time,(eDirection)dir);
}

void Character::Attack(DWORD time)
{
	CreatureManager::sInstance().AddAction(eAction_Attack,m_nNum,time);
}

void Character::Crit(DWORD time)
{
	CreatureManager::sInstance().AddAction(eAction_Crit,m_nNum,time);
}

void Character::Attacked(DWORD time)
{
	CreatureManager::sInstance().AddAction(eAction_Attacked,m_nNum,time);
}

void Character::Defend(DWORD time)
{
	CreatureManager::sInstance().AddAction(eAction_Defend,m_nNum,time);
}

void Character::Dead(DWORD time)
{
	CreatureManager::sInstance().AddAction(eAction_Dead,m_nNum,time);
}

void Character::Walk(DWORD time)
{
	CreatureManager::sInstance().AddAction(eAction_Walk,m_nNum,time);
}

void Character::Move(int x,int y)
{
	DWORD data = x + y << 8;
	CreatureManager::sInstance().AddAction(eAction_Move,m_nNum,0,eDirection_None,data);
}

void Character::PushAction(NewAction action)
{
	m_iAction = action;
}