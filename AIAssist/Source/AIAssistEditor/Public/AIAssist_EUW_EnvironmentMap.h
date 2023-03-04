// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "AIAssist_EUW_EnvironmentMap.generated.h"

class AAIAssist_EnvironmentMap;

UCLASS(BlueprintType, Blueprintable)
class AIASSISTEDITOR_API UAIAssist_EUW_EnvironmentMap : public UEditorUtilityWidget
{
	GENERATED_BODY()

protected:
	UFUNCTION(BlueprintCallable, Category = "AIAssist_EnvironmentMap")
	UWorld* GetEditorWorld() const;
	
	UFUNCTION(BlueprintCallable, Category = "AIAssist_EnvironmentMap")
	void	CreateAIAssist_EnvironmentMapData();

	UFUNCTION(BlueprintCallable, Category = "AIAssist_EnvironmentMap")
	void	SetSelectVoxelIndex(const int32 InIndex);
	UFUNCTION(BlueprintCallable, Category = "AIAssist_EnvironmentMap")
	void	SetDispInfo(bool bInDisp);
	UFUNCTION(BlueprintCallable, Category = "AIAssist_EnvironmentMap")
	void	SetDispAroundBox(bool bInDisp);
	UFUNCTION(BlueprintCallable, Category = "AIAssist_EnvironmentMap")
	void	SetDispVoxelList(bool bInDisp);
	UFUNCTION(BlueprintCallable, Category = "AIAssist_EnvironmentMap")
	void	SetDispDetailSelectVoxel(bool bInDisp);
	UFUNCTION(BlueprintCallable, Category = "AIAssist_EnvironmentMap")
	void	SetDispDetailNearCamera(bool bInDisp);
	UFUNCTION(BlueprintCallable, Category = "AIAssist_EnvironmentMap")
	void	SetCreateOnlySelectVoxel(const bool bInOnly);
	UFUNCTION(BlueprintCallable, Category = "AIAssist_EnvironmentMap")
	void	SetTestCurtain(const bool bInTest);
	UFUNCTION(BlueprintCallable, Category = "AIAssist_EnvironmentMap")
	void	SetTestPerformance(const bool bInTest);

	void	CollectAllAIAssist_EnvironmentMap(TArray<AAIAssist_EnvironmentMap*>& OutList) const;
};
