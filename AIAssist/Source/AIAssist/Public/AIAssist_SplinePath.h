// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AIAssist_SplinePath.generated.h"

class USplineComponent;

UCLASS()
class AIASSIST_API AAIAssist_SplinePath : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAIAssist_SplinePath();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


protected:
	UPROPERTY(BlueprintReadWrite)
	USplineComponent* mSplineComponent = nullptr;
};
