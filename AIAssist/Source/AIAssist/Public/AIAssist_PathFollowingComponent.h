// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Navigation/PathFollowingComponent.h"
#include "AIAssist_PathFollowingComponent.generated.h"

class USplineComponent;

UCLASS()
class AIASSIST_API UAIAssist_PathFollowingComponent : public UPathFollowingComponent
{
	GENERATED_BODY()
	

	void SetSplinePathMode(const USplineComponent* InSpline);

private:
	TWeakObjectPtr<const USplineComponent> mSplineComponent;
};
