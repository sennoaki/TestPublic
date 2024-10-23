// Octree

#include "AIAssist_Octree.h"

#include "Components/BoxComponent.h"

#include "Engine/Canvas.h"
#include "Debug/DebugDrawService.h"
#include "DrawDebugHelpers.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "GameFramework/PlayerController.h"
#include "AIAssist_Compress.h"
#include "Misc/Attribute.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/TargetPoint.h"

#if USE_CSDEBUG
#include "CSDebugSubsystem.h"
#include "CSDebugUtility.h"
#include "DebugMenu/CSDebugMenuManager.h"
#include "DebugMenu/CSDebugMenuNodeGetter.h"
#include "DebugInfoWindow/CSDebugInfoWindowText.h"
#endif


AAIAssist_Octree::AAIAssist_Octree(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	//PrimaryActorTick.bCanEverTick = true;

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

/**
 * @brief	指定ノードの取得
 */
const FAIAssist_OctreeNode* AAIAssist_Octree::GetNode(const int32 InX, const int InY, const int InZ, const int32 InTargetDepth) const
{
	const FAIAssist_OctreeNode* CurrentNode = mRoot;
	for (int32 Depth = 0; Depth < InTargetDepth; ++Depth)
	{
		const int32 ChildIndex = GetChildIndex(InX, InY, InZ, Depth);
		CurrentNode = CurrentNode->GetChild(ChildIndex);  // 次の深さへ進む
		if (CurrentNode == nullptr)
		{
			return nullptr;
		}
	}
	return CurrentNode;
}

/**
 * @brief	線分と交差してるNode取得
 */
const FAIAssist_OctreeNode* AAIAssist_Octree::Intersect(const FVector& InBasePos, const FVector& InTargetPos) const
{
	const float CheckDistance = FVector::Distance(InBasePos, InTargetPos);
	if (CheckDistance <= FLT_MIN)
	{
		return nullptr;
	}

	FRay CheckRay;
	CheckRay.mOrigin = mEditorTestRayCastBasePos;
	CheckRay.mDirection = FVector(InTargetPos - InBasePos).GetSafeNormal();
	return RayCastIterative(CheckRay, 0.f, CheckDistance);
}

void AAIAssist_Octree::PostInitProperties()
{
	Super::PostInitProperties();
}
void AAIAssist_Octree::PostLoad()
{
	Super::PostLoad();

#if USE_CSDEBUG
	if(GetClass()->GetDefaultObject() != this)
	{
		DeubgRequestDraw(true);
	}
#endif
#if WITH_EDITOR
	EditorUpdateAroundBox();
#endif
}
void AAIAssist_Octree::BeginPlay()
{
	Super::BeginPlay();

	BuildOctree();
}

void AAIAssist_Octree::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

#if USE_CSDEBUG
	DeubgRequestDraw(false);
#endif
}

void AAIAssist_Octree::BeginDestroy()
{
#if USE_CSDEBUG
	DeubgRequestDraw(false);
#endif
	Super::BeginDestroy();

	if (mRoot == nullptr)
	{
		delete mRoot;
	}
}

/**
 * @brief	OctreeDataを元にOctree構築
 */
void AAIAssist_Octree::BuildOctree()
{
	if (mRoot)
	{
		delete mRoot;
		mRoot = nullptr;
	}
	mRoot = new FAIAssist_OctreeNode();

	const int32 DataListNum = GetDataListNum();
	for (int32 i = 0; i < DataListNum; ++i)
	{
		const FAIAssist_OctreeData* OctreeData = GetData(i);
		if (OctreeData == nullptr)
		{
			continue;
		}

		int32 X = 0;
		int32 Y = 0;
		int32 Z = 0;
		CalcNodeIndex(X, Y, Z, mMaxDepth, OctreeData->mNodeIndex);
		if (FAIAssist_OctreeNode* OctreeNode = Insert(X, Y, Z))
		{
			OctreeNode->mDataListIndex = i;
		}
	}
}

/**
 * @brief	新規ノード挿入
 */
FAIAssist_OctreeNode* AAIAssist_Octree::Insert(const int32 InX, const int32 InY, const int32 InZ)
{
	FAIAssist_OctreeNode* CurrentNode = mRoot;
	for (int32 Depth = 0; Depth < mMaxDepth; ++Depth)
	{
		int32 ChildIndex = GetChildIndex(InX, InY, InZ, Depth);
		CurrentNode->CreateChild(ChildIndex);  // 子ノードが存在しない場合は作成
		CurrentNode = CurrentNode->GetChild(ChildIndex);  // 次の深さへ進む
	}
	return CurrentNode;
}

/**
 * @brief	指定ノードのChildListのIndex取得
 */
int32 AAIAssist_Octree::GetChildIndex(const int32 InX, const int32 InY, const int32 InZ, const int32 InDepth) const
{
	const int32 Shift = mMaxDepth - InDepth- 1;
	return ((InX >> Shift) & 1) << 2 | ((InY >> Shift) & 1) << 1 | ((InZ >> Shift) & 1);
}

/**
 * @brief	指定ノードの中心座標取得
 */
FVector AAIAssist_Octree::CalcNodeCenter(const int32 InX, const int32 InY, const int32 InZ, const int32 InDepth) const
{
	// 初期の中心点とサイズ
	float Size = mRootLength*0.5f;  // ルートノードの半分のサイズ
	FVector CenterPos = GetActorLocation();  // 中心点

	for (int32 Depth = 0; Depth < InDepth; ++Depth)
	{
		int32 Shift = mMaxDepth - Depth - 1;
		// 各軸ごとに位置を決定し、中心点を調整
		CenterPos.X += ((InX >> Shift) & 1) ? Size : -Size;
		CenterPos.Y += ((InY >> Shift) & 1) ? Size : -Size;
		CenterPos.Z += ((InZ >> Shift) & 1) ? Size : -Size;
		Size *= 0.5f;  // 次の深さに応じてサイズを半分にする
	}

	return CenterPos;
}

/**
 * @brief	指定座標のノードIndex取得
 */
uint32 AAIAssist_Octree::CalcNodeIndex(const FVector& InPos, const int32 InDepth) const
{
	const float ExtentLength = CalcVoxelExtentLength(InDepth) * 2.f;
	const FVector BasePos = GetActorLocation() - FVector(mRootLength,mRootLength,mRootLength);
	const int32 IndexX = static_cast<int32>((InPos.X - BasePos.X) / ExtentLength);
	const int32 IndexY = static_cast<int32>((InPos.Y - BasePos.Y) / ExtentLength);
	const int32 IndexZ = static_cast<int32>((InPos.Z - BasePos.Z) / ExtentLength);

	return CalcNodeIndex(IndexX, IndexY, IndexZ, InDepth);

#if 0
	uint32 Index = 0;

	// オクツリーのルートサイズから開始し、深さに応じて細分化
	float Size = mRootLength * 0.5f;
	float X = InPos.X;
	float Y = InPos.Y;
	float Z = InPos.Z;

	for (int32 i = 0; i < InDepth; ++i)
	{
		int32 Octant = 0;

		if (X >= Size)
		{
			Octant |= 4;  // xビット
			X -= Size;
		}
		if (Y >= Size)
		{
			Octant |= 2;  // yビット
			Y -= Size;
		}
		if (Z >= Size)
		{
			Octant |= 1;  // zビット
			Z -= Size;
		}

		// インデックスを左に3ビットシフトしてオクタントを追加
		Index = (Index << 3) | Octant;

		// 次の深さのためにサイズを半分に
		Size *= 0.5f;
	}

	return Index;
#endif
}
AAIAssist_Octree::tNodeIndex AAIAssist_Octree::CalcNodeIndex(const int32 InX, const int32 InY, const int32 InZ, const int32 InDepth) const
{
	uint32 NodeIndex = 0;
	for (int32 i = 0; i < InDepth; ++i)
	{
		const int32 Shift = InDepth - i - 1;
		const int32 Bit = (1 << Shift);
		NodeIndex |= ((InX & Bit) >> Shift) << (3 * i + 2);
		NodeIndex |= ((InY & Bit) >> Shift) << (3 * i + 1);
		NodeIndex |= ((InZ & Bit) >> Shift) << (3 * i);
	}
	return NodeIndex;
}
bool AAIAssist_Octree::CalcNodeIndex(int32& OutX, int32& OutY, int32& OutZ, const int32 InDepth, const tNodeIndex InIndex) const
{
	OutX = 0;
	OutY = 0;
	OutZ = 0;

	// 深さに基づいてインデックスから座標を取り出す
	for (int32 i = 0; i < InDepth; ++i)
	{
		int32 shift = 3 * i;

		// 各軸のビットを取り出し、適切な位置に配置
		OutX |= ((InIndex >> (shift + 2)) & 1) << (InDepth - i - 1);
		OutY |= ((InIndex >> (shift + 1)) & 1) << (InDepth - i - 1);
		OutZ |= ((InIndex >> shift) & 1) << (InDepth - i - 1);
	}
	return true;
}

/**
 * @brief	指定Indexのノード取得
 */
const FAIAssist_OctreeNode* AAIAssist_Octree::GetNodeFromIndex(const tNodeIndex InNodendex, const int32 InDepth) const
{
	const FAIAssist_OctreeNode* Node = mRoot;
	for (int32 i = 0; i < InDepth; ++i)
	{
		int32 Shift = (InDepth - 1 - i) * 3;
		int32 Octant = (InNodendex >> Shift) & 0x7;  // 3ビットを取り出す

		if (!Node->mChildNodeList[Octant])
		{
			return nullptr;  // ノードが存在しない場合
		}
		Node = Node->mChildNodeList[Octant];
	}
	return Node;
}

/**
 * @brief	指定深度のVoxelのExtent取得
 */
float AAIAssist_Octree::CalcVoxelExtentLength(const int32 InDepth) const
{
	float ExtentLength = mRootLength;
	for (int32 i = 0; i < InDepth; ++i)
	{
		ExtentLength *= 0.5f;
	}
	return ExtentLength;
}

/**
 * @brief	指定深度のVoxelの一辺の数
 */
int32 AAIAssist_Octree::CalcVoxelEdgeNum(const int32 InDepth) const
{
	int32 VoxelNum = 1;
	for (int32 i = 0; i < InDepth; ++i)
	{
		VoxelNum *= 2;
	}
	return VoxelNum;
}

/**
 * @brief	指定Indexのノード取得
 */
const FAIAssist_OctreeNode* AAIAssist_Octree::RayCastIterative(const FRay& InRay, float InMin, float InMax) const
{
	struct FStackElement
	{
		FAIAssist_OctreeNode* mNode = nullptr;
		FVector mCenterPos = FVector::ZeroVector;
		float mSize = 0.f;
		int32 mDepth = 0;
	};

	// 探索するノードを保持するスタック
	TArray<FStackElement> StackList;

	// ルートノードから探索開始
	StackList.Push({ mRoot, GetActorLocation(), mRootLength, 0 });

	while (StackList.Num() > 0)
	{
		FStackElement Element = StackList.Top();
		StackList.Pop();

		const FAIAssist_OctreeNode* CheckNode = Element.mNode;
		const FVector CheckCenterPos = Element.mCenterPos;
		const float CheckVoxelSize = Element.mSize;
		const int32 Depth = Element.mDepth;

		// ノードが空であればスキップ
		if (CheckNode == nullptr
			|| CheckNode->mChildNodeList.Num() == 0)
		{
			continue;
		}

		// レイがノードのAABBと交差していなければスキップ
		if (!RayIntersectsAABB(InRay, CheckCenterPos, CheckVoxelSize, InMin, InMax))
		{
			continue;
		}

		// 目的の深さに到達したらヒット
		if (Depth == mMaxDepth)
		{
			return CheckNode;  // レイがこのボクセルにヒット
		}

		// 子ノードをスタックに追加して探索を続ける
		float CheckVoxelHalfSize = CheckVoxelSize * 0.5f;
		for (int32 i = 0; i < 8; ++i)
		{
			FVector ChildVoxelCenterPos = {
				CheckCenterPos.X + (i & 4 ? CheckVoxelHalfSize : -CheckVoxelHalfSize),
				CheckCenterPos.Y + (i & 2 ? CheckVoxelHalfSize : -CheckVoxelHalfSize),
				CheckCenterPos.Z + (i & 1 ? CheckVoxelHalfSize : -CheckVoxelHalfSize)
			};
			if(FAIAssist_OctreeNode* ChildNode = CheckNode->mChildNodeList[i])
			{
				if(ChildNode->mChildNodeList.Num() > 0)
				{
					StackList.Push({ ChildNode, ChildVoxelCenterPos, CheckVoxelHalfSize, Depth + 1 });
				}
			}
		}
	}
	return nullptr;  // どのノードにもヒットしなかった
}

/**
 * @brief	レイとAABBの交差判定
 */
bool AAIAssist_Octree::RayIntersectsAABB(const FRay& InRay, const FVector& InVoxelCenterPos, const float InVoxelSize, float InOutMin, float InOutMax) const
{
	// 各軸について、レイがAABBと交差するかを確認
	for (int32 i = 0; i < 3; ++i)
	{
		float InvDirection = 1.0f / InRay.mDirection[i];
		float t0 = (InVoxelCenterPos[i] - InVoxelSize - InRay.mOrigin[i]) * InvDirection;
		float t1 = (InVoxelCenterPos[i] + InVoxelSize - InRay.mOrigin[i]) * InvDirection;
		if (InvDirection < 0.0f)
		{
			float TempValue = t0;
			t0 = t1;
			t1 = TempValue;
		}
		InOutMin = t0 > InOutMin ? t0 : InOutMin;
		InOutMax = t1 < InOutMax ? t1 : InOutMax;
		if (InOutMax <= InOutMin)
		{
			return false;
		}
	}
	return true;
}

#if USE_CSDEBUG
/**
 * @brief	Draw登録のon/off
 */
void	AAIAssist_Octree::DeubgRequestDraw(const bool bInActive)
{
	if (bInActive)
	{
		if (!mDebugDrawHandle.IsValid())
		{
			const auto DebugDrawDelegate = FDebugDrawDelegate::CreateUObject(this, &AAIAssist_Octree::DebugDraw);
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
void	AAIAssist_Octree::DebugDraw(UCanvas* InCanvas, APlayerController* InPlayerController)
{
	if (mbDebugDrawInfo)
	{
		DebugDrawInfo(InCanvas);
	}
	if (mbDebugDrawAroundBox)
	{
		DebugDrawAroundBox();
	}
	if (mbDebugDrawActiveVoxel)
	{
		DebugDrawActiveVoxel(InCanvas, mMaxDepth);
	}

#if WITH_EDITOR
	EditorDebugDraw(InCanvas);
	if (mbEditorTestRayCast)
	{
		EditorDrawTestRayCast();
	}
#endif
}

void AAIAssist_Octree::DebugDrawInfo(UCanvas* InCanvas)
{
	FCSDebugInfoWindowText DebugInfo;
	DebugInfo.SetWindowName(TEXT("AIAssist_Octree"));
	DebugInfo.AddText(FString::Printf(TEXT("NodeNum : %d"), mNodeList.Num()));

	int32 TotalSize = 0;
	TotalSize += sizeof(*this);
	
	const int32 NodeSize = sizeof(FAIAssist_OctreeNode);
	const int32 NodeListSize = mNodeList.Num() * NodeSize;
	TotalSize += NodeListSize;
	
	DebugInfo.AddText(FString::Printf(TEXT("TotalSize : %.1f(KB)"), static_cast<float>(TotalSize) / 1024.f));
	DebugInfo.AddText(FString::Printf(TEXT("--NodeSize : %d(B)"), NodeSize));
	DebugInfo.AddText(FString::Printf(TEXT("--NodeListSize : %.1f(KB)"), static_cast<float>(NodeListSize) / 1024.f));

	DebugInfo.Draw(InCanvas, FVector2D(50.f, 50.f));
}

/**
 * @brief	Voxel全体を覆うBoxをデバッグ表示
 */
void AAIAssist_Octree::DebugDrawAroundBox()
{
	DrawDebugBox(GetWorld(),
		GetActorLocation(),
		mAroundBoxExtentV,
		FColor::Red,
		false,
		-1.f,
		0,
		10.f
	);
}

void AAIAssist_Octree::DebugDrawActiveVoxel(UCanvas* InCanvas, const int32 InDepth) const
{
	if (mRoot == nullptr)
	{
		return;
	}
	FVector BasePos = FVector::ZeroVector;
	if(InCanvas != nullptr
		&& InCanvas->SceneView != nullptr)
	{
		BasePos = InCanvas->SceneView->ViewMatrices.GetViewOrigin();
	}
	const float DrawDistanceSq = FMath::Square(1000.f);

	const int32 MaxIndex = 1 << mMaxDepth;// 2^InDepth

	const FVector DrawOffsetInsideExtentV(1.f, 1.f, 1.f);
	FColor Color = FColor::Red;
	float ExtentLength = mRootLength;
	for (int32 i = 0; i < InDepth; ++i)
	{
		ExtentLength *= 0.5f;
	}
	const FVector ExtentV = FVector(ExtentLength, ExtentLength, ExtentLength) - DrawOffsetInsideExtentV;

	for (int32 x = 0; x < MaxIndex; ++x)
	{
		for (int32 y = 0; y < MaxIndex; ++y)
		{
			for (int32 z = 0; z < MaxIndex; ++z)
			{
				if (const FAIAssist_OctreeNode* Node = GetNode(x, y, z, InDepth))
				{
					// ノードの中心点を計算
					const FVector Pos = CalcNodeCenter(x, y, z, InDepth);
					if (FVector::DistSquared(Pos, BasePos) > DrawDistanceSq)
					{
						continue;
					}
					DrawDebugBox(GetWorld(), Pos, ExtentV, Color, false, -1.f, 0, 2.f);
				}
			}
		}
	}
}

void AAIAssist_Octree::DebugDrawVoxelDetail(UCanvas* InCanvas, const int32 InX, const int32 InY, const int32 InZ) const
{
	TArray<FColor> ColorList;
	ColorList.Add(FColor::Blue);
	ColorList.Add(FColor::Yellow);
	ColorList.Add(FColor::Green);
	ColorList.Add(FColor::Orange);
	const int32 ColorListSize = ColorList.Num();

	for(int32 i=1; i<=mMaxDepth; ++i)
	{
		const FColor Color = ColorList[(i-1)%ColorListSize];
		const float Extent = CalcVoxelExtentLength(i);
		const FVector Pos = CalcNodeCenter(InX, InY, InZ, i);
		DrawDebugBox(GetWorld(), Pos, FVector(Extent, Extent, Extent), Color, false, -1.f, 0, 2.f);

		FCSDebugInfoWindowText WindowText;
		WindowText.SetWindowFrameColor(Color);
		WindowText.SetWindowName(FString::Printf(TEXT("Depth[%d]"), i));
		WindowText.AddText(FString::Printf(TEXT("%d,%d,%d"), InX, InY, InZ));
		const int32 Shift = mMaxDepth - i - 1;
		WindowText.AddText(FString::Printf(TEXT("Bit : %d,%d,%d"), (InX >> Shift) & 1, (InY >> Shift) & 1, (InZ >> Shift) & 1));
		WindowText.AddText(FString::Printf(TEXT("Pos : %s"), *Pos.ToString()));
		WindowText.Draw(InCanvas, Pos, 1000.f);
	}
}

#endif//USE_CSDEBUG

#if WITH_EDITOR

void AAIAssist_Octree::EditorTestCreate()
{
	EditorCreateOctreeData();

	BuildOctree();

// 	if(mRoot)
// 	{
// 		delete mRoot;
// 		mRoot = nullptr;
// 	}
// 
// 	mRoot = new FAIAssist_OctreeNode();

	//Insert(2, 3, 5);
// 	Insert(0, 0, 0);
// 	Insert(0, 0, 15);
// 	Insert(0, 15, 15);
// 	Insert(0, 15, 0);
// 	Insert(15, 0, 0);
// 	Insert(15, 0, 15);
// 	Insert(15, 15, 15);
//	FAIAssist_OctreeNode* OctreeNode = Insert(15, 15, 0);

// 	tNodeIndex NodeIndex = CalcNodeIndex(FVector(490.f, 490.f, -490.f), 4);
// 	NodeIndex = CalcNodeIndex(15, 0, 15, 4);
// 	int32 X = 0;
// 	int32 Y = 0;
// 	int32 Z = 0;
// 	CalcNodeIndex(X, Y, Z, 4, NodeIndex);
// 	const FAIAssist_OctreeNode* NodeFromIndex = GetNodeFromIndex(NodeIndex, 4);
// 	const FAIAssist_OctreeNode* NodeFromXYZ = GetNode(15, 15, 0, 4);
// 	int32 test = 0;
// 	++test;
}

/**
 * @brief	プロパティ変更時
 */
void AAIAssist_Octree::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	EditorUpdateAroundBox();
}

/**
 * @brief	Actorを動かした時
 */
void AAIAssist_Octree::PostEditMove(bool bFinished)
{
	if (bFinished)
	{//回転とスケールは編集禁止
		SetActorRotation(FRotator::ZeroRotator.Quaternion());
		SetActorScale3D(FVector::OneVector);
	}
	Super::PostEditMove(bFinished);
}
/**
 * @brief	Box形状の更新
 */
void AAIAssist_Octree::EditorUpdateAroundBox()
{
	//Actor位置がBoxの底面の中心になるようにする
	mEditorVoxelBox->SetBoxExtent(mAroundBoxExtentV);
}
/**
 * @brief	RayCastのテスト
 */
void AAIAssist_Octree::EditorDrawTestRayCast()
{
	if(FVector(mEditorTestRayCastTargetPos - mEditorTestRayCastBasePos).IsNearlyZero())
	{
		return;
	}

	FColor LineColor = FColor::Blue;
	if (Intersect(mEditorTestRayCastBasePos, mEditorTestRayCastTargetPos) != nullptr)
	{
		LineColor = FColor::Red;
	}
	DrawDebugLine(
		GetWorld(),
		mEditorTestRayCastBasePos,
		mEditorTestRayCastTargetPos,
		LineColor,
		false,
		-1.f,
		0,
		3.f
	);
}

/**
 * @brief	RayCastのテスト
 */
void AAIAssist_Octree::EditorDebugDraw(UCanvas* InCanvas)
{
	if (mbEditorDrawActiveVoxel)
	{
		DebugDrawActiveVoxel(InCanvas, mMaxDepth);
	}
	if (mbEditorDrawVoxelDetail)
	{
		DebugDrawVoxelDetail(InCanvas, mEditorDrawVoxelIndexX, mEditorDrawVoxelIndexY, mEditorDrawVoxelIndexZ);
	}
}

#endif// WITH_EDITOR

FAIAssist_OctreeNode::FAIAssist_OctreeNode()
{
	mChildNodeList.Reserve(msChildCount);
	for (int32 i = 0; i < msChildCount; ++i)
	{
		mChildNodeList.Add(nullptr);
	}
}

FAIAssist_OctreeNode::~FAIAssist_OctreeNode()
{
	for (int32 i = 0; i < msChildCount; ++i)
	{
		delete mChildNodeList[i];
	}
	mChildNodeList.Empty();
}

FAIAssist_OctreeNode* FAIAssist_OctreeNode::GetChild(const int32 InIndex) const
{
	if (InIndex < mChildNodeList.Num())
	{
		return mChildNodeList[InIndex];
	}
	return nullptr;
}

void FAIAssist_OctreeNode::CreateChild(const int32 InIndex)
{
	if (InIndex < mChildNodeList.Num()
		&& mChildNodeList[InIndex] == nullptr)
	{
		mChildNodeList[InIndex] = new FAIAssist_OctreeNode();
	}
}

bool FAIAssist_OctreeNode::IsLeaf() const
{
	for (const FAIAssist_OctreeNode* Node : mChildNodeList)
	{
		if (Node == nullptr)
		{
			return false;
		}
	}
	return true;
}
