// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "AIAssist_EUW_ReferenceMap.generated.h"

class AAIAssist_ReferenceMap;

UCLASS(BlueprintType, Blueprintable)
class AIASSISTEDITOR_API UAIAssist_EUW_ReferenceMap : public UEditorUtilityWidget
{
	GENERATED_BODY()

protected:
	UFUNCTION(BlueprintCallable, Category = "AIAssist_ReferenceMap")
	UWorld* GetEditorWorld() const;
	
	UFUNCTION(BlueprintCallable, Category = "AIAssist_ReferenceMap")
	void	CreateAIAssist_ReferenceMapData();

	UFUNCTION(BlueprintCallable, Category = "AIAssist_ReferenceMap")
	void	SetSelectVoxelIndex(const int32 InIndex);
	UFUNCTION(BlueprintCallable, Category = "AIAssist_ReferenceMap")
	void	SetDispInfo(bool bInDisp);
	UFUNCTION(BlueprintCallable, Category = "AIAssist_ReferenceMap")
	void	SetDispArroundBox(bool bInDisp);
	UFUNCTION(BlueprintCallable, Category = "AIAssist_ReferenceMap")
	void	SetDispVoxelList(bool bInDisp);
	UFUNCTION(BlueprintCallable, Category = "AIAssist_ReferenceMap")
	void	SetDispDetailSelectVoxel(bool bInDisp);
	UFUNCTION(BlueprintCallable, Category = "AIAssist_ReferenceMap")
	void	SetCollectCheckVisibleBaseIndex(const int32 InIndex);
	UFUNCTION(BlueprintCallable, Category = "AIAssist_ReferenceMap")
	void	SetCollectCheckVisibleTargetIndex(const int32 InIndex);

	void	CollectAllAIAssist_ReferenceMap(TArray<AAIAssist_ReferenceMap*>& OutList) const;
};
