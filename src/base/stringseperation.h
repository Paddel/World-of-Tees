#pragma once

#include <iostream>
#include <stdlib.h>
#include <base/system.h>
//using namespace std;

#define MAX_MAPSTRING_SIZE 1024*2

static char Nums[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-'};

static inline void ReduceString(char *str, int num)
{
	int len = str_length(str);
	for(int k=num; k < len-1; k++){
		str[k-num] = str[k+1];
	}
	str[len-1-num] = NULL;
}

static char *GetSepStr(char SepChar, char **content)
{
	char *pContStr = *content;
	char *pOutStr  = 0;

	if(*content == NULL)//string is empty
		return "";

	int len = str_length(pContStr);
	for(int i = 0; i < len; i++)
	{
		if(pContStr[i] && pContStr[i] == SepChar)
		{
			pContStr[i] = 0;
			pOutStr = &pContStr[0];
			*content = &pContStr[i+1];
			if(i == len-1)
				*content = 0;
			break;
		}
	}

	if(!pOutStr)
	{
		pOutStr = pContStr;
		*content = 0;
	}

	return pOutStr;
}

static int GetSepInt(char SepChar, char **content)
{
	char *pNumStr = GetSepStr(SepChar, content);

	if(!pNumStr)
		return -1;

	//Check chars
	for(int i = 0; i < str_length(pNumStr); i++)
	{
		bool check = false;
		for(int a = 0; a < str_length(Nums); a++)
		{
			if(pNumStr[i] == Nums[a])
			{//char is available
				check = true;
				break;
			}
		}

		if(!check)
		{//char is NOT available
			return -1;
		}
	}

	return atoi(pNumStr);
}

static inline bool IsAsciiStr(char *content)
{
	// trim right and set maximum length to 128 utf8-characters
	int Length = 0;
	const char *p = content;
	const char *pEnd = 0;
	while(*p)
 	{
		const char *pStrOld = p;
		int Code = str_utf8_decode(&p);

		// check if unicode is not empty
		if(Code > 0x20 && Code != 0xA0 && Code != 0x034F && (Code < 0x2000 || Code > 0x200F) && (Code < 0x2028 || Code > 0x202F) &&
			(Code < 0x205F || Code > 0x2064) && (Code < 0x206A || Code > 0x206F) && (Code < 0xFE00 || Code > 0xFE0F) &&
			Code != 0xFEFF && (Code < 0xFFF9 || Code > 0xFFFC))
		{
			pEnd = 0;
		}
		else if(pEnd == 0)
			pEnd = pStrOld;

		if(++Length >= 127)
		{
			return false;
		}
 	}
	if(pEnd != 0)
		return false;

	return true;
}