/*Copyright 2015 Huawei Technologies Co., Ltd. All rights reserved.
eSDK is licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
		http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.*/

#include "StdAfx.h"
#include "AudioMgr.h"
#include "IVSCommon.h"
#include "LockGuard.h"
#include "Mmsystem.h"

#pragma comment (lib, "Winmm.lib")

CAudioMgr::CAudioMgr(void)
{//lint !e1927 !e1926
    m_pUserMgr = NULL;
    m_pHandleTalkBackMutex = VOS_CreateMutex();
}

CAudioMgr::~CAudioMgr(void)
{
    m_pUserMgr = NULL;
    try
    {
        HandleTalkBackMapIter iter = m_HanleTalkBackMap.begin();
        while(m_HanleTalkBackMap.end() != iter)
        {
            CAudioObj* pTalkback = dynamic_cast<CAudioObj*>(iter->second);//lint !e611
            if (NULL != pTalkback)
            {
                IVS_DELETE(pTalkback);
            }
            ++iter;
        }
        m_HanleTalkBackMap.clear();

        if (NULL != m_pHandleTalkBackMutex)
        {
            (void)VOS_DestroyMutex(m_pHandleTalkBackMutex);
            m_pHandleTalkBackMutex = NULL;
        }
    }
    catch (...)
    {
    }

    if (NULL != m_pHandleTalkBackMutex)
    {
        VOS_DestroyMutex(m_pHandleTalkBackMutex);
        m_pHandleTalkBackMutex = NULL;
    }
}

//清除数据
void CAudioMgr::ClearUp()
{
	m_audioObj.ClearUp();
}

void CAudioMgr::SetUserMgr(CUserMgr* pUserMgr)
{
    m_pUserMgr = pUserMgr;

    // modify by wanglei 00165153:2013.5.3 m_audioObj是成员变量，m_pUserMgr后续需要用到
    m_audioObj.SetUserMgr(m_pUserMgr);
}

//根据句柄获取空闲对象
CAudioObj* CAudioMgr::GetFreeTalkBack(IVS_ULONG& ulHandle)
{
    CLockGuard lock(m_pHandleTalkBackMutex);
    CAudioObj *pTalkback = NULL;
    HandleTalkBackMapIter talkbackIter = m_HanleTalkBackMap.begin();
    HandleTalkBackMapIter talkbackIterEnd = m_HanleTalkBackMap.end();

    //查找空闲对象;
    for (; talkbackIter != talkbackIterEnd; talkbackIter++)
    {
        pTalkback = dynamic_cast<CAudioObj*>(talkbackIter->second);//lint !e611
        if (NULL != pTalkback)
        {
            if (MEDIA_STATUS_FREE == pTalkback->GetStatus())
            {
                break;
            }
        }
        pTalkback = NULL;
    }

    if (NULL != pTalkback)
    {
        m_HanleTalkBackMap.erase(talkbackIter);
    }
    else
    {
        pTalkback = IVS_NEW((CAudioObj*&)pTalkback);
        if (NULL == pTalkback)
        {
            BP_RUN_LOG_ERR(IVS_OPERATE_MEMORY_ERROR, "Get Free Talkback", "create talkback error");
            return NULL;
        }
    }

    //初始化talkback对象;
    pTalkback->SetUserMgr(m_pUserMgr);
    pTalkback->SetStatus(MEDIA_STATUS_BUSY);

    //插入列表;
    ulHandle = reinterpret_cast<unsigned long>(pTalkback);
    (void)m_HanleTalkBackMap.insert(std::make_pair(ulHandle, pTalkback));

    pTalkback->SetHandle(ulHandle);
    return pTalkback;
}//lint !e1788

//开始语音对讲
IVS_INT32 CAudioMgr::StartVoiceTalkback(const IVS_VOICE_TALKBACK_PARAM* pTalkbackParam,
                                        const IVS_CHAR* pCameraCode,
                                        IVS_ULONG* pHandle)
{
	if (NULL == pTalkbackParam)
	{
		BP_RUN_LOG_ERR(IVS_PARA_INVALID, "Start Voice Talkback", "NULL == pTalkbackParam");
		return IVS_PARA_INVALID;
	}

	if (NULL == pCameraCode)
	{
		BP_RUN_LOG_ERR(IVS_PARA_INVALID, "Start Voice Talkback", "NULL == pCameraCode");
		return IVS_PARA_INVALID;
	}

    if (NULL == pHandle)
    {
        BP_RUN_LOG_ERR(IVS_PARA_INVALID, "Start Voice Talkback", "NULL == pHandle");
        return IVS_PARA_INVALID;
    }

	int voiceCodeLen = strlen(pCameraCode);//lint !e713
	if( voiceCodeLen <= 0 || voiceCodeLen > IVS_DEV_CODE_LEN)
	{
		BP_RUN_LOG_ERR(IVS_PARA_INVALID, "Audio Mgr",
                       "StartVoiceTalkback invalid pCameraCode:%s",pCameraCode);
		return IVS_PARA_INVALID;
	}

    IVS_ULONG ulHandle = SDK_INVALID_HANDLE;
    int iRet = IVS_FAIL;

	//检测是否有WaveIn设备
	if (0 == waveInGetNumDevs())
	{
		BP_RUN_LOG_ERR(IVS_PLAYER_RET_AUDIO_DEVICE_NOT_FIND, "Start Voice Talkback",
			"waveInGetNumDevs fail");
		return IVS_PLAYER_RET_AUDIO_DEVICE_NOT_FIND;
	}


    //初始化talkback对象;
    CAudioObj *pAudioObj = GetFreeTalkBack(ulHandle);

    if (NULL != pAudioObj)
    {
        iRet = pAudioObj->StartVoiceTalkback(pTalkbackParam->uiProtocolType,
                                             pTalkbackParam->bDirectFirst,
                                             pCameraCode);
        if (IVS_SUCCEED != iRet)
        {
            pAudioObj->RetSet();
            BP_RUN_LOG_ERR(IVS_FAIL, "Start Voice Talkback",
                        "pAudioObj->StartVoiceTalkback failed iRet:%d",iRet);
            // modify by wanglei 00165153:2013.6.8 DTS2013052308679 
            // 如果已经有其他设备开启对讲，统一返回 IVS_SDK_TALKBACK_ERR_ALREADY_OPENED 错误码
            if (iRet ==  IVS_PLAYER_RET_VOICE_TALKBACK_STATUS_ERROR)
            {
                iRet = IVS_SDK_TALKBACK_ERR_ALREADY_OPENED;
            }
        }
        else
        {
            *pHandle = ulHandle;
        }
    }
    else
    {
        iRet = IVS_SDK_ERR_FINDAUDIOHANDLE_FAILED;
    }

    return iRet;
	
	//将handle与对应的语音类对象和设备信息对应关系保存起来,AddStream内部会分配内存保存数据，所以此处可以穿入临时变量指针。
}

//停止语音对讲
IVS_INT32 CAudioMgr::StopVoiceTalkback(INT32 iTalkbackHandle)
{
	//获取流信息里保存的语音对象指针
	CAudioObj* pAudioObj = m_HanleTalkBackMap[iTalkbackHandle];//lint !e732
	if(NULL == pAudioObj)
	{
		BP_RUN_LOG_ERR(IVS_NULL_POINTER, "AudioMgr StopVoiceTalkback",
                       "CAudioMgr::StopVoiceTalkback NULL == pAudioObj");
		return IVS_NULL_POINTER;
	}

	int iRet = pAudioObj->StopVoiceTalkback();
	if(IVS_SUCCEED != iRet)
	{
		BP_RUN_LOG_ERR(iRet, "AudioMgr StopVoiceTalkback",
            "CAudioMgr::StopVoiceTalkback pAudioObj->StopVoiceTalkback failed iRet:%d",iRet);
		return iRet;
	}

	//释放资源
	if(pAudioObj) //lint !e774
	{ 
		delete pAudioObj;
        
        // modify by wanglei 00165153:2013.5.8 DTS2013050708570
        // m_HanleTalkBackMap.second 中存放的是指针，删除指针对应的值空间后，不置空会导致野指针，类析构的时候会再次删除会导致访问越界
        m_HanleTalkBackMap.erase(iTalkbackHandle);//lint !e732
	}

	return IVS_SUCCEED;
}

//添加语音设备到广播组
IVS_INT32 CAudioMgr::AddBroadcastDevice(const IVS_CHAR* pCameraCode)
{
	if (NULL == pCameraCode)
	{
		BP_RUN_LOG_ERR(IVS_PARA_INVALID, "AudioMgr AddBroadcastDevice",
            "CAudioMgr::AddBroadcastDevice NULL == pCameraCode");
		return IVS_PARA_INVALID;
	}

	int voiceCodeLen = strlen(pCameraCode);//lint !e713
	if( voiceCodeLen <= 0 || voiceCodeLen > IVS_DEV_CODE_LEN)
	{
		BP_RUN_LOG_ERR(IVS_PARA_INVALID, "AudioMgr AddBroadcastDevice",
            "CAudioMgr::AddBroadcastDevice invalid pCameraCode:%s",pCameraCode);
		return IVS_PARA_INVALID;
	}

    INT32 iRet = m_audioObj.AddBroadcastDevice(pCameraCode);
    if(IVS_SUCCEED != iRet)
    {
		BP_RUN_LOG_ERR(iRet, "AudioMgr AddBroadcastDevice",
            "CAudioMgr::AddBroadcastDevice pAudioObj->StartVoiceTalkback failed iRet:%d",iRet);
        return iRet;
    }
	return IVS_SUCCEED;
}

//删除语音设备
IVS_INT32 CAudioMgr::DeleteBroadcastDevice(const IVS_CHAR* pCameraCode)
{
	if (NULL == pCameraCode)
	{
		BP_RUN_LOG_ERR(IVS_PARA_INVALID, "AudioMgr::DeleteBroadcastDevice",
            "CAudioMgr::DeleteBroadcastDevice NULL == pCameraCode");
		return IVS_PARA_INVALID;
	}

	int voiceCodeLen = strlen(pCameraCode);//lint !e713
	if( voiceCodeLen <= 0 || voiceCodeLen > IVS_DEV_CODE_LEN)
	{
		BP_RUN_LOG_ERR(IVS_PARA_INVALID, "AudioMgr::DeleteBroadcastDevice",
            "CAudioMgr::DeleteBroadcastDevice invalid pCameraCode:%s",pCameraCode);
		return IVS_PARA_INVALID;
	}

    INT32 iRet = m_audioObj.DeleteBroadcastDevice(pCameraCode);
    if(IVS_SUCCEED != iRet)
    {
		BP_RUN_LOG_ERR(iRet, "AudioMgr::DeleteBroadcastDevice",
            "CAudioMgr::DeleteBroadcastDevice pAudioObj->DeleteBroadcastDevice failed iRet:%d",iRet);
        return iRet;
    }
	return IVS_SUCCEED;
}

//开始实时语音广播
IVS_INT32 CAudioMgr::StartRealBroadcast()
{
    // wanglei:该地方不能删除，原因可能：	(void)GetLocalIpPort(); 在SetUserMgr调用
    m_audioObj.SetUserMgr(m_pUserMgr);
    
	//检测是否有WaveIn设备
	if (0 == waveInGetNumDevs())
	{
		BP_RUN_LOG_ERR(IVS_PLAYER_RET_AUDIO_DEVICE_NOT_FIND, "AudioMgr StartRealBroadcast",
			"waveInGetNumDevs fail");
		return IVS_PLAYER_RET_AUDIO_DEVICE_NOT_FIND;
	}

    INT32 iRet = m_audioObj.StartRealBroadcast();
    if(IVS_SUCCEED != iRet)
    {
		BP_RUN_LOG_ERR(iRet, "AudioMgr StartRealBroadcast",
            "CAudioMgr::StartRealBroadcast pAudioObj->StartVoiceTalkback failed iRet:%d",iRet);
    }		
	return iRet;//lint !e539
}

//停止实时语音广播
IVS_INT32 CAudioMgr::StopRealBroadcast()
{
	int iRet = m_audioObj.StopRealBroadcast();
    if(IVS_SUCCEED != iRet)
    {
		BP_RUN_LOG_ERR(iRet, "AudioMgr::StopRealBroadcast",
            "CAudioMgr::StopRealBroadcast pAudioObj->StopRealBroadcast failed iRet:%d",iRet);
    }	
	return iRet;
}

//开始文件语音广播
IVS_INT32 CAudioMgr::StartFileBroadcast(const IVS_CHAR* pFileName,IVS_INT32 iCycle)
{
	if(NULL == pFileName)//pFileName长度检验留在m_audioObj层
	{
		BP_RUN_LOG_ERR(IVS_PARA_INVALID, "AudioMgr StartFileBroadcast",
            "CAudioMgr::StartFileBroadcast NULL == pFileName");
		return IVS_PARA_INVALID;
	}

	if(iCycle<0 || iCycle>1)
	{
		BP_RUN_LOG_ERR(IVS_PARA_INVALID, "AudioMgr StartFileBroadcast",
            "CAudioMgr::StartFileBroadcast invalid iCycle");
		return IVS_PARA_INVALID;
	}

    // wanglei:该地方不能删除，原因可能：	(void)GetLocalIpPort(); 在SetUserMgr调用
    m_audioObj.SetUserMgr(m_pUserMgr);

    INT32 iRet = m_audioObj.StartFileBroadcast(pFileName, iCycle);
    if(IVS_SUCCEED != iRet)
    {
		BP_RUN_LOG_ERR(iRet, "AudioMgr StartFileBroadcast",
            "CAudioMgr::StartFileBroadcast pAudioObj->StartVoiceTalkback failed iRet:%d",iRet);
    }	
	return iRet;
}

//停止文件语音广播
IVS_INT32 CAudioMgr::StopFileBroadcast()
{
	int iRet = m_audioObj.StopFileBroadcast();
    if(IVS_SUCCEED != iRet)
    {
		BP_RUN_LOG_ERR(iRet, "AudioMgr StopFileBroadcast",
            "CAudioMgr::StopFileBroadcast pAudioObj->StopRealBroadcast failed iRet:%d",iRet);
    }	

	return iRet;
}

//设置音量
IVS_INT32 CAudioMgr::SetVolume(unsigned int uiPlayHandle, unsigned int uiVolumeValue)
{
    IVS_INT32 iRet = IVS_FAIL;

    CAudioObj *pAudioObj = m_HanleTalkBackMap[uiPlayHandle];
    if (NULL != pAudioObj)
    {
        iRet = pAudioObj->SetVolume(uiVolumeValue);
    }
    else
    {
        iRet = IVS_SDK_ERR_FINDVIDEOHANDLE_FAILED;
    }

    return iRet;
}

//获取音量
IVS_INT32 CAudioMgr::GetVolume(unsigned int uiPlayHandle, unsigned int* puiVolumeValue)
{	
    IVS_INT32 iRet = IVS_FAIL;

    CAudioObj *pAudioObj = m_HanleTalkBackMap[uiPlayHandle];
    if (NULL != pAudioObj)
    {
        iRet = pAudioObj->GetVolume(puiVolumeValue);
    }
    else
    {
        iRet = IVS_SDK_ERR_FINDVIDEOHANDLE_FAILED;
    }

    return iRet;
}

CAudioObj* CAudioMgr::GetAudioObj(IVS_ULONG ulHandle)
{
	CAudioObj *pAudioObj = NULL;
	HandleTalkBackMapIter iter = m_HanleTalkBackMap.begin();
	HandleTalkBackMapIter end  = m_HanleTalkBackMap.end();

	//查找空闲对象;
	for (; iter != end; iter++)
	{
		if (iter->first == ulHandle)
		{
			pAudioObj = dynamic_cast<CAudioObj*>(iter->second);//lint !e611
			break;
		}
	}
	return pAudioObj;
}//lint !e1788

// 通过RTSP获取PalyHandle(CAudioObj的基类地址)，用于事件上报
IVS_ULONG CAudioMgr::GetTalkBackPlayHandleByRtspHandle(IVS_ULONG ulRtspHandle)
{
	CAudioObj *pAudioObj = NULL;
	HandleTalkBackMapIter iter = m_HanleTalkBackMap.begin();
	HandleTalkBackMapIter end  = m_HanleTalkBackMap.end();

	IVS_ULONG ulPlayHandle = 0;
	//查找空闲对象;
	for (; iter != end; iter++)
	{
		pAudioObj = dynamic_cast<CAudioObj*>(iter->second);//lint !e611
		if (NULL == pAudioObj)
		{
			continue;
		}

		if (pAudioObj->IsExistTalkBackRtspHandle(ulRtspHandle))
		{
			ulPlayHandle = iter->first;
			break;
		}
	}

	return ulPlayHandle;
}//lint !e954

// 通过RTSP获取PalyHandle(CAudioObj的基类地址)，用于事件上报
IVS_ULONG CAudioMgr::GetBroadCastPlayHandleByRtspHandle(IVS_ULONG ulRtspHandle)
{
	bool bRet = m_audioObj.IsExistBroadCastRtspHandle(ulRtspHandle);
	if (bRet)
	{
		return m_audioObj.GetPlayHandle();
	}
	
	return 0;	
}

IVS_ULONG CAudioMgr::GetTalkBackPlayHandleByNetHandle(IVS_ULONG ulNetHandle)
{
    CAudioObj *pAudioObj = NULL;
    HandleTalkBackMapIter iter = m_HanleTalkBackMap.begin();
    HandleTalkBackMapIter end  = m_HanleTalkBackMap.end();

    IVS_ULONG ulPlayHandle = 0;
    //查找空闲对象;
    for (; iter != end; iter++)
    {
        pAudioObj = dynamic_cast<CAudioObj*>(iter->second);//lint !e611
        if (NULL == pAudioObj)
        {
            continue;
        }

        if (pAudioObj->IsExistTalkBackNetHandle(ulNetHandle))
        {
            ulPlayHandle = iter->first;
            break;
        }
    }

    return ulPlayHandle;
}//lint !e954

IVS_ULONG CAudioMgr::GetBroadCastPlayHandleByNetHandle(IVS_ULONG ulNetHandle)
{
    bool bRet = m_audioObj.IsExistBroadCastNetHandle(ulNetHandle);
    if (bRet)
    {
        return m_audioObj.GetPlayHandle();
    }

    return 0;	
}

bool CAudioMgr::IsBroadCast(IVS_ULONG ulPlayHandle) const
{
	unsigned long ulPlayHandleTmp = m_audioObj.GetPlayHandle();
	return (ulPlayHandle == ulPlayHandleTmp) ? true : false;
}

IVS_INT32 CAudioMgr::GetBroadCastDeviceCodeByConnHandle(IVS_ULONG ulRtspOrNetHandle, std::string& strDeviceCode)
{
    return m_audioObj.GetBroadCastDeviceCodeByConnHandle(ulRtspOrNetHandle, strDeviceCode);
}
IVS_INT32 CAudioMgr::GetTalkBackDeviceCodeByConnHandle(IVS_ULONG ulRtspOrNetHandle, std::string& strDeviceCode)
{
    CAudioObj *pAudioObj = NULL;
    HandleTalkBackMapIter iter = m_HanleTalkBackMap.begin();
    HandleTalkBackMapIter end  = m_HanleTalkBackMap.end();

    for (; iter != end; iter++)
    {
        pAudioObj = dynamic_cast<CAudioObj*>(iter->second);
        if (NULL == pAudioObj)
        {
            continue;
        }
        if (pAudioObj->GetRtspHandle() == (int)ulRtspOrNetHandle || pAudioObj->GetNetSourceHandle() == ulRtspOrNetHandle)
        {
            pAudioObj->GetCameraID(strDeviceCode);
            return IVS_SUCCEED;
        }
    }

    return IVS_FAIL;
}//lint !e954



