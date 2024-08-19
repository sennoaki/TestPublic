// Fill out your copyright notice in the Description page of Project Settings.


#include "AIAssist_PathFollowingComponent.h"


void UAIAssist_PathFollowingComponent::SetSplinePathMode(const USplineComponent* InSpline)
{
	mSplineComponent = InSpline;
}
