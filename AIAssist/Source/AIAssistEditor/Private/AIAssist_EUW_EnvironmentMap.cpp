// Fill out your copyright notice in the Description page of Project Settings.


#include "AIAssist_EUW_EnvironmentMap.h"

#include "Editor.h"
#include "LevelEditorViewport.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "LevelEditor.h"
#include "SLevelViewport.h"
#include "LevelEditorViewport.h"
#include "Debug/DebugDrawService.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "UnrealEd/Public/Editor.h"
#include "UnrealEd/Public/UnrealEdGlobals.h"
#include "UnrealEd/Classes/Editor/UnrealEdEngine.h"
#include "Misc/FileHelper.h"
#include "AIAssist_EnvironmentMap.h"

UWorld* UAIAssist_EUW_EnvironmentMap::GetEditorWorld() const
{
	const TIndirectArray<FWorldContext>& WorldContextList = GUnrealEd->GetWorldContexts();
	for (auto& WorldContext : WorldContextList)
	{
		if (WorldContext.WorldType == EWorldType::Editor)
		{
			return WorldContext.World();
		}
	}

	return nullptr;
}

void UAIAssist_EUW_EnvironmentMap::CreateAIAssist_EnvironmentMapData()
{
	TArray<AAIAssist_EnvironmentMap*> AIAssist_EnvironmentMapList;
	CollectAllAIAssist_EnvironmentMap(AIAssist_EnvironmentMapList);
	for (AAIAssist_EnvironmentMap* AIAssist_EnvironmentMap : AIAssist_EnvironmentMapList)
	{
		AIAssist_EnvironmentMap->EditorRequestCreateAIAssist_EnvironmentMapData();
	}
}

void UAIAssist_EUW_EnvironmentMap::SetSelectVoxelIndex(const int32 InIndex)
{
	TArray<AAIAssist_EnvironmentMap*> AIAssist_EnvironmentMapList;
	CollectAllAIAssist_EnvironmentMap(AIAssist_EnvironmentMapList);
	for (AAIAssist_EnvironmentMap* AIAssist_EnvironmentMap : AIAssist_EnvironmentMapList)
	{
		AIAssist_EnvironmentMap->DebugSetSelectVoxelIndex(InIndex);
	}
}

void UAIAssist_EUW_EnvironmentMap::SetDispInfo(bool bInDisp)
{
	TArray<AAIAssist_EnvironmentMap*> AIAssist_EnvironmentMapList;
	CollectAllAIAssist_EnvironmentMap(AIAssist_EnvironmentMapList);
	for (AAIAssist_EnvironmentMap* AIAssist_EnvironmentMap : AIAssist_EnvironmentMapList)
	{
		AIAssist_EnvironmentMap->DebugSetDispInfo(bInDisp);
	}
}
void UAIAssist_EUW_EnvironmentMap::SetDispAroundBox(bool bInDisp)
{
	TArray<AAIAssist_EnvironmentMap*> AIAssist_EnvironmentMapList;
	CollectAllAIAssist_EnvironmentMap(AIAssist_EnvironmentMapList);
	for (AAIAssist_EnvironmentMap* AIAssist_EnvironmentMap : AIAssist_EnvironmentMapList)
	{
		AIAssist_EnvironmentMap->DebugSetDispAroundBox(bInDisp);
	}
}
void UAIAssist_EUW_EnvironmentMap::SetDispVoxelList(bool bInDisp)
{
	TArray<AAIAssist_EnvironmentMap*> AIAssist_EnvironmentMapList;
	CollectAllAIAssist_EnvironmentMap(AIAssist_EnvironmentMapList);
	for (AAIAssist_EnvironmentMap* AIAssist_EnvironmentMap : AIAssist_EnvironmentMapList)
	{
		AIAssist_EnvironmentMap->DebugSetDispVoxelList(bInDisp);
	}
}
void UAIAssist_EUW_EnvironmentMap::SetDispDetailSelectVoxel(bool bInDisp)
{
	TArray<AAIAssist_EnvironmentMap*> AIAssist_EnvironmentMapList;
	CollectAllAIAssist_EnvironmentMap(AIAssist_EnvironmentMapList);
	for (AAIAssist_EnvironmentMap* AIAssist_EnvironmentMap : AIAssist_EnvironmentMapList)
	{
		AIAssist_EnvironmentMap->DebugSetDispDetailSelectVoxel(bInDisp);
	}
}
void UAIAssist_EUW_EnvironmentMap::SetDispDetailNearCamera(bool bInDisp)
{
	TArray<AAIAssist_EnvironmentMap*> AIAssist_EnvironmentMapList;
	CollectAllAIAssist_EnvironmentMap(AIAssist_EnvironmentMapList);
	for (AAIAssist_EnvironmentMap* AIAssist_EnvironmentMap : AIAssist_EnvironmentMapList)
	{
		AIAssist_EnvironmentMap->DebugSetDispDetailNearCamera(bInDisp);
	}
}
void UAIAssist_EUW_EnvironmentMap::SetCreateOnlySelectVoxel(bool bInOnly)
{
	TArray<AAIAssist_EnvironmentMap*> AIAssist_EnvironmentMapList;
	CollectAllAIAssist_EnvironmentMap(AIAssist_EnvironmentMapList);
	for (AAIAssist_EnvironmentMap* AIAssist_EnvironmentMap : AIAssist_EnvironmentMapList)
	{
		AIAssist_EnvironmentMap->EditorSetCreateOnlySelectVoxel(bInOnly);
	}
}
void UAIAssist_EUW_EnvironmentMap::SetTestCurtain(const bool bInTest)
{
	TArray<AAIAssist_EnvironmentMap*> AIAssist_EnvironmentMapList;
	CollectAllAIAssist_EnvironmentMap(AIAssist_EnvironmentMapList);
	for (AAIAssist_EnvironmentMap* AIAssist_EnvironmentMap : AIAssist_EnvironmentMapList)
	{
		AIAssist_EnvironmentMap->EditorSetTestCurtain(bInTest);
	}
}
void UAIAssist_EUW_EnvironmentMap::SetTestPerformance(const bool bInTest)
{
	TArray<AAIAssist_EnvironmentMap*> AIAssist_EnvironmentMapList;
	CollectAllAIAssist_EnvironmentMap(AIAssist_EnvironmentMapList);
	for (AAIAssist_EnvironmentMap* AIAssist_EnvironmentMap : AIAssist_EnvironmentMapList)
	{
		AIAssist_EnvironmentMap->EditorSetTestPerformance(bInTest);
	}
}

void UAIAssist_EUW_EnvironmentMap::CollectAllAIAssist_EnvironmentMap(TArray<AAIAssist_EnvironmentMap*>& OutList) const
{
	TArray<AActor*> AIAssist_EnvironmentMapList;
	UGameplayStatics::GetAllActorsOfClass(this, AAIAssist_EnvironmentMap::StaticClass(), AIAssist_EnvironmentMapList);
	for (AActor* Actor : AIAssist_EnvironmentMapList)
	{
		if (AAIAssist_EnvironmentMap* AIAssist_EnvironmentMap = Cast<AAIAssist_EnvironmentMap>(Actor))
		{
			OutList.Add(AIAssist_EnvironmentMap);
		}
	}
}
