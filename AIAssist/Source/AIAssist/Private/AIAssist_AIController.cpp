// Fill out your copyright notice in the Description page of Project Settings.


#include "AIAssist_AIController.h"
#include "AIAssist_PathFollowingComponent.h"

AAIAssist_AIController::AAIAssist_AIController(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer.SetDefaultSubobjectClass<UAIAssist_PathFollowingComponent>(TEXT("PathFollowingComponent")))
{

}
