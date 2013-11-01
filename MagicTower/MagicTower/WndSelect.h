#ifndef WND_SELECT_H
#define WND_SELECT_H

#include "commonTools.h"
#include "UI.h"

class Character;
class WndSelect : public UIWindow
{
	CREATEWINDOW(WndSelect)
public:
	enum eControlID
	{
		eControlID_ListBox = 1,
	};

	WndSelect();
	virtual ~WndSelect();

// 	virtual void Render(){}
 	virtual void Update(float dt);
	virtual void Render();
	virtual void OnMouseOver(float x,float y);
	virtual void SetRenderPositon(float _x=-1,float _y=-1);
	virtual void	SetBindChar(Character* bindChar);
	virtual Character*	GetBindChar(){return m_pBindChar;}
private:
	UIListBox* m_pListBox;
	std::map<int,int> m_mListItemToSkillId;
	std::map<int,int> m_mListItemToItemId;
	Character* m_pBindChar;
};

#endif