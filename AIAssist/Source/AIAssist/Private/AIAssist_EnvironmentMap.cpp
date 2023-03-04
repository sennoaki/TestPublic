// AIが地形認知するためのデータ解析＆提供

#include "AIAssist_EnvironmentMap.h"

#include "Components/BoxComponent.h"

#include "Engine/Canvas.h"
#include "Debug/DebugDrawService.h"
#include "DrawDebugHelpers.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "GameFramework/PlayerController.h"
#include "AIAssist_Compress.h"

#if USE_CSDEBUG
#include "CSDebugSubsystem.h"
#include "CSDebugUtility.h"
#include "DebugMenu/CSDebugMenuManager.h"
#include "DebugMenu/CSDebugMenuNodeGetter.h"
#include "DebugInfoWindow/CSDebugInfoWindowText.h"
#endif
#include "Misc/Attribute.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/TargetPoint.h"

DECLARE_STATS_GROUP(TEXT("AIAssist"), STATGROUP_AIAssist, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("AIAssist_EnvironmentMap_CheckCurtain"), STAT_AIAssist_EnvironmentMap_CheckCurtain, STATGROUP_AIAssist);
DECLARE_CYCLE_STAT(TEXT("AIAssist_EnvironmentMap_CheckCurtainWhile"), STAT_AIAssist_EnvironmentMap_CheckCurtainWhile, STATGROUP_AIAssist);

/**
 * @brief	周囲のVoxelの通し番号取得(x,y,zの順にシフトして番号振り)
 */
int32	CalcAroundVoxelLinkIndex(const int32 InX, const int32 InY, const int32 InZ)
{
	if (InX < 0 || InX >= 3
		|| InY < 0 || InY >= 3
		|| InZ < 0 || InZ >= 3
		)
	{
		return -1;
	}
	return InX + InY * 3 + InZ * 9;
}

AAIAssist_EnvironmentMap::AAIAssist_EnvironmentMap(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("SceneComponent"));
	RootComponent->Mobility = EComponentMobility::Static;

#if WITH_EDITOR
	mEditorVoxelBox = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(this, TEXT("EditorVoxelBox"));
	mEditorVoxelBox->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	mEditorVoxelBox->SetVisibility(true);
	mEditorVoxelBox->SetHiddenInGame(true);
	mEditorVoxelBox->SetCollisionProfileName(FName("NoCollision"));
	EditorUpdateAroundBox();
#endif
}

void AAIAssist_EnvironmentMap::PostInitProperties()
{
	Super::PostInitProperties();
}
void AAIAssist_EnvironmentMap::PostLoad()
{
	Super::PostLoad();

#if UE_BUILD_DEVELOPMENT
	if(GetClass()->GetDefaultObject() != this)
	{
		DeubgRequestDraw(true);
	}
#endif
#if WITH_EDITOR
	EditorUpdateAroundBox();
	SetupParam();
	SetupVoxelMap();
#endif
}
void AAIAssist_EnvironmentMap::BeginPlay()
{
	Super::BeginPlay();

	SetupParam();
	SetupVoxelMap();

#if USE_CSDEBUG
	UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	UCSDebugSubsystem* CSDebugSubsystem = GameInstance->GetSubsystem<UCSDebugSubsystem>();
	UCSDebugMenuManager* CSDebugMenu = CSDebugSubsystem->GetDebugMenuManager();
	CSDebugMenu->AddNodePropertyBool(TEXT("AIAssist_EnvironmentMap/DispInfo"), mbDebugDispInfo);
	CSDebugMenu->AddNodePropertyBool(TEXT("AIAssist_EnvironmentMap/DispAroundBox"), mbDebugDispAroundBox);
	CSDebugMenu->AddNodePropertyBool(TEXT("AIAssist_EnvironmentMap/DispVoxelList"), mbDebugDispVoxelList);
	CSDebugMenu->AddNodePropertyBool(TEXT("AIAssist_EnvironmentMap/DispPlayerVisibleVoxel"), mbDebugDispPlayerVisibleVoxel);
#if WITH_EDITOR
	CSDebugMenu->AddNodePropertyBool(TEXT("AIAssist_EnvironmentMap/TestPerformance"), mbEditorTestPerformance);
#endif
#endif
}

void AAIAssist_EnvironmentMap::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

#if UE_BUILD_DEVELOPMENT
	DeubgRequestDraw(false);
#endif
}

void AAIAssist_EnvironmentMap::BeginDestroy()
{
#if UE_BUILD_DEVELOPMENT
	DeubgRequestDraw(false);
#endif
#if WITH_EDITOR
	RunEditorTick(false);
	if (mEditorAsyncTask)
	{
		mEditorAsyncTask->EnsureCompletion();
		delete mEditorAsyncTask;
		mEditorAsyncTask = nullptr;
	}
#endif

	Super::BeginDestroy();
}

void AAIAssist_EnvironmentMap::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

/**
 * @brief	ローカル保持のパラメータ更新
 */
void AAIAssist_EnvironmentMap::SetupParam()
{
	mAroundBoxCenterPos = CalcAroundBoxCenterPos();
	mAroundBoxExtentV = CalcAroundBoxExtentV();
	mAroundBoxBeginPos = CalcAroundBoxBeginPos();
}

/**
 * @brief	mActiveVoxelListを元にmVoxelMapを準備
 */
void AAIAssist_EnvironmentMap::SetupVoxelMap()
{
	mVoxelMap.Empty();
	for (FAIAssist_EnvironmentMapVoxel& Voxel : mActiveVoxelList)
	{
		mVoxelMap.Add(Voxel.mVoxelIndex, &Voxel);
	}
}

/**
 * @brief	Voxel全体を覆うBoxのExtentV
 */
FVector AAIAssist_EnvironmentMap::CalcAroundBoxExtentV() const
{
	FVector ExtentV(
		mVoxelLength * static_cast<float>(mVoxelXNum),
		mVoxelLength * static_cast<float>(mVoxelYNum),
		mVoxelLength * static_cast<float>(mVoxelZNum)
	);
	ExtentV *= 0.5f;
	return ExtentV;
}
/**
 * @brief	Voxel全体を覆うBoxの中心座標
 */
FVector AAIAssist_EnvironmentMap::CalcAroundBoxCenterPos() const
{
	const FVector ExtentV = CalcAroundBoxExtentV();
	return GetActorLocation() + FVector(0.f, 0.f, ExtentV.Z);
}

/**
 * @brief	Voxel全体を覆うBoxの計算基点
 */
FVector AAIAssist_EnvironmentMap::CalcAroundBoxBeginPos() const
{
	const FVector ExtentV = CalcAroundBoxExtentV();
	return GetActorLocation() - FVector(ExtentV.X, ExtentV.Y, 0.f);
}

/**
 * @brief	指定Voxelの中心座標
 */
FVector AAIAssist_EnvironmentMap::CalcVoxelCenterPos(const int32 InX, const int32 InY, const int32 InZ) const
{
	FVector VoxelPos(
		mVoxelLength * static_cast<float>(InX),
		mVoxelLength * static_cast<float>(InY),
		mVoxelLength * static_cast<float>(InZ)
	);
	const float HalfLength = mVoxelLength * 0.5f;
	VoxelPos += FVector(HalfLength, HalfLength, HalfLength);
	VoxelPos += mAroundBoxBeginPos;
	return VoxelPos;
}

/**
 * @brief	Voxelのx,y,zのIndexからUIDを取得
 */
int32 AAIAssist_EnvironmentMap::CalcVoxelUID(const int32 InX, const int32 InY, const int32 InZ) const
{
	if (InX < 0 || InX >= mVoxelXNum
		|| InY < 0 || InY >= mVoxelYNum
		|| InZ < 0 || InZ >= mVoxelZNum
		)
	{
		return INDEX_NONE;
	}
	const FAIAssist_EnvironmentMapVoxelIndex VoxelIndex(InX, InY, InZ);
	return VoxelIndex.mUID;
}

int32 AAIAssist_EnvironmentMap::CalcVoxelUID(const FVector& InPos) const
{
	int32 IndexX = 0;
	int32 IndexY = 0;
	int32 IndexZ = 0;
	if (!CalcVoxelIndexXYZ(IndexX, IndexY, IndexZ, InPos))
	{
		return INDEX_NONE;
	}
	return CalcVoxelUID(IndexX, IndexY, IndexZ);
}

/**
 * @brief	Voxel全体のIndexからx,y,zのIndexを取得
 */
bool AAIAssist_EnvironmentMap::CalcVoxelIndexXYZ(int32& OutX, int32& OutY, int32& OutZ, const int32 InUID) const
{
	if (InUID < 0
		|| InUID > 16777216)
	{
		OutX = INDEX_NONE;
		OutY = INDEX_NONE;
		OutZ = INDEX_NONE;
		return false;
	}
	const FAIAssist_EnvironmentMapVoxelIndex VoxelIndex(InUID);
	OutX = VoxelIndex.mIndex.mX;
	OutY = VoxelIndex.mIndex.mY;
	OutZ = VoxelIndex.mIndex.mZ;
	return true;
}
/**
 * @brief	指定座標を管理するVoxelのx,y,zのIndex取得
 */
bool AAIAssist_EnvironmentMap::CalcVoxelIndexXYZ(int32& OutX, int32& OutY, int32& OutZ, const FVector& InPos) const
{
	const FVector TargetV = InPos - mAroundBoxBeginPos;
	const float RcpVoxelLength = 1.f / mVoxelLength;
	OutX = static_cast<int32>(TargetV.X * RcpVoxelLength);
	OutY = static_cast<int32>(TargetV.Y * RcpVoxelLength);
	OutZ = static_cast<int32>(TargetV.Z * RcpVoxelLength);

	bool bInvalidValue = false;
	if (OutX < 0 || OutX >= mVoxelXNum)
	{
		OutX = INDEX_NONE;
		bInvalidValue = true;
	}
	if (OutY < 0 || OutY >= mVoxelYNum)
	{
		OutY = INDEX_NONE;
		bInvalidValue = true;
	}
	if (OutZ < 0 || OutZ >= mVoxelZNum)
	{
		OutZ = INDEX_NONE;
		bInvalidValue = true;
	}

	return !bInvalidValue;
}

/**
 * @brief	指定のVoxelが存在してるかどうか
 */
bool AAIAssist_EnvironmentMap::IsOwnVoxel(const int32 InX, const int32 InY, const int32 InZ) const
{
	return IsOwnVoxel(CalcVoxelUID(InX, InY, InZ));
}
bool AAIAssist_EnvironmentMap::IsOwnVoxel(const int32 InUID) const
{
	if (InUID == INDEX_NONE)
	{
		return false;
	}
	for (const FAIAssist_EnvironmentMapVoxel& Voxel : mActiveVoxelList)
	{
		if (Voxel.mVoxelIndex == InUID)
		{
			return true;
		}
	}
	return false;
}

/**
 * @brief	指定のVoxel間が遮蔽されてるかどうか
 */
bool AAIAssist_EnvironmentMap::CheckCurtain(const int32 InBaseIndex, const int32 InTargetIndex) const
{
	SCOPE_CYCLE_COUNTER(STAT_AIAssist_EnvironmentMap_CheckCurtain);
	if (InBaseIndex == InTargetIndex)
	{
		return false;
	}

	int32 BaseX = 0;
	int32 BaseY = 0;
	int32 BaseZ = 0;
	if (!CalcVoxelIndexXYZ(BaseX, BaseY, BaseZ, InBaseIndex))
	{
		return false;
	}
	int32 TargetX = 0;
	int32 TargetY = 0;
	int32 TargetZ = 0;
	if (!CalcVoxelIndexXYZ(TargetX, TargetY, TargetZ, InTargetIndex))
	{
		return false;
	}

	const FVector BaseVoxelPos = CalcVoxelCenterPos(BaseX, BaseY, BaseZ);
	const FVector TargetVoxelPos = CalcVoxelCenterPos(TargetX, TargetY, TargetZ);
	const FVector TargetNV = FVector(TargetVoxelPos - BaseVoxelPos).GetSafeNormal();
	
	FVector OffsetV = FVector(0.5f, 0.5f, 0.5f);
	if (TargetNV.X < 0.f)
	{
		OffsetV.X = -0.5f;
	}
	if (TargetNV.Y < 0.f)
	{
		OffsetV.Y = -0.5f;
	}
	if (TargetNV.Z < 0.f)
	{
		OffsetV.Z = -0.5f;
	}
	int32 OffsetX = 0;
	int32 OffsetY = 0;
	int32 OffsetZ = 0;
	int32 OldIndexX = BaseX;
	int32 OldIndexY = BaseY;
	int32 OldIndexZ = BaseZ;
	const int32 TargetOffsetXLen = FMath::Abs(TargetX - BaseX);
	const int32 TargetOffsetYLen = FMath::Abs(TargetY - BaseY);
	const int32 TargetOffsetZLen = FMath::Abs(TargetZ - BaseZ);
	while (FMath::Abs(OffsetX) <= TargetOffsetXLen
		&& FMath::Abs(OffsetY) <= TargetOffsetYLen
		&& FMath::Abs(OffsetZ) <= TargetOffsetZLen
		)
	{
		const int32 IndexX = BaseX + OffsetX;
		const int32 IndexY = BaseY + OffsetY;
		const int32 IndexZ = BaseZ + OffsetZ;

		const int32 UID = CalcVoxelUID(OldIndexX, OldIndexY, OldIndexZ);
		if (const FAIAssist_EnvironmentMapVoxel* const* Element = mVoxelMap.Find(UID))
		{
			const FAIAssist_EnvironmentMapVoxel& Voxel = **Element;
			const int32 NextOffsetX = IndexX - OldIndexX;
			const int32 NextOffsetY = IndexY - OldIndexY;
			const int32 NextOffsetZ = IndexZ - OldIndexZ;
			const int32 AroundVoxelCurtainBitIndex = CalcAroundVoxelLinkIndex(NextOffsetX + 1, NextOffsetY + 1, NextOffsetZ + 1);
			if (Voxel.mAroundVoxelCurtainBit & 1 << AroundVoxelCurtainBitIndex)
			{
				return true;
			}
		}

		OffsetV += TargetNV;
		OffsetX = static_cast<int32>(OffsetV.X);
		OffsetY = static_cast<int32>(OffsetV.Y);
		OffsetZ = static_cast<int32>(OffsetV.Z);
		OldIndexX = IndexX;
		OldIndexY = IndexY;
		OldIndexZ = IndexZ;
	}

	return false;
}

#if UE_BUILD_DEVELOPMENT

/**
 * @brief	デバッグ対象のVoxelIndexを指定
 */
void AAIAssist_EnvironmentMap::DebugSetSelectVoxelIndex(const int32 InIndex)
{
	mDebugSelectVoxelIndex = InIndex;
}
void AAIAssist_EnvironmentMap::DebugSetSelectVoxelIndex(const int32 InX, const int32 InY, const int32 InZ)
{
	mDebugSelectVoxelIndex = CalcVoxelUID(InX, InY, InZ);
}

/**
 * @brief	Draw登録のon/off
 */
void	AAIAssist_EnvironmentMap::DeubgRequestDraw(const bool bInActive)
{
	if (bInActive)
	{
		if (!mDebugDrawHandle.IsValid())
		{
			const auto DebugDrawDelegate = FDebugDrawDelegate::CreateUObject(this, &AAIAssist_EnvironmentMap::DebugDraw);
			if (DebugDrawDelegate.IsBound())
			{
				mDebugDrawHandle = UDebugDrawService::Register(TEXT("GameplayDebug"), DebugDrawDelegate);
			}
		}
	}
	else
	{
		if (mDebugDrawHandle.IsValid())
		{
			UDebugDrawService::Unregister(mDebugDrawHandle);
			mDebugDrawHandle.Reset();
		}
	}
}
/**
 * @brief	Draw
 */
void	AAIAssist_EnvironmentMap::DebugDraw(UCanvas* InCanvas, APlayerController* InPlayerController)
{
#if WITH_EDITOR
	if (mEditorAsyncTask
		&& !mEditorAsyncTask->IsDone())
	{
		return;
	}
	if (UCSDebugUtility::IsNeedStopDebugDraw(GetWorld()))
	{
		return;
	}
#endif

	if (mbDebugDispInfo)
	{
		DebugDrawInfo(InCanvas);
	}
	if (mbDebugDispAroundBox)
	{
		DebugDrawAroundBox();
	}
	if (mbDebugDispVoxelList)
	{
		DebugDrawVoxelList(InCanvas);
	}
	if (mbDebugDispPlayerVisibleVoxel)
	{
		DebugDrawPlayerVisibleVoxel();
	}
	

	for (const FHitResult& Result : mEditorCreateVoxelCheckCollisionLog)
	{
		DrawDebugPoint(GetWorld(), Result.TraceStart, 10.f, FColor::Blue);
		DrawDebugPoint(GetWorld(), Result.TraceEnd, 10.f, FColor::Yellow);
		FColor LineColor = FColor::Blue;
		if (Result.bBlockingHit)
		{
			LineColor = FColor::Red;
			DrawDebugPoint(GetWorld(), Result.ImpactPoint, 10.f, LineColor);
		}
		DrawDebugLine(GetWorld(), Result.TraceStart, Result.TraceEnd, LineColor);
	}

#if WITH_EDITOR
	if (mbEditorTestCurtain)
	{
		EditorTestCheckCurtain(InCanvas);
	}
#endif
}

void AAIAssist_EnvironmentMap::DebugDrawInfo(UCanvas* InCanvas)
{
	FCSDebugInfoWindowText DebugInfo;
	DebugInfo.SetWindowName(TEXT("AIAssist_EnvironmentMap"));
	DebugInfo.AddText(FString::Printf(TEXT("VoxelNum : %d"), mActiveVoxelList.Num()));

	int32 TotalSize = 0;
	TotalSize += sizeof(*this);
	
	const int32 VoxelSize = sizeof(FAIAssist_EnvironmentMapVoxel);
	const int32 VoxelListSize = mActiveVoxelList.Num() * VoxelSize;
	TotalSize += VoxelListSize;
	
	const int32 VoxelMapSize = mVoxelMap.Num() * 8;
	TotalSize += VoxelMapSize;

	DebugInfo.AddText(FString::Printf(TEXT("TotalSize : %.1f(KB)"), static_cast<float>(TotalSize) / 1024.f));
	DebugInfo.AddText(FString::Printf(TEXT("--Voxel : %d(B)"), VoxelSize));
	DebugInfo.AddText(FString::Printf(TEXT("--VoxelListSize : %.1f(KB)"), static_cast<float>(VoxelListSize) / 1024.f));
	DebugInfo.AddText(FString::Printf(TEXT("--VoxelMapSize : %.1f(KB)"), static_cast<float>(VoxelMapSize) / 1024.f));

#if WITH_EDITORONLY_DATA
	if (mbEditorTestPerformance)
	{
		EditorTestPerformance(DebugInfo);
	}
#endif

	DebugInfo.Draw(InCanvas, FVector2D(50.f, 50.f));
}

/**
 * @brief	Voxel全体を覆うBoxをデバッグ表示
 */
void AAIAssist_EnvironmentMap::DebugDrawAroundBox()
{
	DrawDebugBox(GetWorld(),
		CalcAroundBoxCenterPos(),
		CalcAroundBoxExtentV(),
		FColor::Red,
		false,
		-1.f,
		0,
		10.f
	);
}

/**
 * @brief	Voxelをデバッグ表示
 */
void AAIAssist_EnvironmentMap::DebugDrawVoxelList(UCanvas* InCanvas)
{
	FVector ViewPos = FVector::ZeroVector;
	if (const FSceneView* View = InCanvas->SceneView)
	{
		ViewPos = View->ViewMatrices.GetViewOrigin();
	}
	const float ViewBorderDistanceSq = FMath::Square(3000.f);
	const float HalfLengthShort = mVoxelLength * 0.5f - 1.f;//重なり回避のために少し小さく
	const float QuarterVoxelLength = mVoxelLength * 0.25f;
	const float QuarterVoxelLengthShort = mVoxelLength * 0.25f - 1.f;//重なり回避のために少し小さく
	const FVector DispVoxelExtentV(HalfLengthShort, HalfLengthShort, HalfLengthShort);
	const FVector DispSubVoxelExtentV(QuarterVoxelLengthShort, QuarterVoxelLengthShort, QuarterVoxelLengthShort);

	for (const FAIAssist_EnvironmentMapVoxel& Voxel : mActiveVoxelList)
	{
		const bool bSelectVoxel = (Voxel.mVoxelIndex == mDebugSelectVoxelIndex);
		if (mbDebugDispDetailSelectVoxel
			&& !bSelectVoxel)
		{
			continue;
		}

		int32 IndexX = 0;
		int32 IndexY = 0;
		int32 IndexZ = 0;
		if (!CalcVoxelIndexXYZ(IndexX, IndexY, IndexZ, Voxel.mVoxelIndex))
		{
			continue;
		}
		const FVector VoxelCenterPos = CalcVoxelCenterPos(IndexX, IndexY, IndexZ);
		if (FVector::DistSquared(ViewPos, VoxelCenterPos) > ViewBorderDistanceSq)
		{
			continue;
		}

		const float DispWindowDistance = mVoxelLength * 2.f;
		bool bDispDetail = mbDebugDispDetailSelectVoxel;
		if (!bDispDetail
			&& mbDebugDispDetailNearCamera)
		{
			const FSceneView* View = InCanvas->SceneView;
			if (View
				&& FVector::DistSquared(View->ViewMatrices.GetViewOrigin(), VoxelCenterPos) < FMath::Square(DispWindowDistance))
			{
				bDispDetail = true;
			}
		}

		DrawDebugBox(GetWorld(), VoxelCenterPos, DispVoxelExtentV, FColor::Green);

#if USE_CSDEBUG
		FCSDebugInfoWindowText DebugInfo;
		DebugInfo.AddText(FString::Printf(TEXT("UID : %d"), Voxel.mVoxelIndex));
		DebugInfo.AddText(FString::Printf(TEXT("X:%d Y:%d Z:%d"), IndexX, IndexY, IndexZ));
		DebugInfo.AddText(FString::Printf(TEXT("GroundHeightRatio : %d"), Voxel.mGroundHeightRatio_Compress));
		DebugInfo.AddText(FString::Printf(TEXT("mAroundVoxelCurtainBit : %d"), Voxel.mAroundVoxelCurtainBit));
		DebugInfo.AddText(FString::Printf(TEXT("TouchGround : %d"), Voxel.mbTouchGround));
		if (bDispDetail)
		{
			if(Voxel.mbTouchGround)
			{
				FAIAssist_CompressOneFloat CompressOneFloat;
				CompressOneFloat.SetData(Voxel.mGroundHeightRatio_Compress);
				const float GroundHeightRaito = CompressOneFloat.GetValue();
				const float GroundHeightZ = (VoxelCenterPos.Z - mVoxelLength * 0.5f) + mVoxelLength * GroundHeightRaito;
				const FVector GroundHeightPos(VoxelCenterPos.X, VoxelCenterPos.Y, GroundHeightZ);
				DrawDebugPoint(GetWorld(), GroundHeightPos, 20.f, FColor::Red);
			}

			for (int32 OffsetIndexX = -1; OffsetIndexX <= 1; ++OffsetIndexX)
			{
				for (int32 OffsetIndexY = -1; OffsetIndexY <= 1; ++OffsetIndexY)
				{
					for (int32 OffsetIndexZ = -1; OffsetIndexZ <= 1; ++OffsetIndexZ)
					{
						const int32 AroundVoxelIndexX = IndexX + OffsetIndexX;
						const int32 AroundVoxelIndexY = IndexY + OffsetIndexY;
						const int32 AroundVoxelIndexZ = IndexZ + OffsetIndexZ;
						const int32 AroundVoxelIndex = CalcVoxelUID(AroundVoxelIndexX, AroundVoxelIndexY, AroundVoxelIndexZ);
						if (AroundVoxelIndex != INDEX_NONE
							&& AroundVoxelIndex != Voxel.mVoxelIndex)
						{
							FColor FaceColor = FColor::Blue;
							const int32 AroundVoxelLinkIndex = CalcAroundVoxelLinkIndex(OffsetIndexX+1, OffsetIndexY+1, OffsetIndexZ+1);
							if (Voxel.mAroundVoxelCurtainBit & 1 << AroundVoxelLinkIndex)
							{
								FaceColor = FColor::Red;
							}
							//const FVector AroundVoxelCenterPos = CalcVoxelCenterPos(AroundVoxelIndexX, AroundVoxelIndexY, AroundVoxelIndexZ);
							//const FVector AroundVoxelNV = FVector(AroundVoxelCenterPos - VoxelCenterPos).GetSafeNormal();
							const FVector AroundVoxelNV = FVector(static_cast<float>(OffsetIndexX), static_cast<float>(OffsetIndexY), static_cast<float>(OffsetIndexZ)).GetSafeNormal();
							const FVector DispLinePos = VoxelCenterPos + AroundVoxelNV*HalfLengthShort;
							DrawDebugPoint(GetWorld(), DispLinePos, 10.f, FaceColor);
							DrawDebugLine(GetWorld(), VoxelCenterPos, DispLinePos, FaceColor, false, -1.f, 255);

							FCSDebugInfoWindowText DebugInfoAround;
							DebugInfoAround.AddText(FString::Printf(TEXT("Index : %d"), AroundVoxelLinkIndex));
							DebugInfoAround.SetWindowFrameColor(FColor::White);
							DebugInfoAround.Draw(InCanvas, DispLinePos, 100.f);
						}
					}
				}
			}
		}
		DebugInfo.Draw(InCanvas, VoxelCenterPos, DispWindowDistance);
#endif
	}
}

/**
 * @brief	Player位置のVoxelから見えるVoxelをデバッグ表示
 */
void AAIAssist_EnvironmentMap::DebugDrawPlayerVisibleVoxel()
{
	FVector PlayerPos = FVector::ZeroVector;
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		if (const APlayerController* PlayerController = Iterator->Get())
		{
			if (const APawn* Pawn = PlayerController->GetPawn())
			{
				PlayerPos = Pawn->GetActorLocation();
				break;
			}
		}
	}

	int32 IndexX = 0;
	int32 IndexY = 0;
	int32 IndexZ = 0;
	if (!CalcVoxelIndexXYZ(IndexX, IndexY, IndexZ, PlayerPos))
	{
		return;
	}

	const float HalfLengthShort = mVoxelLength * 0.5f - 1.f;//重なり回避のために少し小さく
	const FVector DispVoxelExtentV(HalfLengthShort, HalfLengthShort, HalfLengthShort);
	const FVector VoxelPos = CalcVoxelCenterPos(IndexX, IndexY, IndexZ);
	DrawDebugBox(GetWorld(), VoxelPos, DispVoxelExtentV, FColor::Green);

	const int32 BaseVoxelUID = CalcVoxelUID(IndexX, IndexY, IndexZ);
	const auto VoxelMapElement = mVoxelMap.Find(BaseVoxelUID);
	if (VoxelMapElement == nullptr)
	{
		return;
	}
	const FAIAssist_EnvironmentMapVoxel* BaseVoxel = *VoxelMapElement;
	if (BaseVoxel == nullptr)
	{
		return;
	}
}


#endif//UE_BUILD_DEVELOPMENT

#if WITH_EDITOR

/**
 * @brief	プロパティ変更時
 */
void AAIAssist_EnvironmentMap::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	EditorUpdateAroundBox();

	SetupParam();
}

/**
 * @brief	Actorを動かした時
 */
void AAIAssist_EnvironmentMap::PostEditMove(bool bFinished)
{
	if (bFinished)
	{//回転とスケールは編集禁止
		SetActorRotation(FRotator::ZeroRotator.Quaternion());
		SetActorScale3D(FVector::OneVector);

		SetupParam();
	}
	Super::PostEditMove(bFinished);
}

/**
 * @brief	Editor処理用のTickのon/off
 */
void AAIAssist_EnvironmentMap::RunEditorTick(const bool bInRun)
{
	if (bInRun)
	{
		if (!mEditorTickHandle.IsValid())
		{
			mEditorTickHandle = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &AAIAssist_EnvironmentMap::EditorTick));
		}
	}
	else
	{
		FTicker::GetCoreTicker().RemoveTicker(mEditorTickHandle);
	}
}

/**
 * @brief	Editor処理用のTick
 */
bool AAIAssist_EnvironmentMap::EditorTick(float InDeltaSecond)
{
	if (mEditorAsyncTask
		&& mEditorAsyncTask->IsDone())
	{
		if (mbEditorRequestCancelAsyncTask)
		{
			if (SNotificationItem* NotificationItem = mEditorNotificationItem.Get())
			{
				NotificationItem->SetCompletionState(SNotificationItem::ECompletionState::CS_Fail);
				NotificationItem->ExpireAndFadeout();
			}
		}
		else
		{
			if (SNotificationItem* NotificationItem = mEditorNotificationItem.Get())
			{
				mEditorAsyncTaskState = FString(TEXT("CreateAIAssist_EnvironmentMapData End"));
				NotificationItem->SetCompletionState(SNotificationItem::ECompletionState::CS_Success);
				NotificationItem->ExpireAndFadeout();
			}
		}

		mEditorAsyncTask->EnsureCompletion();
		delete mEditorAsyncTask;
		mEditorAsyncTask = nullptr;
		Modify(true);
	}
	return true;
}

/**
 * @brief	Voxel情報を非同期で生成開始
 */
void AAIAssist_EnvironmentMap::EditorCreateAIAssist_EnvironmentMapData()
{
	if (GetWorld()->WorldType != EWorldType::Editor)
	{
		return;
	}

	SetupParam();

	RunEditorTick(true);
	if (mEditorAsyncTask == nullptr)
	{
		mEditorAsyncFunc = [this] {this->EditorCreateVoxelList(); };
		mEditorAsyncTask = new FAsyncTask<FAIAssist_EnvironmentMapAsyncTask>(mEditorAsyncFunc);
		mEditorAsyncTask->StartBackgroundTask();
		mbEditorRequestCancelAsyncTask = false;

		if (SNotificationItem* Notification = mEditorNotificationItem.Get())
		{
			Notification->ExpireAndFadeout();
			mEditorNotificationItem.Reset();
		}
		FNotificationInfo NotificationInfo(FText::FromString(FString(TEXT("CreateAIAssist_EnvironmentMapData"))));
		NotificationInfo.bFireAndForget = false;
		mEditorAsyncTaskState = FString(TEXT("CreateAIAssist_EnvironmentMapData Begin"));

		FNotificationButtonInfo ButtonInfo(
			FText::FromString(TEXT("Cancel")),
			FText(),
			FSimpleDelegate::CreateUObject(this, &AAIAssist_EnvironmentMap::EditorCancelCreateMapData)
		);
		NotificationInfo.ButtonDetails.Add(ButtonInfo);

		mEditorNotificationItem = FSlateNotificationManager::Get().AddNotification(NotificationInfo);
		if (SNotificationItem* NotificationItem = mEditorNotificationItem.Get())
		{
			NotificationItem->SetCompletionState(SNotificationItem::ECompletionState::CS_Pending);

			TAttribute<FText>::FGetter TextGetter;
			TextGetter.BindLambda([&]()
				{
					return FText::FromString(mEditorAsyncTaskState);
				});
			TAttribute<FText> TextAttribute = TAttribute<FText>::Create(TextGetter);
			NotificationItem->SetText(TextAttribute);
		}
	}
}

/**
 * @brief	非同期のVoxel生成をキャンセル
 */
void AAIAssist_EnvironmentMap::EditorCancelCreateMapData()
{
	mbEditorRequestCancelAsyncTask = true;
}

/**
 * @brief	Voxel情報を生成
 */
void AAIAssist_EnvironmentMap::EditorCreateVoxelList()
{
	mActiveVoxelList.Empty();
	mEditorCreateVoxelCheckCollisionLog.Empty();
	const int32 MaxVoxelNum = mVoxelXNum * mVoxelYNum * mVoxelZNum;
	int32 CheckCount = 0;
	for (int32 x = 0; x < mVoxelXNum; ++x)
	{
		for (int32 y = 0; y < mVoxelYNum; ++y)
		{
			for (int32 z = 0; z < mVoxelZNum; ++z)
			{
				mEditorAsyncTaskState = FString::Printf(TEXT("AIAssist_EnvironmentMap CreateVoxel %d/%d"), CheckCount, MaxVoxelNum);
				EditorCreateVoxel(x, y, z);
				++CheckCount;
			}
		}
	}

	//生成終わったので、処理しやすくするためにMap用意
	SetupVoxelMap();
}

/**
 * @brief	コリジョンに触れてたらVoxel生成
 */
void AAIAssist_EnvironmentMap::EditorCreateVoxel(const int32 InX, const int32 InY, const int32 InZ)
{
	const int32 BaseIndex = CalcVoxelUID(InX, InY, InZ);
	const bool bCollectCollisionLog = ( mDebugSelectVoxelIndex == BaseIndex);
#if WITH_EDITOR
	if (mbEditorCreateOnlySelectVoxel
		&& !bCollectCollisionLog)
	{
		return;
	}
#endif

	const FVector BaseCenterPos = CalcVoxelCenterPos(InX, InY, InZ);
	const float HalfVoxelLength = mVoxelLength * 0.5f;
	const float QuarterVoxelLength = mVoxelLength * 0.25f;

	//Voxelの上から下に地面チェック
	bool bHitGround = false;
	float HighGroundZ = FLT_MIN;
	{
		TArray<FVector> CheckPosList;
		//CheckPosList.Add(BaseCenterPos + FVector(0.f, 0.f, QuarterVoxelLength));
		CheckPosList.Add(BaseCenterPos + FVector(QuarterVoxelLength, QuarterVoxelLength, HalfVoxelLength));
		CheckPosList.Add(BaseCenterPos + FVector(QuarterVoxelLength, -QuarterVoxelLength, HalfVoxelLength));
		CheckPosList.Add(BaseCenterPos + FVector(-QuarterVoxelLength, QuarterVoxelLength, HalfVoxelLength));
		CheckPosList.Add(BaseCenterPos + FVector(-QuarterVoxelLength, -QuarterVoxelLength, HalfVoxelLength));
		for (const FVector& CheckPos : CheckPosList)
		{
			const FVector TargetPos = CheckPos - FVector(0.f, 0.f, mVoxelLength);
			FHitResult HitResult;
			GetWorld()->LineTraceSingleByChannel(
				HitResult,
				CheckPos,
				TargetPos,
				ECollisionChannel::ECC_WorldStatic
			);

// 			if (bCollectCollisionLog)
// 			{
// 				mEditorCreateVoxelCheckCollisionLog.Add(HitResult);
// 			}

			if (HitResult.bBlockingHit)
			{
				bHitGround = true;
				if (HitResult.ImpactPoint.Z > HighGroundZ)
				{
					HighGroundZ = HitResult.ImpactPoint.Z;
				}
			}
		}
	}

	//周囲のVoxelへの遮蔽チェック
	int32 AroundVoxelCurtainBit = 0;
	for (int32 OffsetIndexX = -1; OffsetIndexX <= 1; ++OffsetIndexX)
	{
		for (int32 OffsetIndexY = -1; OffsetIndexY <= 1; ++OffsetIndexY)
		{
			for (int32 OffsetIndexZ = -1; OffsetIndexZ <= 1; ++OffsetIndexZ)
			{
				bool bCurtain = false;
				const int32 AroundVoxelIndexX = InX + OffsetIndexX;
				const int32 AroundVoxelIndexY = InY + OffsetIndexY;
				const int32 AroundVoxelIndexZ = InZ + OffsetIndexZ;
				const int32 AroundVoxelIndex = CalcVoxelUID(AroundVoxelIndexX, AroundVoxelIndexY, AroundVoxelIndexZ);
				if(AroundVoxelIndex != INDEX_NONE
					&& AroundVoxelIndex != BaseIndex
					//&& OffsetIndexX == 0 && OffsetIndexY == 1 && OffsetIndexZ == 0
					)
				{
					const FVector AroundVoxelCenterPos = CalcVoxelCenterPos(AroundVoxelIndexX, AroundVoxelIndexY, AroundVoxelIndexZ);
					bCurtain = EditorIsCuartain(BaseCenterPos, AroundVoxelCenterPos);
				}

				if (bCurtain)
				{
					const int32 AroundVoxelLinkIndex = CalcAroundVoxelLinkIndex(OffsetIndexX+1, OffsetIndexY+1, OffsetIndexZ+1);
					AroundVoxelCurtainBit = AroundVoxelCurtainBit | 1 << AroundVoxelLinkIndex;
				}
			}
		}
	}

	if (bHitGround
		|| AroundVoxelCurtainBit != 0
		)
	{
		FAIAssist_EnvironmentMapVoxel Voxel;
		Voxel.mVoxelIndex = CalcVoxelUID(InX, InY, InZ);
		Voxel.mAroundVoxelCurtainBit = AroundVoxelCurtainBit;
		Voxel.mbTouchGround = bHitGround;
		if(bHitGround)
		{
			const float MinZ = BaseCenterPos.Z - mVoxelLength * 0.5f;
			const float MaxZ = BaseCenterPos.Z + mVoxelLength * 0.5f;
			const float HightGroundZRatio = FMath::Clamp((HighGroundZ - MinZ) / (MaxZ - MinZ), 0.f, 1.f);
			FAIAssist_CompressOneFloat CompressOneFloat;
			CompressOneFloat.SetData(HightGroundZRatio);
			Voxel.mGroundHeightRatio_Compress = CompressOneFloat.GetData();
		}
		mActiveVoxelList.Add(Voxel);
	}
}

bool AAIAssist_EnvironmentMap::EditorIsCuartain(const FVector& InBaseVoxelPos, const FVector& InTargetVoxelPos) const
{
	const FVector TargetNV = FVector(InTargetVoxelPos-InBaseVoxelPos).GetSafeNormal();
	FVector TargetSideNV = FVector::ForwardVector;
	FVector TargetUpNV = FVector::RightVector;
	if (FMath::Abs(FVector::DotProduct(TargetNV, FVector::UpVector)) < 0.9f)
	{
		TargetSideNV = FVector::CrossProduct(TargetNV, FVector::UpVector);
		TargetUpNV = FVector::CrossProduct(TargetNV, TargetSideNV);
	}

	const float HalfVoxelLength = mVoxelLength * 0.5f;
	const FVector TargetFrontOffsetV = TargetNV * HalfVoxelLength;
	const FVector TargetSideOffsetV = TargetSideNV * HalfVoxelLength;
	const FVector TargetUpOffsetV = TargetUpNV * HalfVoxelLength;
	TArray<FVector> BaseOffsetVList;
	BaseOffsetVList.Add(-TargetFrontOffsetV);
	BaseOffsetVList.Add(-TargetFrontOffsetV + TargetSideOffsetV);
	BaseOffsetVList.Add(-TargetFrontOffsetV - TargetSideOffsetV);
	BaseOffsetVList.Add(-TargetFrontOffsetV + TargetUpOffsetV);
	BaseOffsetVList.Add(-TargetFrontOffsetV - TargetUpOffsetV);

	TArray<FVector> TargetOffsetVList;
	TargetOffsetVList.Add(FVector::ZeroVector);
	TargetOffsetVList.Add(TargetSideOffsetV);
	TargetOffsetVList.Add(-TargetSideOffsetV);
	TargetOffsetVList.Add(TargetUpOffsetV);
	TargetOffsetVList.Add(-TargetUpOffsetV);

	check(BaseOffsetVList.Num() == TargetOffsetVList.Num())
	for (int32 i=0; i<BaseOffsetVList.Num(); ++i)
	{
		const FVector BeginPos = InBaseVoxelPos + BaseOffsetVList[i];
		
		if(i != 0)
		{//開始点が潜ってたら無視
			if(GetWorld()->LineTraceTestByChannel(
				InBaseVoxelPos + BaseOffsetVList[0],
				BeginPos,
				ECollisionChannel::ECC_Visibility)
				)
			{
				continue;
			}
		}

		const FVector EndPos = InTargetVoxelPos + TargetOffsetVList[i];
		FHitResult HitResult;
		GetWorld()->LineTraceSingleByChannel(
			HitResult,
			BeginPos,
			EndPos,
			ECollisionChannel::ECC_Visibility);
		
		//if (bCollectCollisionLog)
		//{
		//	mEditorCreateVoxelCheckCollisionLog.Add(HitResult);
		//}

		if(!HitResult.bBlockingHit)
		{
			return false;
		}
	}

	return true;
}

/**
 * @brief	Box形状の更新
 */
void AAIAssist_EnvironmentMap::EditorUpdateAroundBox()
{
	//Actor位置がBoxの底面の中心になるようにする
	const FVector VoxelBoxExtentV = CalcAroundBoxExtentV();
	mEditorVoxelBox->SetRelativeLocation(FVector(0.f, 0.f, VoxelBoxExtentV.Z));
	mEditorVoxelBox->SetBoxExtent(VoxelBoxExtentV);
}

/**
 * @brief	可視判定テスト
 */
void AAIAssist_EnvironmentMap::EditorTestCheckCurtain(UCanvas* InCanvas)
{
	if (mEditorTestBasePoint == nullptr
		|| mEditorTestTargetPoint == nullptr)
	{
		return;
	}

	const FVector BasePos = mEditorTestBasePoint->GetActorLocation();
	const FVector TargetPos = mEditorTestTargetPoint->GetActorLocation();

	int32 BaseX = 0;
	int32 BaseY = 0;
	int32 BaseZ = 0;
	if (!CalcVoxelIndexXYZ(BaseX, BaseY, BaseZ, BasePos))
	{
		return;
	}
	int32 TargetX = 0;
	int32 TargetY = 0;
	int32 TargetZ = 0;
	if (!CalcVoxelIndexXYZ(TargetX, TargetY, TargetZ, TargetPos))
	{
		return;
	}

	const int32 BaseVoxelIndex = CalcVoxelUID(BaseX, BaseY, BaseZ);
	const int32 TargetVoxelIndex = CalcVoxelUID(TargetX, TargetY, TargetZ);
	FColor LineColor = FColor::Blue;
	if (CheckCurtain(BaseVoxelIndex, TargetVoxelIndex))
	{
		LineColor = FColor::Red;
	}

	const FVector BaseCenterPos = CalcVoxelCenterPos(BaseX, BaseY, BaseZ);
	const FVector TargetCenterPos = CalcVoxelCenterPos(TargetX, TargetY, TargetZ);
	DrawDebugLine(GetWorld(), BaseCenterPos, TargetCenterPos, LineColor);
}

/**
 * @brief	コリジョンチェックとの速度比べ
 */
void AAIAssist_EnvironmentMap::EditorTestPerformance(FCSDebugInfoWindowText& OutInfoWindow) const
{
	if (mEditorTestBasePoint == nullptr
		|| mEditorTestTargetPoint == nullptr)
	{
		return;
	}

	FVector BasePos = mEditorTestBasePoint->GetActorLocation();
	FVector TargetPos = mEditorTestTargetPoint->GetActorLocation();

	double AIAssist_EnvironmentMapTime = FPlatformTime::Seconds();
	for (int32 i = 0; i < 10000; ++i)
	{
		const int32 BaseIndex = CalcVoxelUID(BasePos);
		const int32 TargetIndex = CalcVoxelUID(TargetPos);
		if (CheckCurtain(BaseIndex, TargetIndex))
		{
			int32 test = 0;
			++test;
		}
	}
	AIAssist_EnvironmentMapTime = FPlatformTime::Seconds() - AIAssist_EnvironmentMapTime;
	OutInfoWindow.AddText(FString::Printf(TEXT("Voxel : %.5lf"), AIAssist_EnvironmentMapTime));

	double CollisionTime = FPlatformTime::Seconds();
	for (int32 i = 0; i < 10000; ++i)
	{
		FHitResult HitResult;
		if (GetWorld()->LineTraceSingleByChannel(
			HitResult,
			BasePos,
			TargetPos,
			ECollisionChannel::ECC_WorldStatic
		))
		{
			int32 test = 0;
			++test;
		}
	}
	CollisionTime = FPlatformTime::Seconds() - CollisionTime;
	OutInfoWindow.AddText(FString::Printf(TEXT("Collision : %.5lf"), CollisionTime));
}
#endif// WITH_EDITOR
