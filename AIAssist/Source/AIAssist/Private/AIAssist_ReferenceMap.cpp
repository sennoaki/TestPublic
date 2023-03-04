// AIが地形認知するためのデータ解析＆提供

#include "AIAssist_ReferenceMap.h"

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

AAIAssist_ReferenceMap::AAIAssist_ReferenceMap(const FObjectInitializer& ObjectInitializer)
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
	EditorUpdateArroundBox();
#endif
}

void AAIAssist_ReferenceMap::PostInitProperties()
{
	Super::PostInitProperties();
}
void AAIAssist_ReferenceMap::PostLoad()
{
	Super::PostLoad();

#if UE_BUILD_DEVELOPMENT
	DeubgRequestDraw(true);
#endif
#if WITH_EDITOR
	EditorUpdateArroundBox();
	SetupParam();
	SetupVoxelMap();
#endif
}
void AAIAssist_ReferenceMap::BeginPlay()
{
	Super::BeginPlay();

	SetupParam();
	SetupVoxelMap();

#if USE_CSDEBUG
	UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	UCSDebugSubsystem* CSDebugSubsystem = GameInstance->GetSubsystem<UCSDebugSubsystem>();
	UCSDebugMenuManager* CSDebugMenu = CSDebugSubsystem->GetDebugMenuManager();
	CSDebugMenu->AddNodePropertyBool(TEXT("AIAssist_ReferenceMap/DispInfo"), mbDebugDispInfo);
	CSDebugMenu->AddNodePropertyBool(TEXT("AIAssist_ReferenceMap/DispArroundBox"), mbDebugDispArroundBox);
	CSDebugMenu->AddNodePropertyBool(TEXT("AIAssist_ReferenceMap/DispVoxelList"), mbDebugDispVoxelList);
	CSDebugMenu->AddNodePropertyBool(TEXT("AIAssist_ReferenceMap/DispPlayerVisibleVoxel"), mbDebugDispPlayerVisibleVoxel);
#endif
}

void AAIAssist_ReferenceMap::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

#if UE_BUILD_DEVELOPMENT
	DeubgRequestDraw(false);
#endif
}

void AAIAssist_ReferenceMap::BeginDestroy()
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

void AAIAssist_ReferenceMap::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

/**
 * @brief	ローカル保持のパラメータ更新
 */
void AAIAssist_ReferenceMap::SetupParam()
{
	mArroundBoxCenterPos = CalcArroundBoxCenterPos();
	mArroundBoxExtentV = CalcArroundBoxExtentV();
	mArroundBoxBeginPos = CalcArroundBoxBeginPos();
	mVisbleCheckVoxelNum = 0;
	if(mVoxelLength > 0.f)
	{
		mVisbleCheckVoxelNum = mCheckVisibleDistance/mVoxelLength;
		mVisbleCheckVoxelNum += 1;
	}
}

/**
 * @brief	mActiveVoxelListを元にmVoxelMapを準備
 */
void AAIAssist_ReferenceMap::SetupVoxelMap()
{
	mVoxelMap.Empty();
	for (FAIAssist_ReferenceMapVoxel& Voxel : mActiveVoxelList)
	{
		mVoxelMap.Add(Voxel.mListIndex, &Voxel);
	}

	for (FAIAssist_ReferenceMapVoxel& Voxel : mActiveVoxelList)
	{
		Voxel.mArroundVoxelVisibleMap.Empty();

		if (Voxel.mbUseVisbleBit)
		{
			TArray<int32> IndexList;
			CollectVisibleCheckIndexList(IndexList, Voxel.mListIndex);
			for (int32 i = 0; i < IndexList.Num(); ++i)
			{
				const bool bVisble = FAIAssist_CompressBitArray::GetBool(Voxel.mArroundVoxelVisibleUtilList, i);
				Voxel.mArroundVoxelVisibleMap.Add(IndexList[i], bVisble);
			}
		}
		else if (Voxel.mbUseVisbleIndexList)
		{
			for(const int32& Index : Voxel.mArroundVoxelVisibleUtilList)
			{
				Voxel.mArroundVoxelVisibleMap.Add(Index, true);
			}
		}
		else if (Voxel.mbUseReverseVisbleIndexList)
		{
			for (const int32& Index : Voxel.mArroundVoxelVisibleUtilList)
			{
				Voxel.mArroundVoxelVisibleMap.Add(Index, true);
			}
		}
	}
}

/**
 * @brief	Voxel全体を覆うBoxのExtentV
 */
FVector AAIAssist_ReferenceMap::CalcArroundBoxExtentV() const
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
FVector AAIAssist_ReferenceMap::CalcArroundBoxCenterPos() const
{
	const FVector ExtentV = CalcArroundBoxExtentV();
	return GetActorLocation() + FVector(0.f,0.f,ExtentV.Z);
}

/**
 * @brief	Voxel全体を覆うBoxの計算基点
 */
FVector AAIAssist_ReferenceMap::CalcArroundBoxBeginPos() const
{
	const FVector ExtentV = CalcArroundBoxExtentV();
	return GetActorLocation() - FVector(ExtentV.X, ExtentV.Y, 0.f);
}

/**
 * @brief	指定Voxelの中心座標
 */
FVector AAIAssist_ReferenceMap::CalcVoxelCenterPos(const int32 InX, const int32 InY, const int32 InZ) const
{
	FVector VoxelPos(
		mVoxelLength * static_cast<float>(InX),
		mVoxelLength * static_cast<float>(InY),
		mVoxelLength * static_cast<float>(InZ)
	);
	const float HalfLength = mVoxelLength * 0.5f;
	VoxelPos += FVector(HalfLength, HalfLength, HalfLength);
	VoxelPos += mArroundBoxBeginPos;
	return VoxelPos;
}

/**
 * @brief	Voxelのx,y,zのIndexから全体Indexを取得
 */
int32 AAIAssist_ReferenceMap::CalcVoxelListIndex(const int32 InX, const int32 InY, const int32 InZ) const
{
	if( InX < 0 || InX >= mVoxelXNum
		|| InY < 0 || InY >= mVoxelYNum
		|| InZ < 0 || InZ >= mVoxelZNum
		)
	{
		return smInvalidIndex;
	}
	return InX + InY * mVoxelXNum + InZ * mVoxelXNum * mVoxelYNum;
}

int32 AAIAssist_ReferenceMap::CalcVoxelListIndex(const FVector& InPos) const
{
	int32 IndexX = 0;
	int32 IndexY = 0;
	int32 IndexZ = 0;
	CalcVoxelXYZIndex(IndexX, IndexY, IndexZ, InPos);
	return CalcVoxelListIndex(IndexX,IndexY,IndexZ);
}

/**
 * @brief	Voxel全体のIndexからx,y,zのIndexを取得
 */
void AAIAssist_ReferenceMap::CalcVoxelXYZIndex(int32& OutX, int32& OutY, int32& OutZ, const int32 InListIndex) const
{
	const int32 MulMaxXY = mVoxelXNum * mVoxelYNum;
	OutZ = InListIndex / MulMaxXY;

	const int32 RestZ = InListIndex % MulMaxXY;
	OutY = RestZ / mVoxelXNum;
	OutX = RestZ % mVoxelXNum;
}
/**
 * @brief	指定座標を管理するVoxelのx,y,zのIndex取得
 */
void AAIAssist_ReferenceMap::CalcVoxelXYZIndex(int32& OutX, int32& OutY, int32& OutZ, const FVector& InPos) const
{
	const FVector TargetV = InPos - mArroundBoxBeginPos;
	const float RcpVoxelLength = 1.f / mVoxelLength;
	OutX = static_cast<int32>(TargetV.X * RcpVoxelLength);
	OutY = static_cast<int32>(TargetV.Y * RcpVoxelLength);
	OutZ = static_cast<int32>(TargetV.Z * RcpVoxelLength);

	if(OutX < 0 || OutX >= mVoxelXNum)
	{
		OutX = smInvalidIndex;
	}
	if (OutY < 0 || OutY >= mVoxelYNum)
	{
		OutY = smInvalidIndex;
	}
	if (OutZ < 0 || OutZ >= mVoxelZNum)
	{
		OutZ = smInvalidIndex;
	}
}

/**
 * @brief	指定のVoxelが存在してるかどうか
 */
bool AAIAssist_ReferenceMap::IsOwnVoxelList(const int32 InX, const int32 InY, const int32 InZ) const
{
	return IsOwnVoxelList(CalcVoxelListIndex(InX,InY,InZ));
}
bool AAIAssist_ReferenceMap::IsOwnVoxelList(const int32 InListIndex) const
{
	if( InListIndex == smInvalidIndex)
	{
		return false;
	}
	for(const FAIAssist_ReferenceMapVoxel& Voxel : mActiveVoxelList)
	{
		if(Voxel.mListIndex == InListIndex)
		{
			return true;
		}
	}
	return false;
}

/**
 * @brief	可視判定対象の周囲のVoxelIndexのリスト取得
 */
void AAIAssist_ReferenceMap::CollectVisibleCheckIndexList(TArray<int32>& OutList, const int32 InIndex) const
{
	int32 BaseX = 0;
	int32 BaseY = 0;
	int32 BaseZ = 0;
	CalcVoxelXYZIndex(BaseX, BaseY, BaseZ, InIndex);

	const int32 CheckBeginXIndex = FMath::Max(BaseX - mVisbleCheckVoxelNum, 0);
	const int32 CheckEndXIndex = FMath::Min(BaseX + mVisbleCheckVoxelNum, mVoxelXNum);
	const int32 CheckBeginYIndex = FMath::Max(BaseY - mVisbleCheckVoxelNum, 0);
	const int32 CheckEndYIndex = FMath::Min(BaseY + mVisbleCheckVoxelNum, mVoxelYNum);
	const int32 CheckBeginZIndex = FMath::Max(BaseZ - mVisbleCheckVoxelNum, 0);
	const int32 CheckEndZIndex = FMath::Min(BaseZ + mVisbleCheckVoxelNum, mVoxelZNum);

	TArray<int32> ArroundIndexList;
	for (int32 x = CheckBeginXIndex; x <= CheckEndXIndex; ++x)
	{
		for (int32 y = CheckBeginYIndex; y <= CheckEndYIndex; ++y)
		{
			for (int32 z = CheckBeginZIndex; z <= CheckEndZIndex; ++z)
			{
				const int32 ListIndex = CalcVoxelListIndex(x, y, z);
				if (ListIndex != smInvalidIndex
					&& ListIndex != InIndex)
				{
					const FAIAssist_ReferenceMapVoxel* const* TargetVoxelPtr = mVoxelMap.Find(ListIndex);
					if (TargetVoxelPtr != nullptr)
					{
						OutList.Add(ListIndex);
					}
				}
			}
		}
	}
}

/**
 * @brief	指定のVoxel間が可視状態かどうか
 */
bool AAIAssist_ReferenceMap::CheckVisibleVoxel(const int32 InBaseIndex, const int32 InTargetIndex) const
{
	const FAIAssist_ReferenceMapVoxel* const* BaseVoxelPtr = mVoxelMap.Find(InBaseIndex);
	if (BaseVoxelPtr == nullptr)
	{
		return false;
	}
	const FAIAssist_ReferenceMapVoxel& BaseVoxel = **BaseVoxelPtr;
	if(BaseVoxel.mbUseVisbleBit)
	{
#if 1
		if(const bool* bVisible = BaseVoxel.mArroundVoxelVisibleMap.Find(InTargetIndex))
		{
			return *bVisible;
		}
		return false;
#else
		TArray<int32> IndexList;
		CollectVisibleCheckIndexList(IndexList, BaseVoxel.mListIndex);
		if(const int32 Index = IndexList.Find(InTargetIndex))
		{
			return !FAIAssist_CompressBitArray::GetBool(BaseVoxel.mArroundVoxelVisibleUtilList, Index);
		}
		return true;
#endif
	}
	else if (BaseVoxel.mbUseVisbleIndexList)
	{
#if 1
		return (BaseVoxel.mArroundVoxelVisibleMap.Find(InTargetIndex) != nullptr);
#else
		return (BaseVoxel.mArroundVoxelVisibleUtilList.Find(InTargetIndex) != INDEX_NONE);
#endif
	}
	else if(BaseVoxel.mbUseReverseVisbleIndexList)
	{
#if 1
		return (BaseVoxel.mArroundVoxelVisibleMap.Find(InTargetIndex) == nullptr);
#else
		return (BaseVoxel.mArroundVoxelVisibleUtilList.Find(InTargetIndex) == INDEX_NONE);
#endif
	}
	return true;
}

#if UE_BUILD_DEVELOPMENT

/**
 * @brief	デバッグ対象のVoxelIndexを指定
 */
void AAIAssist_ReferenceMap::DebugSetSelectVoxelIndex(const int32 InIndex)
{
	mDebugSelectVoxelIndex = InIndex;
}
void AAIAssist_ReferenceMap::DebugSetSelectVoxelIndex(const int32 InX, const int32 InY, const int32 InZ)
{
	mDebugSelectVoxelIndex = CalcVoxelListIndex(InX,InY,InZ);
}

/**
 * @brief	Draw登録のon/off
 */
void	AAIAssist_ReferenceMap::DeubgRequestDraw(const bool bInActive)
{
	if (bInActive)
	{
		if (!mDebugDrawHandle.IsValid())
		{
			const auto DebugDrawDelegate = FDebugDrawDelegate::CreateUObject(this, &AAIAssist_ReferenceMap::DebugDraw);
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
void	AAIAssist_ReferenceMap::DebugDraw(UCanvas* InCanvas, APlayerController* InPlayerController)
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
	for(const FEditorCheckVisibleLog& Log : mEditorCheckVisibleLogList)
	{
		FColor Color = FColor::Blue;
		if (Log.mbHit)
		{
			Color = FColor::Red;
		}
		DrawDebugLine(GetWorld(), Log.mBasePos, Log.mTargetPos, Color);
	}
#endif

	if (mbDebugDispInfo)
	{
		DebugDrawInfo(InCanvas);
	}
	if (mbDebugDispArroundBox)
	{
		DebugDrawArroundBox();
	}
	if (mbDebugDispVoxelList)
	{
		DebugDrawVoxelList(InCanvas);
	}
	if (mbDebugDispPlayerVisibleVoxel)
	{
		DebugDrawPlayerVisibleVoxel();
	}

	//DebugTestPerformance();
}

void AAIAssist_ReferenceMap::DebugDrawInfo(UCanvas* InCanvas)
{
	FCSDebugInfoWindowText DebugInfo;
	DebugInfo.SetWindowName(TEXT("AIAssist_ReferenceMap"));
	DebugInfo.AddText(FString::Printf(TEXT("VoxelNum : %d"), mActiveVoxelList.Num()));

	int32 TotalSize = 0;
	int32 TotalMapSize = 0;
	TotalSize += sizeof(*this);
	for (const FAIAssist_ReferenceMapVoxel& Voxel : mActiveVoxelList)
	{
		TotalSize += sizeof(FAIAssist_ReferenceMapVoxel);
		TotalSize += Voxel.mArroundVoxelVisibleUtilList.Num() * sizeof(int32);

		int32 MapSize = Voxel.mArroundVoxelVisibleMap.Num() + sizeof(int32);
		MapSize += Voxel.mArroundVoxelVisibleMap.Num() + sizeof(bool);

		TotalSize += MapSize;
		TotalMapSize += MapSize;
	}
	DebugInfo.AddText(FString::Printf(TEXT("TotalSize : %.1f(KB)"), static_cast<float>(TotalSize) / 1024.f));
	DebugInfo.AddText(FString::Printf(TEXT("TotalMapSize : %.1f(KB)"), static_cast<float>(TotalMapSize) / 1024.f));

	DebugInfo.Draw(InCanvas, FVector2D(50.f,50.f));
}

/**
 * @brief	Voxel全体を覆うBoxをデバッグ表示
 */
void AAIAssist_ReferenceMap::DebugDrawArroundBox()
{
	DrawDebugBox(GetWorld(),
		CalcArroundBoxCenterPos(),
		CalcArroundBoxExtentV(),
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
void AAIAssist_ReferenceMap::DebugDrawVoxelList(UCanvas* InCanvas)
{
	FVector ViewPos = FVector::ZeroVector;
	if(const FSceneView* View = InCanvas->SceneView)
	{
		ViewPos = View->ViewMatrices.GetViewOrigin();
	}
	const float ViewBorderDistanceSq = FMath::Square(5000.f);
	const float HalfLengthShort = mVoxelLength * 0.5f - 1.f;//重なり回避のために少し小さく
	const FVector DispVoxelExtentV(HalfLengthShort, HalfLengthShort, HalfLengthShort);

	for (const FAIAssist_ReferenceMapVoxel& Voxel : mActiveVoxelList)
	{
		const bool bSelectVoxel = (Voxel.mListIndex == mDebugSelectVoxelIndex);
		if (mbDebugDispDetailSelectVoxel
			&& !bSelectVoxel)
		{
			continue;
		}

		int32 IndexX = 0;
		int32 IndexY = 0;
		int32 IndexZ = 0;
		CalcVoxelXYZIndex(IndexX, IndexY, IndexZ, Voxel.mListIndex);
		const FVector VoxelCenterPos = CalcVoxelCenterPos(IndexX, IndexY, IndexZ);
		if (FVector::DistSquared(ViewPos, VoxelCenterPos) > ViewBorderDistanceSq)
		{
			continue;
		}

		DrawDebugBox(GetWorld(), VoxelCenterPos, DispVoxelExtentV, FColor::Green);

#if USE_CSDEBUG
		FCSDebugInfoWindowText DebugInfo;
		DebugInfo.AddText(FString::Printf(TEXT("ListIndex : %d"), Voxel.mListIndex));
		DebugInfo.AddText(FString::Printf(TEXT("X:%d Y:%d Z:%d"), IndexX, IndexY, IndexZ));
		DebugInfo.AddText(FString::Printf(TEXT("VisibleVoxelNum : %d"), Voxel.mArroundVoxelVisibleUtilList.Num()));
		DebugInfo.AddText(FString::Printf(TEXT("GroundHeightRatio : %d"), Voxel.mGroundHeightRatio_Compress));
		DebugInfo.AddText(FString::Printf(TEXT("TouchGround : %d"), Voxel.mbTouchGround));
		DebugInfo.AddText(FString::Printf(TEXT("mbUseVisbleBit : %d"), Voxel.mbUseVisbleBit));
		DebugInfo.AddText(FString::Printf(TEXT("mbUseVisbleIndexList : %d"), Voxel.mbUseVisbleIndexList));
		DebugInfo.AddText(FString::Printf(TEXT("mbUseReverseVisbleIndexList : %d"), Voxel.mbUseReverseVisbleIndexList));
		if (mbDebugDispDetailSelectVoxel)
		{
			//for (const int32& ListIndex : Voxel.mArroundVoxelVisibleBit)
			TArray<int32> IndexList;
			CollectVisibleCheckIndexList(IndexList, Voxel.mListIndex);
			for (int32 i = 0; i < IndexList.Num(); ++i)
			{
				const int32 ListIndex = IndexList[i];
				if (!CheckVisibleVoxel(Voxel.mListIndex, ListIndex))
				{
					continue;
				}
				int32 TargetX = 0;
				int32 TargetY = 0;
				int32 TargetZ = 0;
				CalcVoxelXYZIndex(TargetX, TargetY, TargetZ, ListIndex);
				const FVector TargetPos = CalcVoxelCenterPos(TargetX, TargetY, TargetZ);
				DrawDebugLine(GetWorld(), VoxelCenterPos, TargetPos, FColor::White);
				DrawDebugBox(GetWorld(), TargetPos, DispVoxelExtentV, FColor::Blue);
			}
			FAIAssist_CompressOneFloat CompressOneFloat;
			CompressOneFloat.SetData(Voxel.mGroundHeightRatio_Compress);
			const float GroundHeightRaito = CompressOneFloat.GetValue();
			const float GroundHeightZ = (VoxelCenterPos.Z - mVoxelLength*0.5f) + mVoxelLength*GroundHeightRaito;
			const FVector GroundHeightPos(VoxelCenterPos.X, VoxelCenterPos.Y, GroundHeightZ);
			DrawDebugPoint(GetWorld(), GroundHeightPos, 20.f, FColor::Red);

#if AIASSIST_DEBUGVOXEL
			for (const FVector& GoundPos : Voxel.mEditorGroundPosList)
			{
				DrawDebugPoint(GetWorld(), GoundPos, 20.f, FColor::Green);
			}
#endif
		}
		DebugInfo.Draw(InCanvas, VoxelCenterPos, mVoxelLength * 2.f);
#endif
	}
}

/**
 * @brief	Player位置のVoxelから見えるVoxelをデバッグ表示
 */
void AAIAssist_ReferenceMap::DebugDrawPlayerVisibleVoxel()
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

	int32 XIndex = 0;
	int32 YIndex = 0;
	int32 ZIndex = 0;
	CalcVoxelXYZIndex(XIndex, YIndex, ZIndex, PlayerPos);

	const float HalfLengthShort = mVoxelLength * 0.5f - 1.f;//重なり回避のために少し小さく
	const FVector DispVoxelExtentV(HalfLengthShort, HalfLengthShort, HalfLengthShort);
	const FVector VoxelPos = CalcVoxelCenterPos(XIndex, YIndex, ZIndex);
	DrawDebugBox(GetWorld(), VoxelPos, DispVoxelExtentV, FColor::Green);

	const int32 BaseVoxelListIndex = CalcVoxelListIndex(XIndex,YIndex,ZIndex);
	const auto VoxelMapElement = mVoxelMap.Find(BaseVoxelListIndex);
	if (VoxelMapElement == nullptr)
	{
		return;
	}
	const FAIAssist_ReferenceMapVoxel* BaseVoxel = *VoxelMapElement;
	if (BaseVoxel == nullptr)
	{
		return;
	}
	//for (const int32 VoxelIndex : BaseVoxel->mArroundVoxelVisibleBit)
	TArray<int32> IndexList;
	CollectVisibleCheckIndexList(IndexList, BaseVoxel->mListIndex);
	for (int32 i = 0; i < IndexList.Num(); ++i)
	{
		const int32 ListIndex = IndexList[i];
		if (!CheckVisibleVoxel(BaseVoxel->mListIndex, ListIndex))
		{
			continue;
		}
		int32 TargetXIndex = 0;
		int32 TargetYIndex = 0;
		int32 TargetZIndex = 0;
		CalcVoxelXYZIndex(TargetXIndex, TargetYIndex, TargetZIndex, ListIndex);
		const FVector TargetVoxelPos = CalcVoxelCenterPos(TargetXIndex, TargetYIndex, TargetZIndex);
		DrawDebugBox(GetWorld(), TargetVoxelPos, DispVoxelExtentV, FColor::Blue);
	}
}

/**
 * @brief	コリジョンチェックとの速度比べ(サードパーソンテンプレートで15倍ぐらい速い
 */
void AAIAssist_ReferenceMap::DebugTestPerformance()
{
	FVector BasePos(40.f, -860.f, 160.f);
	FVector TargetPos(40.f, 1700.f, 160.f);
	double AIAssist_ReferenceMapTime = FPlatformTime::Seconds();
	for (int32 i = 0; i < 10000; ++i)
	{
		const int32 BaseIndex = CalcVoxelListIndex(BasePos);
		const int32 TargetIndex = CalcVoxelListIndex(TargetPos);
		if (CheckVisibleVoxel(BaseIndex, TargetIndex))
		{
			int32 test = 0;
			++test;
		}
	}
	AIAssist_ReferenceMapTime = FPlatformTime::Seconds() - AIAssist_ReferenceMapTime;


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
	int32 test = 0;
	++test;
}

#endif//UE_BUILD_DEVELOPMENT

#if WITH_EDITOR

/**
 * @brief	プロパティ変更時
 */
void AAIAssist_ReferenceMap::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	EditorUpdateArroundBox();

	SetupParam();
}

/**
 * @brief	Actorを動かした時
 */
void AAIAssist_ReferenceMap::PostEditMove(bool bFinished)
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
void AAIAssist_ReferenceMap::RunEditorTick(const bool bInRun)
{
	if (bInRun)
	{
		if (!mEditorTickHandle.IsValid())
		{
			mEditorTickHandle = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &AAIAssist_ReferenceMap::EditorTick));
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
bool AAIAssist_ReferenceMap::EditorTick(float InDeltaSecond)
{
	if (mEditorAsyncTask
		&& mEditorAsyncTask->IsDone())
	{
		if(mbEditorRequestCancelAsyncTask)
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
				mEditorAsyncTaskState = FString(TEXT("CreateAIAssist_ReferenceMapData End"));
				NotificationItem->SetCompletionState(SNotificationItem::ECompletionState::CS_Success);
				NotificationItem->ExpireAndFadeout();
			}
		}

		mEditorAsyncTask->EnsureCompletion();
		delete mEditorAsyncTask;
		mEditorAsyncTask = nullptr;
	}
	return true;
}

/**
 * @brief	Voxel情報を非同期で生成開始
 */
void AAIAssist_ReferenceMap::EditorCreateAIAssist_ReferenceMapData()
{
	if (GetWorld()->WorldType != EWorldType::Editor)
	{
		return;
	}

	SetupParam();

	RunEditorTick(true);
	if(mEditorAsyncTask == nullptr)
	{
		mEditorAsyncFunc = [this] {this->EditorCreateVoxelList();};
		mEditorAsyncTask = new FAsyncTask<FAIAssist_ReferenceMapAsyncTask>(mEditorAsyncFunc);
		mEditorAsyncTask->StartBackgroundTask();
		mbEditorRequestCancelAsyncTask = false;

		if (SNotificationItem* Notification = mEditorNotificationItem.Get())
		{
			Notification->ExpireAndFadeout();
			mEditorNotificationItem.Reset();
		}
		FNotificationInfo NotificationInfo(FText::FromString(FString(TEXT("CreateAIAssist_ReferenceMapData"))));
		NotificationInfo.bFireAndForget = false;
		mEditorAsyncTaskState = FString(TEXT("CreateAIAssist_ReferenceMapData Begin"));

		FNotificationButtonInfo ButtonInfo(
			FText::FromString(TEXT("Cancel")),
			FText(),
			FSimpleDelegate::CreateUObject(this, &AAIAssist_ReferenceMap::EditorCancelCreateMapData)
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
void AAIAssist_ReferenceMap::EditorCancelCreateMapData()
{
	mbEditorRequestCancelAsyncTask = true;
}

/**
 * @brief	Voxel情報を生成
 */
void AAIAssist_ReferenceMap::EditorCreateVoxelList()
{
	mActiveVoxelList.Empty();
	const int32 MaxVoxelNum = mVoxelXNum * mVoxelYNum * mVoxelZNum;
	int32 CheckCount = 0;
	for (int32 x = 0; x < mVoxelXNum; ++x)
	{
		for (int32 y = 0; y < mVoxelYNum; ++y)
		{
			for (int32 z = 0; z < mVoxelZNum; ++z)
			{
				mEditorAsyncTaskState = FString::Printf(TEXT("AIAssist_ReferenceMap CreateVoxel %d/%d"), CheckCount, MaxVoxelNum);
				EditorCreateVoxelTouchGround(x,y,z);
				++CheckCount;
			}
		}
	}

	//EditorCreateVoxelGroundUp();

	//生成終わったので、処理しやすくするためにMap用意
	SetupVoxelMap();

	mEditorCheckVisibleLogList.Empty();
	EditorCheckVoxelVisibility();

	//FAIAssist_ReferenceMapVoxel::mArroundVoxelVisibleMapを設定しとく
	SetupVoxelMap();
}

/**
 * @brief	地面に触れてたらVoxel生成
 */
void AAIAssist_ReferenceMap::EditorCreateVoxelTouchGround(const int32 InX, const int32 InY, const int32 InZ)
{
	//Voxelの上から下にコリジョンチェックしてヒットしたら追加
	const FVector BaseCenterPos = CalcVoxelCenterPos(InX, InY, InZ);
	const float HalfVoxelLength = mVoxelLength * 0.5f;
	const float QuarterVoxelLength = mVoxelLength * 0.25f;
	TArray<FVector> CheckPosList;
	//CheckPosList.Add(BaseCenterPos + FVector(0.f, 0.f, QuarterVoxelLength));
	CheckPosList.Add(BaseCenterPos + FVector(QuarterVoxelLength, QuarterVoxelLength, HalfVoxelLength));
	CheckPosList.Add(BaseCenterPos + FVector(QuarterVoxelLength, -QuarterVoxelLength, HalfVoxelLength));
	CheckPosList.Add(BaseCenterPos + FVector(-QuarterVoxelLength, QuarterVoxelLength, HalfVoxelLength));
	CheckPosList.Add(BaseCenterPos + FVector(-QuarterVoxelLength, -QuarterVoxelLength, HalfVoxelLength));
	bool bHitGround = false;
	float HighGroundZ = FLT_MIN;
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
		if (HitResult.bBlockingHit)
		{
			bHitGround = true;
			if (HitResult.ImpactPoint.Z > HighGroundZ)
			{
				HighGroundZ = HitResult.ImpactPoint.Z;
			}
		}
	}

	if (bHitGround)
	{
		FAIAssist_ReferenceMapVoxel Voxel;
		Voxel.mListIndex = CalcVoxelListIndex(InX, InY, InZ);
		Voxel.mbTouchGround = true;
		const float MinZ = BaseCenterPos.Z - mVoxelLength * 0.5f;
		const float MaxZ = BaseCenterPos.Z + mVoxelLength * 0.5f;
		const float HightGroundZRatio = FMath::Clamp((HighGroundZ - MinZ) / (MaxZ - MinZ), 0.f, 1.f);
		FAIAssist_CompressOneFloat CompressOneFloat;
		CompressOneFloat.SetData(HightGroundZRatio);
		Voxel.mGroundHeightRatio_Compress = CompressOneFloat.GetData();
#if AIASSIST_DEBUGVOXEL
		Voxel.mEditorGroundPosList = CheckPosList;
#endif
		mActiveVoxelList.Add(Voxel);
	}
}

/**
 * @brief	接地Voxelの地面から一定の高さまでのVoxelを生成
 */
void AAIAssist_ReferenceMap::EditorCreateVoxelGroundUp()
{
	const int32 GroundVoxelNum = mActiveVoxelList.Num();
	for (int32 i=0; i<GroundVoxelNum; ++i)
	{
		EditorCreateVoxelGroundUp(mActiveVoxelList[i]);
	}
}
void AAIAssist_ReferenceMap::EditorCreateVoxelGroundUp(const FAIAssist_ReferenceMapVoxel& InGroundVoxel)
{
	int32 IndexX = 0;
	int32 IndexY = 0;
	int32 IndexZ = 0;
	CalcVoxelXYZIndex(IndexX, IndexY, IndexZ, InGroundVoxel.mListIndex);
	const FVector VoxelCenterPos = CalcVoxelCenterPos(IndexX, IndexY, IndexZ);

	FAIAssist_CompressOneFloat CompressOneFloat;
	CompressOneFloat.SetData(InGroundVoxel.mGroundHeightRatio_Compress);
	const float GroundHeightRaito = CompressOneFloat.GetValue();
	const float GroundHeightZ = (VoxelCenterPos.Z - mVoxelLength * 0.5f) + mVoxelLength * GroundHeightRaito;
	const float BorderTopZ = GroundHeightZ + mEditorCheckCreateVoxelGroundUpHeight;

	const FVector GroundPos(VoxelCenterPos.X,VoxelCenterPos.Y,GroundHeightZ);

	float CheckZ = GroundHeightZ + mVoxelLength;
	while (CheckZ <= BorderTopZ)
	{
		const FVector CheckPos(VoxelCenterPos.X, VoxelCenterPos.Y, CheckZ);
		int32 CheckXIndex = smInvalidIndex;
		int32 CheckYIndex = smInvalidIndex;
		int32 CheckZIndex = smInvalidIndex;
		CalcVoxelXYZIndex(CheckXIndex,CheckYIndex,CheckZIndex,CheckPos);
		if(IsOwnVoxelList(CheckXIndex,CheckYIndex,CheckZIndex))
		{
			CheckZ += mVoxelLength;
			continue;
		}

		FHitResult HitResult;
		GetWorld()->LineTraceSingleByChannel(
			HitResult,
			GroundPos,
			CheckPos,
			ECollisionChannel::ECC_WorldStatic
		);

		if (!HitResult.bBlockingHit)
		{
			FAIAssist_ReferenceMapVoxel Voxel;
			Voxel.mListIndex = CalcVoxelListIndex(CheckXIndex, CheckYIndex, CheckZIndex);
			mActiveVoxelList.Add(Voxel);
		}
		
		CheckZ += mVoxelLength;
	}
}

/**
 * @brief	Voxel間の可視判定
 */
void AAIAssist_ReferenceMap::EditorCheckVoxelVisibility()
{
	for (int32 i=0; i<mActiveVoxelList.Num(); ++i)
	{
		mEditorAsyncTaskState = FString::Printf(TEXT("AIAssist_ReferenceMap CheckVisibility %d/%d"), i, mActiveVoxelList.Max());
		FAIAssist_ReferenceMapVoxel& Voxel = mActiveVoxelList[i];
		EditorCheckVoxelVisibility(Voxel);

		if (mbEditorRequestCancelAsyncTask)
		{
			mEditorAsyncTaskState = FString(TEXT("CreateAIAssist_ReferenceMapData Cancel"));
			break;
		}
	}
}
void AAIAssist_ReferenceMap::EditorCheckVoxelVisibility(FAIAssist_ReferenceMapVoxel& OutVoxel)
{
	FAIAssist_ReferenceMapVoxel& BaseVoxel = OutVoxel;
	BaseVoxel.mArroundVoxelVisibleUtilList.Empty();
	int32 BaseX = 0;
	int32 BaseY = 0;
	int32 BaseZ = 0;
	CalcVoxelXYZIndex(BaseX, BaseY, BaseZ, BaseVoxel.mListIndex);
	const FVector BaseCenterPos = CalcVoxelCenterPos(BaseX, BaseY, BaseZ);
	const float HalfVoxelLength = mVoxelLength * 0.5f;
	const float QuarterVoxelLength = mVoxelLength * 0.25f;
	
	float BaseMinZ = BaseCenterPos.Z - mVoxelLength * 0.5f;
	if(BaseVoxel.mbTouchGround)
	{
		FAIAssist_CompressOneFloat CompressOneFloat;
		CompressOneFloat.SetData(BaseVoxel.mGroundHeightRatio_Compress);
		const float GroundHeightRaito = CompressOneFloat.GetValue();
		BaseMinZ += mVoxelLength * GroundHeightRaito;
		BaseMinZ += QuarterVoxelLength;
	}

	TArray<FVector> BasePosList;
	BasePosList.Add(BaseCenterPos + FVector(HalfVoxelLength, 0.f, 0.f));
	BasePosList.Add(BaseCenterPos + FVector(-HalfVoxelLength, 0.f, 0.f));
	BasePosList.Add(BaseCenterPos + FVector(0.f, HalfVoxelLength, 0.f));
	BasePosList.Add(BaseCenterPos + FVector(0.f, -HalfVoxelLength, 0.f));
	BasePosList.Add(BaseCenterPos + FVector(0.f, 0.f, HalfVoxelLength));
	BasePosList.Add(BaseCenterPos + FVector(0.f, 0.f, -HalfVoxelLength));
	for (FVector& Pos : BasePosList)
	{
		if (Pos.Z < BaseMinZ)
		{
			Pos.Z = BaseMinZ;
		}
	}

	const int32 CheckBeginXIndex = FMath::Max(BaseX - mVisbleCheckVoxelNum, 0);
	const int32 CheckEndXIndex = FMath::Min(BaseX + mVisbleCheckVoxelNum, mVoxelXNum);
	const int32 CheckBeginYIndex = FMath::Max(BaseY - mVisbleCheckVoxelNum, 0);
	const int32 CheckEndYIndex = FMath::Min(BaseY + mVisbleCheckVoxelNum, mVoxelYNum);
	const int32 CheckBeginZIndex = FMath::Max(BaseZ - mVisbleCheckVoxelNum, 0);
	const int32 CheckEndZIndex = FMath::Min(BaseZ + mVisbleCheckVoxelNum, mVoxelZNum);

	TArray<int32> ArroundIndexList;
	CollectVisibleCheckIndexList(ArroundIndexList, BaseVoxel.mListIndex);

	TArray<int32> VisibleBitList;
	TArray<int32> VisibleVoxelIndexList;
	TArray<int32> NonVisibleVoxelIndexList;
	for (int32 i=0; i<ArroundIndexList.Num(); ++i)
	{
		const int32 TargetIndex = ArroundIndexList[i];
		FAIAssist_ReferenceMapVoxel** TargetVoxelPtr = mVoxelMap.Find(TargetIndex);
		if (TargetVoxelPtr == nullptr)
		{
			continue;
		}
		FAIAssist_ReferenceMapVoxel& TargetVoxel = **TargetVoxelPtr;
		if (BaseVoxel.mListIndex == TargetVoxel.mListIndex)
		{
			continue;
		}
		int32 TargetX = 0;
		int32 TargetY = 0;
		int32 TargetZ = 0;
		CalcVoxelXYZIndex(TargetX, TargetY, TargetZ, TargetVoxel.mListIndex);
		const FVector TargetCenterPos = CalcVoxelCenterPos(TargetX, TargetY, TargetZ);

		float TargetMinZ = TargetCenterPos.Z - mVoxelLength * 0.5f;
		if (TargetVoxel.mbTouchGround)
		{
			FAIAssist_CompressOneFloat CompressOneFloat;
			CompressOneFloat.SetData(TargetVoxel.mGroundHeightRatio_Compress);
			const float GroundHeightRaito = CompressOneFloat.GetValue();
			TargetMinZ += mVoxelLength * GroundHeightRaito;
			TargetMinZ += QuarterVoxelLength;
		}

		TArray<FVector> TargetPosList;
		TargetPosList.Add(TargetCenterPos + FVector(HalfVoxelLength, 0.f, 0.f));
		TargetPosList.Add(TargetCenterPos + FVector(-HalfVoxelLength, 0.f, 0.f));
		TargetPosList.Add(TargetCenterPos + FVector(0.f, HalfVoxelLength, 0.f));
		TargetPosList.Add(TargetCenterPos + FVector(0.f, -HalfVoxelLength, 0.f));
		TargetPosList.Add(TargetCenterPos + FVector(0.f, 0.f, HalfVoxelLength));
		TargetPosList.Add(TargetCenterPos + FVector(0.f, 0.f, -HalfVoxelLength));
		for (FVector& Pos : TargetPosList)
		{
			if (Pos.Z < TargetMinZ)
			{
				Pos.Z = TargetMinZ;
			}
		}

		bool bAllHit = true;
		for (int32 PosIndex = 0; PosIndex < BasePosList.Num(); ++PosIndex)
		{
			FHitResult HitResult;
			GetWorld()->LineTraceSingleByChannel(
				HitResult,
				BasePosList[PosIndex],
				TargetPosList[PosIndex],
				ECollisionChannel::ECC_Visibility
			);

			if (BaseVoxel.mListIndex == mEditorCollectCheckVisibleBaseIndex
				&& TargetVoxel.mListIndex == mEditorCollectCheckVisibleTargetIndex)
			{
				FEditorCheckVisibleLog CheckVisibleLog;
				CheckVisibleLog.mBasePos = BasePosList[PosIndex];
				CheckVisibleLog.mTargetPos = TargetPosList[PosIndex];
				CheckVisibleLog.mbHit = HitResult.bBlockingHit;
				mEditorCheckVisibleLogList.Add(CheckVisibleLog);
			}

			if (!HitResult.bBlockingHit)
			{
				bAllHit = false;
				continue;
			}
		}

		FAIAssist_CompressBitArray::SetBool(VisibleBitList, i, !bAllHit);
		if (bAllHit)
		{
			NonVisibleVoxelIndexList.AddUnique(TargetIndex);
		}
		else
		{
			VisibleVoxelIndexList.AddUnique(TargetIndex);
		}
	}

	const int32 VisibleBitListNum = VisibleBitList.Num();
	const int32 VisibleVoxelIndexListNum = VisibleVoxelIndexList.Num();
	const int32 NonVisibleVoxelIndexListNum = NonVisibleVoxelIndexList.Num();
 	if (VisibleVoxelIndexListNum < VisibleBitListNum
 		|| NonVisibleVoxelIndexListNum < VisibleBitListNum)
	{
		if (VisibleVoxelIndexListNum < NonVisibleVoxelIndexListNum)
		{
			BaseVoxel.mbUseVisbleIndexList = true;
			BaseVoxel.mArroundVoxelVisibleUtilList = VisibleVoxelIndexList;
		}
		else
		{
			BaseVoxel.mbUseReverseVisbleIndexList = true;
			BaseVoxel.mArroundVoxelVisibleUtilList = NonVisibleVoxelIndexList;
		}
	}
	else
	{
		BaseVoxel.mbUseVisbleBit = true;
		BaseVoxel.mArroundVoxelVisibleUtilList = VisibleBitList;
	}
}

/**
 * @brief	Box形状の更新
 */
void AAIAssist_ReferenceMap::EditorUpdateArroundBox()
{
	//Actor位置がBoxの底面の中心になるようにする
	const FVector VoxelBoxExtentV = CalcArroundBoxExtentV();
	mEditorVoxelBox->SetRelativeLocation(FVector(0.f,0.f,VoxelBoxExtentV.Z));
	mEditorVoxelBox->SetBoxExtent(VoxelBoxExtentV);
}
#endif// WITH_EDITOR
