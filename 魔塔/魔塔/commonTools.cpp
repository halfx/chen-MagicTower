#include "commonTools.h"

LBUTTON_STATE g_getLButtonState(HGE* hge)
{
	static bool lastPressed = false;
	static bool currentPressed = false;
	LBUTTON_STATE state = LBUTTON_NULL;
	currentPressed = hge->Input_GetKeyState(HGEK_LBUTTON);

	if (currentPressed==false && lastPressed==false)
		state =  LBUTTON_NULL;
	else if(currentPressed==true && lastPressed==false)
		state =  LBUTTON_DOWN;
	else if(currentPressed==true && lastPressed==true)
		state =  LBUTTON_HOLD;
	else if(currentPressed==false && lastPressed==true)
		state =  LBUTTON_UP;

	lastPressed = currentPressed;
	return state;
}

int g_getKeyNum(int Key)
{
	if(Key >= HGEK_0 && Key <= HGEK_9)
		return Key-HGEK_0;
	if(Key >= HGEK_A && Key <= HGEK_Z)
		return Key-HGEK_A;
	return 0;
}

//对于不同的key，需要重置state
//目前支持A-Z 0-9  ，扩展其他按键需要扩展静态数组KEY_NUM
KEY_STATE g_getKeyState(HGE* hge,int Key)
{
	static int lastKey[KEY_NUM] = {-1};
	static bool lastPressed[KEY_NUM] = {false};
	static bool currentPressed[KEY_NUM] = {false};
	KEY_STATE state = KEY_NULL;
	int num = g_getKeyNum(Key);
	if (lastKey[num] != Key)
	{
		lastPressed[num] = false;
		currentPressed[num] = false;
		lastKey[num] = Key;
	}
	currentPressed[num] = hge->Input_GetKeyState(Key);

	if (currentPressed[num]==false && lastPressed[num]==false)
		state =  KEY_NULL;
	else if(currentPressed[num]==true && lastPressed[num]==false)
		state =  KEY_DOWN;
	else if(currentPressed[num]==true && lastPressed[num]==true)
		state =  KEY_HOLD;
	else if(currentPressed[num]==false && lastPressed[num]==true)
		state =  KEY_UP;

	lastPressed[num] = currentPressed[num];
	return state;
}

void g_getFiles( std::string path, std::map<std::string,std::string>& files,char* type,int maxFileNum)
{
	std::string* mapNames = new std::string[maxFileNum];
	for (int i=0;i<maxFileNum;i++)
	{
		char filename[50];
		sprintf(filename,"%d%s",i,type);
		mapNames[i].assign(filename);
	}

	//文件句柄
	long   hFile   =   0;
	//文件信息
	_finddata_t fileinfo;
	std::string p;
	if((hFile = _findfirst(p.assign(path).append("\\*").c_str(),&fileinfo)) !=  -1)
	{
		do
		{
			//如果是目录,迭代之
			//如果不是,加入列表
			if((fileinfo.attrib &  _A_SUBDIR))
			{
				if(strcmp(fileinfo.name,".") != 0  &&  strcmp(fileinfo.name,"..") != 0)
					g_getFiles( p.assign(path).append("\\").append(fileinfo.name), files,type,maxFileNum );
			}
			else
			{
				for(int i=0;i<maxFileNum;i++)
				{
					if(strcmp(fileinfo.name,mapNames[i].c_str()) == 0)
					{
						files[p.assign(path).append("\\").append(fileinfo.name)] = fileinfo.name;
						break;
					}
				}
			}
		}while(_findnext(hFile, &fileinfo)  == 0);
		_findclose(hFile);
	}

	delete[] mapNames;
}