// Fill out your copyright notice in the Description page of Project Settings.


#include "AIAssist_EUW_ReferenceMap.h"

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
#include "AIAssist_ReferenceMap.h"

UWorld* UAIAssist_EUW_ReferenceMap::GetEditorWorld() const
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

void UAIAssist_EUW_ReferenceMap::CreateAIAssist_ReferenceMapData()
{
	TArray<AAIAssist_ReferenceMap*> AIAssist_ReferenceMapList;
	CollectAllAIAssist_ReferenceMap(AIAssist_ReferenceMapList);
	for (AAIAssist_ReferenceMap* AIAssist_ReferenceMap : AIAssist_ReferenceMapList)
	{
		AIAssist_ReferenceMap->EditorRequestCreateAIAssist_ReferenceMapData();
	}
}

void UAIAssist_EUW_ReferenceMap::SetSelectVoxelIndex(const int32 InIndex)
{
	TArray<AAIAssist_ReferenceMap*> AIAssist_ReferenceMapList;
	CollectAllAIAssist_ReferenceMap(AIAssist_ReferenceMapList);
	for (AAIAssist_ReferenceMap* AIAssist_ReferenceMap : AIAssist_ReferenceMapList)
	{
		AIAssist_ReferenceMap->DebugSetSelectVoxelIndex(InIndex);
	}
}

void UAIAssist_EUW_ReferenceMap::SetDispInfo(bool bInDisp)
{
	TArray<AAIAssist_ReferenceMap*> AIAssist_ReferenceMapList;
	CollectAllAIAssist_ReferenceMap(AIAssist_ReferenceMapList);
	for (AAIAssist_ReferenceMap* AIAssist_ReferenceMap : AIAssist_ReferenceMapList)
	{
		AIAssist_ReferenceMap->DebugSetDispInfo(bInDisp);
	}
}
void UAIAssist_EUW_ReferenceMap::SetDispArroundBox(bool bInDisp)
{
	TArray<AAIAssist_ReferenceMap*> AIAssist_ReferenceMapList;
	CollectAllAIAssist_ReferenceMap(AIAssist_ReferenceMapList);
	for (AAIAssist_ReferenceMap* AIAssist_ReferenceMap : AIAssist_ReferenceMapList)
	{
		AIAssist_ReferenceMap->DebugSetDispArroundBox(bInDisp);
	}
}
void UAIAssist_EUW_ReferenceMap::SetDispVoxelList(bool bInDisp)
{
	TArray<AAIAssist_ReferenceMap*> AIAssist_ReferenceMapList;
	CollectAllAIAssist_ReferenceMap(AIAssist_ReferenceMapList);
	for (AAIAssist_ReferenceMap* AIAssist_ReferenceMap : AIAssist_ReferenceMapList)
	{
		AIAssist_ReferenceMap->DebugSetDispVoxelList(bInDisp);
	}
}
void UAIAssist_EUW_ReferenceMap::SetDispDetailSelectVoxel(bool bInDisp)
{
	TArray<AAIAssist_ReferenceMap*> AIAssist_ReferenceMapList;
	CollectAllAIAssist_ReferenceMap(AIAssist_ReferenceMapList);
	for (AAIAssist_ReferenceMap* AIAssist_ReferenceMap : AIAssist_ReferenceMapList)
	{
		AIAssist_ReferenceMap->DebugSetDispDetailSelectVoxel(bInDisp);
	}
}

void UAIAssist_EUW_ReferenceMap::SetCollectCheckVisibleBaseIndex(const int32 InIndex)
{
	TArray<AAIAssist_ReferenceMap*> AIAssist_ReferenceMapList;
	CollectAllAIAssist_ReferenceMap(AIAssist_ReferenceMapList);
	for (AAIAssist_ReferenceMap* AIAssist_ReferenceMap : AIAssist_ReferenceMapList)
	{
		AIAssist_ReferenceMap->EditorSetCollectCheckVisibleBaseIndex(InIndex);
	}
}

void UAIAssist_EUW_ReferenceMap::SetCollectCheckVisibleTargetIndex(const int32 InIndex)
{
	TArray<AAIAssist_ReferenceMap*> AIAssist_ReferenceMapList;
	CollectAllAIAssist_ReferenceMap(AIAssist_ReferenceMapList);
	for (AAIAssist_ReferenceMap* AIAssist_ReferenceMap : AIAssist_ReferenceMapList)
	{
		AIAssist_ReferenceMap->EditorSetCollectCheckVisibleTargetIndex(InIndex);
	}
}

void UAIAssist_EUW_ReferenceMap::CollectAllAIAssist_ReferenceMap(TArray<AAIAssist_ReferenceMap*>& OutList) const
{
	TArray<AActor*> AIAssist_ReferenceMapList;
	UGameplayStatics::GetAllActorsOfClass(this, AAIAssist_ReferenceMap::StaticClass(), AIAssist_ReferenceMapList);
	for (AActor* Actor : AIAssist_ReferenceMapList)
	{
		if (AAIAssist_ReferenceMap* AIAssist_ReferenceMap = Cast<AAIAssist_ReferenceMap>(Actor))
		{
			OutList.Add(AIAssist_ReferenceMap);
		}
	}
}
