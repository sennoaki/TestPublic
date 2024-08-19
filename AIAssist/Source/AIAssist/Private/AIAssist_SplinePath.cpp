// Fill out your copyright notice in the Description page of Project Settings.


#include "AIAssist_SplinePath.h"
#include "Components/SplineComponent.h"

// Sets default values
AAIAssist_SplinePath::AAIAssist_SplinePath()
{
	PrimaryActorTick.bCanEverTick = false;
	//RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	mSplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
}

// Called when the game starts or when spawned
void AAIAssist_SplinePath::BeginPlay()
{
	Super::BeginPlay();
	
}

