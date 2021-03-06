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

/********************************************************************
filename	: 	AlarmAreaMgr.h
author		:	pkf57481
created		:	2012/11/7
description	:	告警防区管理;
copyright	:	Copyright (C) 2011-2015
history		:	2012/11/7 初始版本;
*********************************************************************/

#ifndef __ALARMAREA_MGR_H__
#define __ALARMAREA_MGR_H__

#include "hwsdk.h"
#include "ivs_error.h"
#include "SDKDef.h"

class CUserMgr;
class CAlarmAreaMgr
{
public:
	CAlarmAreaMgr(void);
	~CAlarmAreaMgr(void);

	void SetUserMgr(CUserMgr *pUserMgr);

public:
	// 新增防区
	IVS_INT32 AddAlarmArea(const IVS_CHAR* pReqXml,IVS_CHAR** pRspXml)  const;

	//修改防区
	IVS_INT32 ModifyAlarmArea(const IVS_CHAR* pReqXml) const;

	//删除防区
	IVS_INT32 DeleteAlarmArea(const IVS_CHAR* pDomainCode, IVS_UINT32 uiAlarmAreaId)const;

	//查看防区列表
	IVS_INT32 GetAlarmAreaList(const IVS_CHAR* pReqXml, IVS_CHAR** pRspXml)const;

	// 查看防区详情
	IVS_INT32 GetAlarmAreaInfo(const IVS_CHAR* pDomainCode,IVS_UINT32 uiAlarmAreaId,IVS_CHAR** pAlarmArea)const;

	// 手动布防
	IVS_INT32 StartAlarmAreaGuard(const IVS_CHAR* pDomainCode,IVS_UINT32 uiAlarmAreaId)const;

	// 手动撤防
	IVS_INT32 StopAlarmAreaGuard(const IVS_CHAR* pDomainCode, IVS_UINT32 uiAlarmAreaId)const;

	// 新增布防计划
	IVS_INT32 AllocAreaGuardPlan(const IVS_CHAR* pReqXml)const;

	// 修改布防计划
	IVS_INT32 ModifyAreaGuardPlan(const IVS_CHAR* pReqXml)const;

	// 删除布防计划
	IVS_INT32 DeleteAreaGuardPlan(const IVS_CHAR* pDomainCode,IVS_UINT32 uiAlarmAreaId)const;

	// 查看布防计划
	IVS_INT32 GetAreaGuardPlan(const IVS_CHAR* pDomainCode, IVS_UINT32 uiAlarmAreaId, IVS_CHAR** pRspXml)const;
private:
	IVS_INT32 AddAlarmAreaShadowDevCheck(CXml& reqXml)const;

private:
	CUserMgr *m_pUserMgr;
};
#endif // __ALARMAREA_MGR_H__
