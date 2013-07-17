#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "commonTools.h"
#include <sstream>

struct CreatureInfo
{
	int m_nID;
	std::string m_strKind;
	int m_nTexID;
	CreatureInfo()
	{
		m_nID = -1;
		m_strKind = "";
		m_nTexID = -1;
	}
	CreatureInfo(int id,std::string kind,int texid)
	{
		m_nID = id;
		m_strKind = kind;
		m_nTexID = texid;
	}
};

struct SkillInfo
{
	int m_nID;								//技能ID
	int m_nFrames;					//特效帧数
	int m_nAttackRange;			//攻击范围
	int m_nOffX;							//绘制横向偏移
	int m_nOffY;							//绘制纵向偏移
	int m_nIcon;							//图标号
	int m_nCastRange;				//释放范围
	int m_nCost;							//消耗MP
	int m_nSkillType;					//技能类型
	int m_nAttack;						//攻击力
	int m_nLastTurn;					//持续回合数
	std::string m_strName;		//技能名称
	SkillInfo()
	{
		m_nID = -1;
		m_nFrames = 0;
		m_nAttackRange = 0;
		m_nOffX = m_nOffY = 0;
		m_nIcon = -1;
		m_nCastRange = 0;
		m_nCost = 0;
		m_nSkillType = 0;
		m_nAttack = 0;
		m_nLastTurn = 0;
		m_strName = "";
	}
	SkillInfo(int id,int frames,int attackRange,int offx,int offy,int icon,int castRange,int cost,int skillType,int attack,int lastTurn,std::string name)
	{
		m_nID = id;
		m_nFrames = frames;
		m_nAttackRange = attackRange;
		m_nOffX = offx;
		m_nOffY = offy;
		m_nIcon = icon;
		m_nCastRange = castRange;
		m_nCost = cost;
		m_nSkillType = skillType;
		m_nAttack = attack;
		m_nLastTurn = lastTurn;
		m_strName = name;
	}
};

class ConfigManager : public Singleton<ConfigManager>
{
public:
	ConfigManager()
	{
		m_mCreatureConfig.clear();
		m_mCreatureInfo.clear();
	}
	~ConfigManager(){}

	void LoadConfig()
	{
		LoadCreatureConfig();
		LoadSkillConfig();
	}

	void LoadCreatureConfig()
	{
		char pBuf[MAX_PATH];
		char pathConfig[MAX_PATH];
		GetCurrentDirectory(MAX_PATH,pBuf);
		sprintf(pathConfig,"%s\\res\\config\\Creature.txt",pBuf);
		m_mCreatureConfig = g_parseConfigFile(pathConfig);

		for (std::map<std::string,std::string>::iterator mit=m_mCreatureConfig.begin();mit!=m_mCreatureConfig.end();mit++)
		{
			char simbol[256];
			char strKey[256];
			sscanf(mit->first.c_str(),"%s %s",simbol,strKey);
			if (strcmp(simbol,"CreatureInfo")==0)
			{
				int ID = atoi(strKey);
				char kind[50];
				int texID = -1;
				sscanf(mit->second.c_str(),"%s /%d",kind,&texID);
				m_mCreatureInfo[ID] = CreatureInfo(ID,kind,texID);
			}
			else if(strcmp(simbol,"CreatureSkill")==0)
			{
				std::stringstream ssteam(mit->second);
				int skillNum = 0;
				skillNum = std::count(mit->second.begin(),mit->second.end(),'/') + 1;
				int* skillID = new int[skillNum];
				char cTemp;
				for (int i=0;i<skillNum;i++)
				{
					ssteam >> skillID[i] >> cTemp;
					m_mCreatureSkill[strKey].push_back(skillID[i]);
				}

// 				int searchPos = 0;
// 				for (int i=0;i<skillNum;i++)
// 				{
// 					int skillID = -1;
// 					int pos = mit->second.find('/',searchPos);
// 					if(pos == std::string::npos)
// 						skillID = atoi(mit->second.c_str()+searchPos);
// 					else
// 					{
// 						skillID = atoi((mit->second).substr(searchPos,pos).c_str());
// 						searchPos = pos + 1;
// 					}
// 					m_mCreatureSkill[strKey].push_back(skillID);
// 				}
			}
		}
	}
	std::map<int,CreatureInfo>& GetCreatureInfo(){return m_mCreatureInfo;}

	std::vector<int> GetCreatureSkill(std::string kind)
	{
		std::vector<int> vSkill;
		std::map<std::string, std::vector<int> >::iterator it = m_mCreatureSkill.find(kind);
		if(it != m_mCreatureSkill.end())
		{
			vSkill = it->second;
		}
		return vSkill;
	}

	void LoadSkillConfig()
	{
		char pBuf[MAX_PATH];
		char pathConfig[MAX_PATH];
		GetCurrentDirectory(MAX_PATH,pBuf);
		sprintf(pathConfig,"%s\\res\\config\\Skill.txt",pBuf);
		m_mSkillConfig = g_parseConfigFile(pathConfig);

		for (std::map<std::string,std::string>::iterator mit=m_mSkillConfig.begin();mit!=m_mSkillConfig.end();mit++)
		{
			char simbol[256];
			int ID = -1;
			sscanf(mit->first.c_str(),"%s %d",simbol,&ID);
			if (strcmp(simbol,"SkillInfo")==0 && ID!=-1)
			{
				int frames = -1;
				int attackRange = -1;
				int offx = -1,offy = -1;
				int icon = -1;
				int castRange = -1;
				int cost = 0;
				int skillType = 0;
				int attack = 0;
				int lastTurn = 0;
				std::string name;
				sscanf(mit->second.c_str(),"%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%s",&frames,&attackRange,&offx,&offy,&icon,&castRange,&cost,&skillType,&attack,&lastTurn,name.c_str());
				m_mSkillInfo[ID] = SkillInfo(ID,frames,attackRange,offx,offy,icon,castRange,cost,skillType,attack,lastTurn,name.c_str());
			}
		}
	}
	std::map<int,SkillInfo>& GetSkillInfo(){return m_mSkillInfo;}
private:
	std::map<std::string,std::string> m_mCreatureConfig;
	std::map<int,CreatureInfo> m_mCreatureInfo;
	std::map<std::string, std::vector<int> > m_mCreatureSkill;

	std::map<std::string,std::string> m_mSkillConfig;
	std::map<int,SkillInfo> m_mSkillInfo;
};
#endif