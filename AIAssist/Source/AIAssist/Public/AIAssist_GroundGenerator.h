// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AIAssist_GroundGenerator.generated.h"

UCLASS()
class AIASSIST_API AAIAssist_GroundGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	AAIAssist_GroundGenerator();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable)
	void ReGenerateGroundBP(int32 InSeed)
	{
		mSeed = InSeed;
		DeleteGround();
		GenerateGround();
	}

protected:
	void GenerateGround();
	void DeleteGround();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector mIntervalV = FVector(100.f, 100.f, 100.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftClassPtr<AActor> mGroundClass;
	UPROPERTY()
	TArray<AActor*> mGroundActorList;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float mNoiseArgumentXYRatio = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float mNoiseArgumentsZValue = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 mSeed = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 mGroundXNum= 50;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 mGroundYNum = 50;
};
