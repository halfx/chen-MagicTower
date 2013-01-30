#include "MapManager.h"
#include "TexManager.h"

Map::Map()
{
	m_vObjList.clear();
	m_vBlocks.clear();
	m_ilevel = 0;
	m_width = 0;
	m_length = 0;
	m_mapSpr = new hgeSprite(0,0,0,0,0);
}

Map::~Map()
{
	for (std::vector<MapObject*>::iterator it=m_vObjList.begin();it!=m_vObjList.end();it++)
	{
		gSafeDelete((*it)->spr);
		gSafeDelete(*it);
	}
	gSafeDelete(m_mapSpr);
}

void Map::render()
{
	for (std::vector<Block>::iterator it=m_vBlocks.begin();it!=m_vBlocks.end();it++)
	{
		blockInfo _blockInfo(TexManager::sInstance().getBlock(getTerrain(it->attri)));
		m_mapSpr->SetTexture(_blockInfo.tex);
		m_mapSpr->SetTextureRect(_blockInfo.x,_blockInfo.y,_blockInfo.width,_blockInfo.height);
		m_mapSpr->RenderStretch(MAP_OFF_X +MAP_RECT*(*it).xpos,MAP_OFF_Y+MAP_RECT*(*it).ypos,MAP_OFF_X +MAP_RECT*(*it).xpos+MAP_RECT,MAP_OFF_Y+MAP_RECT*(*it).ypos+MAP_RECT);
	}

	for (std::vector<MapObject*>::iterator it=m_vObjList.begin();it!=m_vObjList.end();it++)
	{
		(*it)->spr->Render((MAP_RECT-FLOAT_PIC_SQUARE_WIDTH)/2+MAP_OFF_X +MAP_RECT*(*it)->xpos,MAP_OFF_Y+MAP_RECT*(*it)->ypos);
	}
}

void Map::update()
{

}

MapObject* Map::getObject(int x,int y)
{
	for (int i=0;i<m_vObjList.size();i++)
	{
		if(m_vObjList[i]->xpos == x && m_vObjList[i]->ypos == y)
			return m_vObjList[i];
	}
	return NULL;
}

Block* Map::getBlock(int x,int y)
{
	if(x < 0 || x > MAP_WIDTH_NUM || y < 0 || y > MAP_LENGTH_NUM)
		return NULL;
	return &(m_vBlocks[x+y*MAP_WIDTH_NUM]);
}

MapManager::MapManager(void)
{
	m_vMaps.clear();
	m_markUp = NULL;
	m_iCurrentLevel = 0;
}


MapManager::~MapManager(void)
{
	gSafeDelete(m_markUp);
	for (std::vector<Map*>::iterator it=m_vMaps.begin();it!=m_vMaps.end();it++)
	{
		gSafeDelete((*it));
	}
}

bool MapManager::LoadMaps(std::string path)
{
	if(m_markUp==NULL)
		m_markUp = new CMarkup;
	std::map<std::string,std::string> files;
	g_getFiles(path,files,".xml",LEVEL_NUM);
	
	for (std::map<std::string,std::string>::iterator mit=files.begin();mit!=files.end();mit++)
	{
		m_markUp->Load(mit->first);
		Map* level = new Map;
		int _level = 0;
		int _width = 0,_length = 0;
		int _ID = 0;
		int _Action = 0;
		int _xpos = 0;
		int _ypos = 0;
		m_markUp->ResetMainPos();
		if(m_markUp->FindElem("map"))
		{
			_level = atoi(m_markUp->GetAttrib("level").c_str());
			_width = atoi(m_markUp->GetAttrib("width").c_str());
			_length = atoi(m_markUp->GetAttrib("length").c_str());
			if(_level!=0 && _width!=0 && _length!=0)
			{
				level->setLevel(_level);
				level->setWidthLength(_width,_length);
			}
			else
				return false;
		}
		m_markUp->IntoElem();
		if(m_markUp->FindElem("Block"))
		{
			m_markUp->IntoElem();
			int _type = 0;
			while(m_markUp->FindElem("Terrain"))
			{
				_type = atoi(m_markUp->GetAttrib("type").c_str());
				_xpos = atoi(m_markUp->GetAttrib("xpos").c_str());
				_ypos = atoi(m_markUp->GetAttrib("ypos").c_str());
				Block _block(_xpos,_ypos);
// 				setTerrain(_block.attri,_type);
// 				if (_type == HillTop || _type == CityWall)
// 				{
// 					setStandOn(_type,0);
// 				}
				_block.attri = _type;
				level->addBlock(_block);
			}
			std::sort(level->getVBlock().begin(),level->getVBlock().end(),Block::less_than);
			m_markUp->OutOfElem();
		}
		if(m_markUp->FindElem("Object"))
		{
			m_markUp->IntoElem();
			while(m_markUp->FindElem("Man"))
			{
				_ID = atoi(m_markUp->GetAttrib("ID").c_str());
				_Action = atoi(m_markUp->GetAttrib("Action").c_str());
				_xpos = atoi(m_markUp->GetAttrib("xpos").c_str());
				_ypos = atoi(m_markUp->GetAttrib("ypos").c_str());

				MapObject* mo = new MapObject;
				float xNum=0,yNum=0;
				xNum = _Action%(FLOAT_PIC_WIDTH/FLOAT_PIC_SQUARE_WIDTH);
				yNum = _Action/(FLOAT_PIC_WIDTH/FLOAT_PIC_SQUARE_WIDTH);
				mo->spr			=	new hgeSprite(TexManager::sInstance().getTex(_ID),xNum*FLOAT_PIC_SQUARE_WIDTH,yNum*FLOAT_PIC_SQUARE_HEIGHT,FLOAT_PIC_SQUARE_WIDTH,FLOAT_PIC_SQUARE_HEIGHT);
				mo->ID				=	_ID;
				mo->action		=	_Action;
				mo->xpos			=	_xpos;
				mo->ypos			=	_ypos;
				Block* theBlock = level->getBlock(mo->xpos,mo->ypos);
				if(theBlock != NULL)
					setOccupied((theBlock->attri),1);
				level->addObject(mo);
			}
			m_markUp->OutOfElem();
		}
		m_vMaps.push_back(level);
	}

	//将所有地图文件按照level属性进行排序
	//load 完所有地图文件，设置当前关卡为第一关
	if(m_vMaps.empty())
		return false;
	std::sort(m_vMaps.begin(),m_vMaps.end(),Map::less_than);
	setCurrentLevel(m_vMaps[0]->getLevel());
	return true;
}

Map* MapManager::getMap(int level)
{
	for (int i=0;i<m_vMaps.size();i++)
	{
		if(level == m_vMaps[i]->getLevel())
			return m_vMaps[i];
	}
	return NULL;
}

void MapManager::render()
{
	static int oldLevel = -1;
	static Map* currentMap = NULL;
	if (oldLevel != m_iCurrentLevel)
	{
		currentMap = getMap(m_iCurrentLevel);
		oldLevel = m_iCurrentLevel;
	}
	else
	{
		if(currentMap!=NULL)
			currentMap->render();
	}
}

void MapManager::update()
{
	static int oldLevel = -1;
	static Map* currentMap = NULL;
	if (oldLevel != m_iCurrentLevel)
	{
		currentMap = getMap(m_iCurrentLevel);
		oldLevel = m_iCurrentLevel;
	}
	else
	{
		if(currentMap!=NULL)
			currentMap->update();
	}
}