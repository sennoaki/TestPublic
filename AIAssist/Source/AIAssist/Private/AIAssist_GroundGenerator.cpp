// Fill out your copyright notice in the Description page of Project Settings.


#include "AIAssist_GroundGenerator.h"
#include "AIAssist_PerlinNoise.h"

AAIAssist_GroundGenerator::AAIAssist_GroundGenerator()
{
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AAIAssist_GroundGenerator::BeginPlay()
{
	Super::BeginPlay();
	
	GenerateGround();
}

// Called every frame
void AAIAssist_GroundGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AAIAssist_GroundGenerator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	DeleteGround();
}

void AAIAssist_GroundGenerator::GenerateGround()
{
	TSubclassOf<AActor> GroundActorClass = mGroundClass.LoadSynchronous();
	if (GroundActorClass == nullptr)
	{
		return;
	}

	AIAssist_PerlinNoise PerlinNoise(mSeed);
	const FVector BasePos = GetActorLocation();
	const FRotator BaseRot = GetActorRotation();
	const float PosInterval = 10.f;
	for (int32 x = 0; x < mGroundXNum; ++x)
	{
		FVector OffsetV((float)x * mIntervalV.X, 0.f, 0.f);
		for (int32 y = 0; y < mGroundYNum; ++y)
		{
			OffsetV.Y = (float)y * mIntervalV.Y;

			const double NoiseValue = PerlinNoise.ClacNoise(double(x) * mNoiseArgumentXYRatio, double(y) * mNoiseArgumentXYRatio, mNoiseArgumentsZValue);
			OffsetV.Z = NoiseValue * mIntervalV.Z;
			const FVector SpawnPos = BasePos + BaseRot.RotateVector(OffsetV);
			const FTransform SpawnTransform(GetActorRotation().Quaternion(), SpawnPos);
			AActor* NewGround = GetWorld()->SpawnActorDeferred<AActor>(GroundActorClass, SpawnTransform, this);
			if (NewGround == nullptr)
			{
				continue;
			}
			NewGround->FinishSpawning(SpawnTransform);
			mGroundActorList.Add(NewGround);
		}
	}
}


void AAIAssist_GroundGenerator::DeleteGround()
{
	for (AActor* GroundActor : mGroundActorList)
	{
		GroundActor->Destroy();
	}
	mGroundActorList.Empty();
}