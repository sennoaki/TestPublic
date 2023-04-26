// 複数の部屋とそれをつなぐ道をランダムで生成する

#include "AIAssist_DungeonMaker.h"

#include "Engine/Canvas.h"
#include "Debug/DebugDrawService.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "CanvasItem.h"
#include "RenderUtils.h"

// Sets default values
AAIAssist_DungeonMaker::AAIAssist_DungeonMaker()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AAIAssist_DungeonMaker::BeginPlay()
{
	Super::BeginPlay();

	GenerateGridNodeList();
	GenerateDungeon();

#if UE_BUILD_DEVELOPMENT
	DeubgRequestDraw(true);
#endif
}

void AAIAssist_DungeonMaker::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

#if UE_BUILD_DEVELOPMENT
	DeubgRequestDraw(false);
#endif
}

//ダンジョン生成
void AAIAssist_DungeonMaker::GenerateDungeon()
{
	GenerateGridNodeList();
	GenerateGridSpace();
	AssignGridSpaceIndex();
	GenerateRoom();
	AssignGridRoom();
	SetupRoadGridSpace();
	SetupRoadConnect();
	SetupRoadList();

	for (AActor* Actor : mStageActorList)
	{
		GetWorld()->DestroyActor(Actor);
	}
	mStageActorList.Empty();

	GenerateRoomActor();
	GenerateRoadActor();
}

//GridNodeListを準備
void AAIAssist_DungeonMaker::GenerateGridNodeList()
{
	mGridNodeList.Empty();

	int32 Index = 0;
	for (int32 x = 0; x < mGridNodeXNum; ++x)
	{
		for (int32 y = 0; y < mGridNodeYNum; ++y)
		{
			FAIAssist_DungeonGridNode GridNode;
			GridNode.mUID = Index;
			GridNode.mState = EAIAssist_DungeonGridState::Space;
			mGridNodeList.Add(GridNode);
			++Index;
		}
	}
}

//GridNodeを大きく分割してGridSpace単位を作る
void AAIAssist_DungeonMaker::GenerateGridSpace()
{
	mGridSpaceList.Empty();
	FAIAssist_DungeonGridNodeIndex BeginIndex(0, 0);
	FAIAssist_DungeonGridNodeIndex EndIndex(mGridNodeXNum, mGridNodeYNum);
	mGridSpaceList.Add(FAIAssist_DungeonGridSpace(BeginIndex, EndIndex));

	for (int32 i = 0; i < mGridSpaceList.Num(); )
	{
		const FAIAssist_DungeonGridSpace& GridSpace = mGridSpaceList[i];
		const int32 GridSpaceLenX = GridSpace.mEndIndex.mX - GridSpace.mBeginIndex.mX;
		const int32 GridSpaceLenY = GridSpace.mEndIndex.mY - GridSpace.mBeginIndex.mY;
		if (GridSpaceLenX < mGridSpaceLenMax
			&& GridSpaceLenY < mGridSpaceLenMax)
		{
			++i;
			continue;
		}

		//大きい方で分割
		FAIAssist_DungeonGridNodeIndex SplitBeginIndexA;
		FAIAssist_DungeonGridNodeIndex SplitEndIndexA;
		FAIAssist_DungeonGridNodeIndex SplitBeginIndexB;
		FAIAssist_DungeonGridNodeIndex SplitEndIndexB;
		bool bSplitX = GridSpaceLenX > GridSpaceLenY;
		if (GridSpaceLenX == GridSpaceLenY)
		{
			bSplitX = FMath::RandRange(0, 100) % 2 == 0;
		}
		//const bool bSplitX = FMath::RandRange(0, GridSpaceLenX + GridSpaceLenY) < GridSpaceLenX;
		if (bSplitX)
		{
			const int32 SplitMin = mGridSpaceLenMin;
			const int32 SplitMax = FMath::Min(GridSpaceLenX - mGridSpaceLenMin, mGridSpaceLenMax);
			const int32 SplitCount = FMath::RandRange(SplitMin, SplitMax);
			const int32 SplitX = GridSpace.mBeginIndex.mX + SplitCount;
			SplitBeginIndexA = GridSpace.mBeginIndex;
			SplitEndIndexA = GridSpace.mEndIndex;
			SplitEndIndexA.mX = SplitX;

			SplitBeginIndexB = GridSpace.mBeginIndex;
			SplitBeginIndexB.mX = SplitX;
			SplitEndIndexB = GridSpace.mEndIndex;
		}
		else
		{
			const int32 SplitMin = mGridSpaceLenMin;
			const int32 SplitMax = FMath::Min(GridSpaceLenY - mGridSpaceLenMin, mGridSpaceLenMax);
			const int32 SplitCount = FMath::RandRange(SplitMin, SplitMax);
			const int32 SplitY = GridSpace.mBeginIndex.mY + SplitCount;
			SplitBeginIndexA = GridSpace.mBeginIndex;
			SplitEndIndexA = GridSpace.mEndIndex;
			SplitEndIndexA.mY = SplitY;

			SplitBeginIndexB = GridSpace.mBeginIndex;
			SplitBeginIndexB.mY = SplitY;
			SplitEndIndexB = GridSpace.mEndIndex;
		}

		// 元のGridSpaceを消して、分割したSpaceを追加
		mGridSpaceList.RemoveAt(i);
		mGridSpaceList.Add(FAIAssist_DungeonGridSpace(SplitBeginIndexA, SplitEndIndexA));
		mGridSpaceList.Add(FAIAssist_DungeonGridSpace(SplitBeginIndexB, SplitEndIndexB));
	}

	for (int32 i=0; i<mGridSpaceList.Num(); ++i)
	{
		mGridSpaceList[i].mIndex = i;
	}
}

//各GridNodeに所属するGridSpaceのIndexを設定
void AAIAssist_DungeonMaker::AssignGridSpaceIndex()
{
	for (const FAIAssist_DungeonGridSpace& GridSpace : mGridSpaceList)
	{
		for (int32 x = GridSpace.mBeginIndex.mX; x <= GridSpace.mEndIndex.mX; ++x)
		{
			for (int32 y = GridSpace.mBeginIndex.mY; y <= GridSpace.mEndIndex.mY; ++y)
			{
				if (FAIAssist_DungeonGridNode* GridNode = GetGridNode(x, y))
				{
					GridNode->mGridSpaceIndex = GridSpace.mIndex;
					GridNode->mState = EAIAssist_DungeonGridState::Space;
				}
			}
		}
	}
}

//GridSpace内にGridRoomを生成
void AAIAssist_DungeonMaker::GenerateRoom()
{
	mRoomList.Empty();
	for (const FAIAssist_DungeonGridSpace& GridSpace : mGridSpaceList)
	{
		GenerateRoom(GridSpace);
	}
}

void AAIAssist_DungeonMaker::GenerateRoom(const FAIAssist_DungeonGridSpace& InGridSpace)
{
	FAIAssist_DungeonGridNodeIndex BeginIndex = InGridSpace.mBeginIndex;
	FAIAssist_DungeonGridNodeIndex EndIndex = InGridSpace.mEndIndex;
	const int32 GridSpaceLenX = InGridSpace.mEndIndex.mX - InGridSpace.mBeginIndex.mX;
	const int32 GridSpaceLenY = InGridSpace.mEndIndex.mY - InGridSpace.mBeginIndex.mY;
	if (GridSpaceLenX > mGridRoomLenMin)
	{
		int32 OffsetXMax = GridSpaceLenX - mGridRoomLenMin;
		const int32 OffsetBeginX = FMath::RandRange(1,OffsetXMax-1);
		BeginIndex.mX += OffsetBeginX;

		OffsetXMax -= OffsetBeginX;
		if (OffsetXMax > 0)
		{
			const int32 OffsetEndX = FMath::RandRange(1, OffsetXMax-1);
			EndIndex.mX -= OffsetEndX;
		}
	}

	if( GridSpaceLenY > mGridRoomLenMin)
	{
		int32 OffsetYMax = GridSpaceLenY - mGridRoomLenMin;
		const int32 OffsetBeginY = FMath::RandRange(1, OffsetYMax-1);
		BeginIndex.mY += OffsetBeginY;

		OffsetYMax -= OffsetBeginY;
		if (OffsetYMax > 0)
		{
			const int32 OffsetEndY = FMath::RandRange(1, OffsetYMax-1);
			EndIndex.mY -= OffsetEndY;
		}
	}

	FAIAssist_DungeonRoom Room(BeginIndex, EndIndex);
	Room.mGridSpaceIndex = InGridSpace.mIndex;
	mRoomList.Add(Room);
}

//Roomに所属するGridのStateを更新
void AAIAssist_DungeonMaker::AssignGridRoom()
{
	for (const FAIAssist_DungeonRoom& Room : mRoomList)
	{
		for (int32 x = Room.mBeginIndex.mX; x < Room.mEndIndex.mX; ++x)
		{
			for (int32 y = Room.mBeginIndex.mY; y < Room.mEndIndex.mY; ++y)
			{
				if (FAIAssist_DungeonGridNode* GridNode = GetGridNode(x, y))
				{
					GridNode->mState = EAIAssist_DungeonGridState::Room;
				}
			}
		}
	}
}

//RoomからSpaceの端までRoadを設定
void AAIAssist_DungeonMaker::SetupRoadGridSpace()
{
	mCheckConnectRoadXGridNodeList.Empty();
	mCheckConnectRoadYGridNodeList.Empty();

	for (FAIAssist_DungeonRoom& Room : mRoomList)
	{
		SetupRoadGridSpace(Room);
	}
}

void AAIAssist_DungeonMaker::SetupRoadGridSpace(FAIAssist_DungeonRoom& InRoom)
{
	//Roomの各辺からGridSpaceの端まで道にする
	const FAIAssist_DungeonGridSpace& GridSpace = mGridSpaceList[InRoom.mGridSpaceIndex];

	//+X
	SetupRoadGridSpaceX(InRoom, true);
	//-X
	SetupRoadGridSpaceX(InRoom, false);

	//+Y
	SetupRoadGridSpaceY(InRoom, true);
	//-Y
	SetupRoadGridSpaceY(InRoom, false);
}

void AAIAssist_DungeonMaker::SetupRoadGridSpaceX(FAIAssist_DungeonRoom& InRoom, const bool bInPlusDirection)
{
	const FAIAssist_DungeonGridSpace& GridSpace = mGridSpaceList[InRoom.mGridSpaceIndex];

	int32 RoomIndexX = InRoom.mEndIndex.mX;
	int32 MinX = InRoom.mEndIndex.mX;
	int32 MaxX = GridSpace.mEndIndex.mX;
	if (!bInPlusDirection)
	{
		RoomIndexX = InRoom.mBeginIndex.mX;
		MaxX = InRoom.mBeginIndex.mX;
		MinX = GridSpace.mBeginIndex.mX;
	}

	if (MinX == 0
		|| MaxX >= mGridNodeXNum - 1)
	{
		return;
	}

	if (MinX < MaxX)
	{
		const int32 BeginY = InRoom.mBeginIndex.mY;
		const int32 EndY = InRoom.mEndIndex.mY - 1;
		const int32 RoadY = FMath::RandRange(BeginY, EndY);

		if (const FAIAssist_DungeonGridNode* GridNode = GetGridNode(RoomIndexX, RoadY))
		{
			InRoom.mConnectRoadGridNodeUIDList.Add(GridNode->mUID);
		}

		for (int32 x = MinX; x < MaxX; ++x)
		{
			if (FAIAssist_DungeonGridNode* GridNode = GetGridNode(x, RoadY))
			{
				GridNode->mState = EAIAssist_DungeonGridState::Road;
				if (bInPlusDirection)
				{
					if (x == MaxX - 1)
					{
						mCheckConnectRoadYGridNodeList.Add(GridNode);
					}
				}
				else
				{
					if (x == MinX)
					{
						mCheckConnectRoadYGridNodeList.Add(GridNode);
					}
				}
			}
		}
	}
}

void AAIAssist_DungeonMaker::SetupRoadGridSpaceY(FAIAssist_DungeonRoom& InRoom, const bool bInPlusDirection)
{
	const FAIAssist_DungeonGridSpace& GridSpace = mGridSpaceList[InRoom.mGridSpaceIndex];

	int32 RoomIndexY = InRoom.mEndIndex.mY;
	int32 MinY = InRoom.mEndIndex.mY;
	int32 MaxY = GridSpace.mEndIndex.mY;
	if (!bInPlusDirection)
	{
		RoomIndexY = InRoom.mBeginIndex.mY;
		MaxY = InRoom.mBeginIndex.mY;
		MinY = GridSpace.mBeginIndex.mY;
	}

	if (MinY == 0
		|| MaxY >= mGridNodeYNum - 1)
	{
		return;
	}

	if (MinY < MaxY)
	{
		const int32 BeginX = InRoom.mBeginIndex.mX;
		const int32 EndX = InRoom.mEndIndex.mX - 1;
		const int32 RoadX = FMath::RandRange(BeginX, EndX);

		if (const FAIAssist_DungeonGridNode* GridNode = GetGridNode(RoadX, RoomIndexY))
		{
			InRoom.mConnectRoadGridNodeUIDList.Add(GridNode->mUID);
		}

		for (int32 y = MinY; y < MaxY; ++y)
		{
			if (FAIAssist_DungeonGridNode* GridNode = GetGridNode(RoadX, y))
			{
				GridNode->mState = EAIAssist_DungeonGridState::Road;
				if (bInPlusDirection)
				{
					if (y == MaxY - 1)
					{
						mCheckConnectRoadXGridNodeList.Add(GridNode);
					}
				}
				else
				{
					if (y == MinY)
					{
						mCheckConnectRoadXGridNodeList.Add(GridNode);
					}
				}
			}
		}
	}
}

//Roomから伸びてるRoadを結んで道を生成
void AAIAssist_DungeonMaker::SetupRoadConnect()
{
	for (const FAIAssist_DungeonGridNode* GridNode : mCheckConnectRoadXGridNodeList)
	{
		SetupRoadConnectDirectionX(*GridNode, 1);
	}

	for (const FAIAssist_DungeonGridNode* GridNode : mCheckConnectRoadYGridNodeList)
	{
		SetupRoadConnectDirectionY(*GridNode, 1);
	}
}

void AAIAssist_DungeonMaker::SetupRoadConnectDirectionX(const FAIAssist_DungeonGridNode& InBaseGridNode, const int32 InOffsetIndex)
{
	int32 BaseGridIndexX = INDEX_NONE;
	int32 BaseGridIndexY = INDEX_NONE;
	if (!CalcGridNodeXY(BaseGridIndexX, BaseGridIndexY, InBaseGridNode.mUID))
	{
		return;
	}
	//+Y側が別のSpace
	const int32 CheckGridIndexX = BaseGridIndexX;
	const int32 CheckGridIndexY = BaseGridIndexY + 1;
	const FAIAssist_DungeonGridNode* CheckGridNode = GetGridNode(CheckGridIndexX, CheckGridIndexY);
	if (CheckGridNode == nullptr)
	{
		return;
	}

	if (CheckGridNode->mGridSpaceIndex == InBaseGridNode.mGridSpaceIndex
		|| CheckGridNode->mState != EAIAssist_DungeonGridState::Space)
	{
		return;
	}

	const FAIAssist_DungeonGridSpace& CheckGridSpace = mGridSpaceList[CheckGridNode->mGridSpaceIndex];
	const int32 CheckXBegin = CheckGridSpace.mBeginIndex.mX;
	const int32 CheckXEnd = CheckGridSpace.mEndIndex.mX;
	int32 CheckRoadX = INDEX_NONE;
	for (int32 x = CheckXBegin; x < CheckXEnd; ++x)
	{//隣のSpaceの端のxを順に見て道を探す
		if (const FAIAssist_DungeonGridNode* SpaceEdgeGridNode = GetGridNode(x, CheckGridIndexY))
		{
			if (SpaceEdgeGridNode->mState == EAIAssist_DungeonGridState::Road)
			{
				CheckRoadX = x;
				break;
			}
		}
	}

	if (CheckRoadX != INDEX_NONE)
	{
		int32 RoadXBegin = CheckGridIndexX;
		int32 RoadXEnd = CheckRoadX;
		if (RoadXBegin > RoadXEnd)
		{
			RoadXBegin = CheckRoadX;
			RoadXEnd = CheckGridIndexX;
		}
		for (int32 x = RoadXBegin; x <= RoadXEnd; ++x)
		{
			if (FAIAssist_DungeonGridNode* RoadGridNode = GetGridNode(x, CheckGridIndexY))
			{
				RoadGridNode->mState = EAIAssist_DungeonGridState::Road;
			}
		}
	}
}

void AAIAssist_DungeonMaker::SetupRoadConnectDirectionY(const FAIAssist_DungeonGridNode& InBaseGridNode, const int32 InOffsetIndex)
{
	int32 BaseGridIndexX = INDEX_NONE;
	int32 BaseGridIndexY = INDEX_NONE;
	if (!CalcGridNodeXY(BaseGridIndexX, BaseGridIndexY, InBaseGridNode.mUID))
	{
		return;
	}
	//+X側が別のSpace
	const int32 CheckGridIndexX = BaseGridIndexX + 1;
	const int32 CheckGridIndexY = BaseGridIndexY;
	const FAIAssist_DungeonGridNode* CheckGridNode = GetGridNode(CheckGridIndexX, CheckGridIndexY);
	if (CheckGridNode == nullptr)
	{
		return;
	}

	if (CheckGridNode->mGridSpaceIndex == InBaseGridNode.mGridSpaceIndex
		|| CheckGridNode->mState != EAIAssist_DungeonGridState::Space)
	{
		return;
	}
	
	const FAIAssist_DungeonGridSpace& CheckGridSpace = mGridSpaceList[CheckGridNode->mGridSpaceIndex];
	const int32 CheckYBegin = CheckGridSpace.mBeginIndex.mY;
	const int32 CheckYEnd = CheckGridSpace.mEndIndex.mY;
	int32 CheckRoadY = INDEX_NONE;
	for (int32 y = CheckYBegin; y < CheckYEnd; ++y)
	{//隣のSpaceの端のxを順に見て道を探す
		if (const FAIAssist_DungeonGridNode* SpaceEdgeGridNode = GetGridNode(CheckGridIndexX, y))
		{
			if (SpaceEdgeGridNode->mState == EAIAssist_DungeonGridState::Road)
			{
				CheckRoadY = y;
				break;
			}
		}
	}

	if (CheckRoadY != INDEX_NONE)
	{
		int32 RoadYBegin = CheckGridIndexY;
		int32 RoadYEnd = CheckRoadY;
		if (RoadYBegin > RoadYEnd)
		{
			RoadYBegin = CheckRoadY;
			RoadYEnd = CheckGridIndexY;
		}
		for (int32 y = RoadYBegin; y <= RoadYEnd; ++y)
		{
			if (FAIAssist_DungeonGridNode* RoadGridNode = GetGridNode(CheckGridIndexX, y))
			{
				RoadGridNode->mState = EAIAssist_DungeonGridState::Road;
			}
		}
	}
}

//繋がった道毎にリストにまとめる
void AAIAssist_DungeonMaker::SetupRoadList()
{
	mRoadList.Empty();

	for (const FAIAssist_DungeonRoom& Room : mRoomList)
	{
		for (const int32 GridNodeUID : Room.mConnectRoadGridNodeUIDList)
		{
			SetupRoadList(GridNodeUID);
		}
	}
}

void AAIAssist_DungeonMaker::SetupRoadList(const int32 InBaseGridNodeUID)
{
	const FAIAssist_DungeonGridNode* CheckGridNode = GetGridNode(InBaseGridNodeUID);
	if (CheckGridNode == nullptr)
	{
		return;
	}
	if (CheckGridNode->mRoadIndex != INDEX_NONE)
	{//すでに設定済み
		return;
	}

	FAIAssist_DungeonRoad DungeonRoad;
	DungeonRoad.mGridNodeUIDList.Add(InBaseGridNodeUID);

	int32 OffsetX = 0;
	int32 OffsetY = 0;
	while (true)
	{
		int32 CheckGridIndexX = INDEX_NONE;
		int32 CheckGridIndexY = INDEX_NONE;
		if (!CalcGridNodeXY(CheckGridIndexX, CheckGridIndexY, CheckGridNode->mUID))
		{
			return;
		}
		if (OffsetX != 0
			|| OffsetY != 0)
		{
			if (FAIAssist_DungeonGridNode* OffsetGridNode = GetGridNode(CheckGridIndexX + OffsetX, CheckGridIndexY + OffsetY))
			{
				if( OffsetGridNode->mRoadIndex != INDEX_NONE
					&& OffsetGridNode->mState == EAIAssist_DungeonGridState::Road
					&& DungeonRoad.mGridNodeUIDList.Find(OffsetGridNode->mUID) == -1
					)
				{
					DungeonRoad.mGridNodeUIDList.Add(OffsetGridNode->mUID);
					CheckGridNode = OffsetGridNode;
					continue;
				}
			}
		}

		{//+x方向に道があるか
			if (CheckAddRoadList(DungeonRoad, CheckGridIndexX + 1, CheckGridIndexY))
			{
				CheckGridNode = GetGridNode(CheckGridIndexX + 1, CheckGridIndexY);
				OffsetX = 1;
				OffsetY = 0;
				continue;
			}
		}
		{//-x方向に道があるか
			if (CheckAddRoadList(DungeonRoad, CheckGridIndexX - 1, CheckGridIndexY))
			{
				CheckGridNode = GetGridNode(CheckGridIndexX - 1, CheckGridIndexY);
				OffsetX = -1;
				OffsetY = 0;
				continue;
			}
		}
		{//+y方向に道があるか
			if (CheckAddRoadList(DungeonRoad, CheckGridIndexX, CheckGridIndexY + 1))
			{
				CheckGridNode = GetGridNode(CheckGridIndexX, CheckGridIndexY + 1);
				OffsetX = 0;
				OffsetY = 1;
				continue;
			}
		}
		{//-y方向に道があるか
			if (CheckAddRoadList(DungeonRoad, CheckGridIndexX, CheckGridIndexY - 1))
			{
				CheckGridNode = GetGridNode(CheckGridIndexX, CheckGridIndexY - 1);
				OffsetX = 0;
				OffsetY = -1;
				continue;
			}
		}
		break;
	}

	if (DungeonRoad.mGridNodeUIDList.Num() < 1)
	{
		return;
	}

	const int32 RoadListIndex = mRoomList.Num();
	for (const int32 GridNodeUID : DungeonRoad.mGridNodeUIDList)
	{
		if (FAIAssist_DungeonGridNode* GridNode = GetGridNode(GridNodeUID))
		{
			GridNode->mRoadIndex = RoadListIndex;
		}
	}

	mRoadList.Add(DungeonRoad);
}

bool AAIAssist_DungeonMaker::CheckAddRoadList(FAIAssist_DungeonRoad& OutRoad, const int32 InCheckIndexX, const int32 InCheckIndexY)
{
	if (FAIAssist_DungeonGridNode* OffsetGridNode = GetGridNode(InCheckIndexX, InCheckIndexY))
	{
		if (OffsetGridNode->mRoadIndex == INDEX_NONE
			&& OffsetGridNode->mState == EAIAssist_DungeonGridState::Road
			&& OutRoad.mGridNodeUIDList.Find(OffsetGridNode->mUID) == -1
			)
		{
			OutRoad.mGridNodeUIDList.Add(OffsetGridNode->mUID);
			return true;
		}
	}
	return false;
}

//部屋を構成するActorを生成
void AAIAssist_DungeonMaker::GenerateRoomActor()
{
	TSubclassOf<AActor> FloorActorClass = mFloorActorClass.LoadSynchronous();
	if (FloorActorClass == nullptr)
	{
		return;
	}

	const FVector CenterPos = GetActorLocation();
	const FVector ExtentV = FVector(static_cast<float>(mGridNodeXNum), static_cast<float>(mGridNodeYNum), 0.f) * mGridNodeLength * 0.5f;
	const FVector BeginEdgePos = CenterPos - ExtentV;
	const FVector GridCenterOffsetV = FVector(mGridNodeLength,mGridNodeLength,0.f)*0.5f;
	for(const FAIAssist_DungeonRoom& Room : mRoomList)
	{
		const FVector RoomBeginEdgePos = BeginEdgePos
			+ FVector(static_cast<float>(Room.mBeginIndex.mX)*mGridNodeLength, static_cast<float>(Room.mBeginIndex.mY) * mGridNodeLength, 0.f);
		const FVector RoomEndEdgePos = BeginEdgePos
			+ FVector(static_cast<float>(Room.mEndIndex.mX) * mGridNodeLength, static_cast<float>(Room.mEndIndex.mY) * mGridNodeLength, 0.f);
		const FVector FloorCenterPos = (RoomBeginEdgePos + RoomEndEdgePos)*0.5f;
		const FVector FloorSizeV(RoomEndEdgePos - RoomBeginEdgePos);
		const FVector FloorScaleV(FloorSizeV.X*0.01f, FloorSizeV.Y*0.01f, 1.f);//1辺が100cm想定なので
		const FTransform FloorTransform(FQuat::Identity, FloorCenterPos, FloorScaleV);
		CreateFloorActor(FloorTransform);

		//各辺の壁をチェック
		GenerateRoomWallX(Room, true);
		GenerateRoomWallX(Room, false);
		GenerateRoomWallY(Room, true);
		GenerateRoomWallY(Room, false);
	}
}

//指定Roomの+X,-Xの辺の壁を生成
void AAIAssist_DungeonMaker::GenerateRoomWallX(const FAIAssist_DungeonRoom& InRoom, bool bInPlusDirection)
{
	const FVector CenterPos = GetActorLocation();
	const FVector ExtentV = FVector(static_cast<float>(mGridNodeXNum), static_cast<float>(mGridNodeYNum), 0.f) * mGridNodeLength * 0.5f;
	const FVector BeginEdgePos = CenterPos - ExtentV;
	const FVector GridCenterOffsetV = FVector(mGridNodeLength, mGridNodeLength, 0.f) * 0.5f;

	FVector OffsetWallPos = FVector(-mGridNodeLength * 0.5f, 0.f, mWallLength * 0.5f);
	int32 CheckIndexX = InRoom.mEndIndex.mX;
	if (!bInPlusDirection)
	{
		OffsetWallPos = FVector(mGridNodeLength * 0.5f, 0.f, mWallLength * 0.5f);
		CheckIndexX = InRoom.mBeginIndex.mX - 1;
	}

	const int32 BeginIndexY = InRoom.mBeginIndex.mY;
	const int32 EndIndexY = InRoom.mEndIndex.mY;
	for (int32 y = BeginIndexY; y < EndIndexY; ++y)
	{
		if (const FAIAssist_DungeonGridNode* GridNode = GetGridNode(CheckIndexX, y))
		{
			if (GridNode->mState == EAIAssist_DungeonGridState::Road)
			{
				continue;
			}
		}

		const FVector GridEdgePos = BeginEdgePos
			+ FVector(static_cast<float>(CheckIndexX) * mGridNodeLength, static_cast<float>(y) * mGridNodeLength, 0.f);
		const FVector GridCenterPos = GridEdgePos + GridCenterOffsetV;
		const FVector WallPos = GridCenterPos + OffsetWallPos;
		const FVector WallScaleV(mGridNodeLength * 0.01f, mWallLength * 0.01f, 1.f);//元が回転してるのでYでいい
		const FRotator WallRot(0.f, 90.f, 0.f);
		const FTransform WallTransform(WallRot.Quaternion(), WallPos, WallScaleV);
		CreateWallActor(WallTransform);
	}
}

//指定Roomの+Y,-Yの辺の壁を生成
void AAIAssist_DungeonMaker::GenerateRoomWallY(const FAIAssist_DungeonRoom& InRoom, bool bInPlusDirection)
{
	const FVector CenterPos = GetActorLocation();
	const FVector ExtentV = FVector(static_cast<float>(mGridNodeXNum), static_cast<float>(mGridNodeYNum), 0.f) * mGridNodeLength * 0.5f;
	const FVector BeginEdgePos = CenterPos - ExtentV;
	const FVector GridCenterOffsetV = FVector(mGridNodeLength, mGridNodeLength, 0.f) * 0.5f;

	FVector OffsetWallPos = FVector(0.f, -mGridNodeLength * 0.5f, mWallLength * 0.5f);
	int32 CheckIndexY = InRoom.mEndIndex.mY;
	if (!bInPlusDirection)
	{
		OffsetWallPos = FVector(0.f, mGridNodeLength * 0.5f, mWallLength * 0.5f);
		CheckIndexY = InRoom.mBeginIndex.mY - 1;
	}

	const int32 BeginIndexX = InRoom.mBeginIndex.mX;
	const int32 EndIndexX = InRoom.mEndIndex.mX;
	for (int32 x = BeginIndexX; x < EndIndexX; ++x)
	{
		if (const FAIAssist_DungeonGridNode* GridNode = GetGridNode(x, CheckIndexY))
		{
			if (GridNode->mState == EAIAssist_DungeonGridState::Road)
			{
				continue;
			}
		}

		const FVector GridEdgePos = BeginEdgePos
			+ FVector(static_cast<float>(x) * mGridNodeLength, static_cast<float>(CheckIndexY) * mGridNodeLength, 0.f);
		const FVector GridCenterPos = GridEdgePos + GridCenterOffsetV;
		const FVector WallPos = GridCenterPos + OffsetWallPos;
		const FVector WallScaleV(mGridNodeLength * 0.01f, mWallLength * 0.01f, 1.f);//元が回転してるのでYでいい
		const FRotator WallRot(0.f, 0.f, 0.f);
		const FTransform WallTransform(WallRot.Quaternion(), WallPos, WallScaleV);
		CreateWallActor(WallTransform);
	}
}

void AAIAssist_DungeonMaker::GenerateRoadActor()
{
	const FVector CenterPos = GetActorLocation();
	const FVector ExtentV = FVector(static_cast<float>(mGridNodeXNum), static_cast<float>(mGridNodeYNum), 0.f) * mGridNodeLength * 0.5f;
	const FVector BeginEdgePos = CenterPos - ExtentV;
	const FVector GridCenterOffsetV = FVector(mGridNodeLength, mGridNodeLength, 0.f) * 0.5f;
	for (const FAIAssist_DungeonRoad& DungeonRoad : mRoadList)
	{
		const int32 RoadNum = DungeonRoad.mGridNodeUIDList.Num();
		for (int32 i=0; i< RoadNum; ++i)
		{
			const int32 GridNodeUID = DungeonRoad.mGridNodeUIDList[i];
			int32 GridIndexX = INDEX_NONE;
			int32 GridIndexY = INDEX_NONE;
			if (!CalcGridNodeXY(GridIndexX, GridIndexY, GridNodeUID))
			{
				continue;
			}

			const FVector GridEdgePos = BeginEdgePos
				+ FVector(static_cast<float>(GridIndexX) * mGridNodeLength, static_cast<float>(GridIndexY) * mGridNodeLength, 0.f);
			const FVector GridCenterPos = GridEdgePos + GridCenterOffsetV;
			const FVector ScaleV(mGridNodeLength * 0.01f, mGridNodeLength * 0.01f, 1.f);//1辺が100cm想定なので
			FTransform SpawnTransform(FQuat::Identity, GridCenterPos, ScaleV);
			CreateFloorActor(SpawnTransform);

			const bool bEdgeRoadNode = ( i==0 || i== RoadNum-1);
			GenerateRoadWall(GridNodeUID, bEdgeRoadNode);
		}
	}
}

void AAIAssist_DungeonMaker::GenerateRoadWall(const int32 InGridNodeUID, const bool bInEdgeRoadNode)
{
	int32 GridIndexX = INDEX_NONE;
	int32 GridIndexY = INDEX_NONE;
	if (!CalcGridNodeXY(GridIndexX, GridIndexY, InGridNodeUID))
	{
		return;
	}

	const FVector CenterPos = GetActorLocation();
	const FVector ExtentV = FVector(static_cast<float>(mGridNodeXNum), static_cast<float>(mGridNodeYNum), 0.f) * mGridNodeLength * 0.5f;
	const FVector BeginEdgePos = CenterPos - ExtentV;
	const FVector GridCenterOffsetV = FVector(mGridNodeLength, mGridNodeLength, 0.f) * 0.5f;

	//壁が必要なGridかどうか
	auto IsNeedWall = [&](const int32 InIndexX, const int32 InIndexY)
	{
		if (const FAIAssist_DungeonGridNode* GridNode = GetGridNode(InIndexX, InIndexY))
		{
			if (bInEdgeRoadNode)
			{
				if (GridNode->mState != EAIAssist_DungeonGridState::Road
					&& GridNode->mState != EAIAssist_DungeonGridState::Room)
				{
					return true;
				}
			}
			else if (GridNode->mState != EAIAssist_DungeonGridState::Road)
			{
				return true;
			}
		}
		else
		{
			return true;
		}
		return false;
	};

	//+x,-x方向に壁が必要なら作成
	auto CheckXWall = [&](const bool bInPlusDirection)
	{
		int32 CheckIndexX = GridIndexX + 1;
		FVector OffsetWallPos(mGridNodeLength * 0.5f, 0.f, mWallLength * 0.5f);
		if (!bInPlusDirection)
		{
			CheckIndexX = GridIndexX -1;
			OffsetWallPos = FVector(-mGridNodeLength * 0.5f, 0.f, mWallLength * 0.5f);
		}
		if (IsNeedWall(CheckIndexX, GridIndexY))
		{
			const FVector GridEdgePos = BeginEdgePos
				+ FVector(static_cast<float>(GridIndexX) * mGridNodeLength, static_cast<float>(GridIndexY) * mGridNodeLength, 0.f);
			const FVector GridCenterPos = GridEdgePos + GridCenterOffsetV;
			const FVector WallPos = GridCenterPos + OffsetWallPos;
			const FVector WallScaleV(mGridNodeLength * 0.01f, mWallLength * 0.01f, 1.f);//元が回転してるのでYでいい
			const FRotator WallRot(0.f, 90.f, 0.f);
			const FTransform WallTransform(WallRot.Quaternion(), WallPos, WallScaleV);
			CreateWallActor(WallTransform);
		}
	};

	//+y,-y方向に壁が必要なら作成
	auto CheckYWall = [&](const bool bInPlusDirection)
	{
		int32 CheckIndexY = GridIndexY + 1;
		FVector OffsetWallPos(0.f, mGridNodeLength * 0.5f, mWallLength * 0.5f);
		if (!bInPlusDirection)
		{
			CheckIndexY = GridIndexY - 1;
			OffsetWallPos = FVector(0.f, -mGridNodeLength * 0.5f, mWallLength * 0.5f);
		}
		if (IsNeedWall(GridIndexX, CheckIndexY))
		{
			const FVector GridEdgePos = BeginEdgePos
				+ FVector(static_cast<float>(GridIndexX) * mGridNodeLength, static_cast<float>(GridIndexY) * mGridNodeLength, 0.f);
			const FVector GridCenterPos = GridEdgePos + GridCenterOffsetV;
			const FVector WallPos = GridCenterPos + OffsetWallPos;
			const FVector WallScaleV(mGridNodeLength * 0.01f, mWallLength * 0.01f, 1.f);//元が回転してるのでYでいい
			const FRotator WallRot(0.f, 0.f, 0.f);
			const FTransform WallTransform(WallRot.Quaternion(), WallPos, WallScaleV);
			CreateWallActor(WallTransform);
		}
	};
	
	CheckXWall(true);
	CheckXWall(false);
	CheckYWall(true);
	CheckYWall(false);
}

//床のActor生成
void AAIAssist_DungeonMaker::CreateFloorActor(const FTransform& InTransform)
{
	TSubclassOf<AActor> FloorActorClass = mFloorActorClass.LoadSynchronous();
	if (FloorActorClass == nullptr)
	{
		return;
	}
	AActor* FloorActor = GetWorld()->SpawnActorDeferred<AActor>(
		FloorActorClass,
		InTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
		);
	
	if (FloorActor)
	{
		FloorActor->FinishSpawning(InTransform);
		mStageActorList.Add(FloorActor);
	}
}

//壁のActor生成
void AAIAssist_DungeonMaker::CreateWallActor(const FTransform& InTransform)
{
	TSubclassOf<AActor> WallActorClass = mWallActorClass.LoadSynchronous();
	if (WallActorClass == nullptr)
	{
		return;
	}
	AActor* WallActor = GetWorld()->SpawnActorDeferred<AActor>(
		WallActorClass,
		InTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
		);

	if (WallActor)
	{
		WallActor->FinishSpawning(InTransform);
		mStageActorList.Add(WallActor);
	}
}

/**
 * @brief	Gridのx,yのIndexからUIDを取得
 */
int32 AAIAssist_DungeonMaker::CalcGridNodeUID(const int32 InX, const int32 InY) const
{
	if (InX < 0 || InX >= mGridNodeXNum
		|| InY < 0 || InY >= mGridNodeYNum
		)
	{
		return INDEX_NONE;
	}
	return InY + InX *mGridNodeYNum;
}

/**
 * @brief	Voxel全体のIndexからx,y,zのIndexを取得
 */
bool AAIAssist_DungeonMaker::CalcGridNodeXY(int32& OutX, int32& OutY, const int32 InUID) const
{
	if (InUID < 0
		|| InUID > mGridNodeXNum*mGridNodeYNum)
	{
		OutX = INDEX_NONE;
		OutY = INDEX_NONE;
		return false;
	}

	OutX = InUID / mGridNodeYNum;
	OutY = InUID - (OutX*mGridNodeYNum);
	return true;
}

FAIAssist_DungeonGridNode* AAIAssist_DungeonMaker::GetGridNode(const int32 InUID)
{
	if (InUID >= 0
		&& InUID < mGridNodeXNum * mGridNodeYNum)
	{
		return &mGridNodeList[InUID];
	}
	return nullptr;
}
FAIAssist_DungeonGridNode* AAIAssist_DungeonMaker::GetGridNode(const int32 InX, const int32 InY)
{
	const int32 GridNodeUID = CalcGridNodeUID(InX, InY);
	if (GridNodeUID != INDEX_NONE
		&& GridNodeUID >= 0
		&& GridNodeUID < mGridNodeXNum*mGridNodeYNum)
	{
		return &mGridNodeList[GridNodeUID];
	}
	return nullptr;
}

// Called every frame
void AAIAssist_DungeonMaker::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

#if UE_BUILD_DEVELOPMENT
/**
 * @brief	Draw登録のon/off
 */
void	AAIAssist_DungeonMaker::DeubgRequestDraw(const bool bInActive)
{
	if (bInActive)
	{
		if (!mDebugDrawHandle.IsValid())
		{
			const auto DebugDrawDelegate = FDebugDrawDelegate::CreateUObject(this, &AAIAssist_DungeonMaker::DebugDraw);
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
void AAIAssist_DungeonMaker::DebugDraw(UCanvas* InCanvas, APlayerController* InPlayerController)
{
	if (mbDebugDraw2DGrid
		|| mbDebugDraw2DGridSpace)
	{
		DebugDraw2DMap(InCanvas);
	}

	if (mbDebugDraw3DGrid)
	{
		DebugDraw3DMap(InCanvas);
	}
}

void AAIAssist_DungeonMaker::DebugDraw2DMap(UCanvas* InCanvas)
{
	const FLinearColor WindowBackColor(0.01f, 0.01f, 0.01f, 0.5f);
	const FLinearColor WindowFrameColor(0.1f, 0.9f, 0.1f, 1.f);
	float BaseWindowWidth = 300.f;
	float BaseWindowHeight = 300.f;

	FVector2D WindowEdgePos(30.f,30.f);
	const uint32 WindowPointListSize = 4;
	const FVector2D WindowPointList[WindowPointListSize] = {
		FVector2D(WindowEdgePos.X, WindowEdgePos.Y),//左上
		FVector2D(WindowEdgePos.X, WindowEdgePos.Y + BaseWindowHeight),//左下
		FVector2D(WindowEdgePos.X + BaseWindowWidth, WindowEdgePos.Y + BaseWindowHeight),//右下
		FVector2D(WindowEdgePos.X + BaseWindowWidth, WindowEdgePos.Y)//右上
	};
	{//下敷き
		{
			FCanvasTriangleItem TileItem(WindowPointList[0], WindowPointList[1], WindowPointList[2], GWhiteTexture);
			TileItem.SetColor(WindowBackColor);
			TileItem.BlendMode = SE_BLEND_Translucent;
			InCanvas->DrawItem(TileItem);
		}
		{
			FCanvasTriangleItem TileItem(WindowPointList[2], WindowPointList[3], WindowPointList[0], GWhiteTexture);
			TileItem.SetColor(WindowBackColor);
			TileItem.BlendMode = SE_BLEND_Translucent;
			InCanvas->DrawItem(TileItem);
		}
	}
	for (uint32 i = 0; i < WindowPointListSize; ++i)
	{//枠
		DrawDebugCanvas2DLine(InCanvas, WindowPointList[i], WindowPointList[(i + 1) % WindowPointListSize], WindowFrameColor);
	}

	const float GridDispInsideLen = 2.f;
	const float GridDispLenX = BaseWindowWidth / static_cast<float>(mGridNodeXNum);
	const float GridDispLenY = BaseWindowHeight / static_cast<float>(mGridNodeYNum);
	const FVector2D GridDispExtent(GridDispLenX*0.5f-GridDispInsideLen, GridDispLenY*0.5f-GridDispInsideLen);
	const FVector2D GridDispCenterOffset = GridDispExtent;
	const FVector2D GridDispBasePos = WindowEdgePos + FVector2D(GridDispInsideLen, GridDispInsideLen);

	const int32 SpaceColorNum = 8;
	const FLinearColor SpaceColor[SpaceColorNum] = {
		FColor::Red,
		FColor::Blue,
		FColor::Yellow,
		FColor::Magenta,
		FColor::Orange,
		FColor::Purple,
		FColor::Turquoise,
		FColor::Cyan,
	};

	if(mbDebugDraw2DGridSpace)
	{
		int32 SpaceIndex = 0;
		for(const FAIAssist_DungeonGridSpace& GridSpace : mGridSpaceList)
		{
			const FVector2D BeginPos(static_cast<float>(GridSpace.mBeginIndex.mX) * GridDispLenX, static_cast<float>(GridSpace.mBeginIndex.mY) * GridDispLenY);
			const FVector2D EndPos(static_cast<float>(GridSpace.mEndIndex.mX) * GridDispLenX, static_cast<float>(GridSpace.mEndIndex.mY) * GridDispLenY);
			const FVector2D CenterPos = (BeginPos+EndPos)*0.5f;
			const FVector2D ExtenetV = EndPos - CenterPos - FVector2D(GridDispInsideLen, GridDispInsideLen);
			DrawCanvasQuadrangle(InCanvas, CenterPos+GridDispBasePos, ExtenetV, SpaceColor[SpaceIndex%SpaceColorNum]);
			++SpaceIndex;
		}
	}

	if(mbDebugDraw2DGrid)
	{
		int32 Index = 0;
		for (int32 x = 0; x < mGridNodeXNum; ++x)
		{
			for (int32 y = 0; y < mGridNodeYNum; ++y)
			{
				const FAIAssist_DungeonGridNode& GridNode = mGridNodeList[Index];
				FLinearColor GridNodeColor = WindowFrameColor;
				switch (GridNode.mState)
				{
 				case EAIAssist_DungeonGridState::Space:
 					GridNodeColor = FColor::Silver;
 					break;
 				case EAIAssist_DungeonGridState::Road:
 					GridNodeColor = FColor::Black;
 					break;
				default:
					GridNodeColor = SpaceColor[GridNode.mGridSpaceIndex % SpaceColorNum];
					break;
				}

// 				for (const FAIAssist_DungeonGridNode* ConnectRoadGridNode : mCheckConnectRoadXGridNodeList)
// 				{
// 					if (&GridNode == ConnectRoadGridNode)
// 					{
// 						GridNodeColor = FColor::White;
// 						break;
// 					}
// 				}

				FVector2D CenterPos(static_cast<float>(x)*GridDispLenX, static_cast<float>(y) * GridDispLenY);
				CenterPos += GridDispExtent;
				CenterPos += GridDispBasePos;
				DrawCanvasQuadrangle(InCanvas, CenterPos, GridDispExtent, GridNodeColor);
				++Index;
			}
		}
	}
}

void AAIAssist_DungeonMaker::DebugDraw3DMap(UCanvas* InCanvas)
{
	const int32 SpaceColorNum = 8;
	const FColor SpaceColor[SpaceColorNum] = {
		FColor::Red,
		FColor::Blue,
		FColor::Yellow,
		FColor::Magenta,
		FColor::Orange,
		FColor::Purple,
		FColor::Turquoise,
		FColor::Cyan,
	};

	const FVector CenterPos = GetActorLocation();
	const FVector ExtentV = FVector(static_cast<float>(mGridNodeXNum), static_cast<float>(mGridNodeYNum), 0.f) * mGridNodeLength * 0.5f;
	const FVector BeginEdgePos = CenterPos - ExtentV;
	const FVector GridCenterOffsetV = FVector(mGridNodeLength, mGridNodeLength, 0.f) * 0.5f;
	const FVector GridDrawExtentV = FVector(mGridNodeLength, mGridNodeLength, mGridNodeLength) * 0.5f - FVector(3.f,3.f,3.f);
	for (const FAIAssist_DungeonGridNode& GridNode : mGridNodeList)
	{
		int32 GridIndexX = INDEX_NONE;
		int32 GridIndexY = INDEX_NONE;
		if (!CalcGridNodeXY(GridIndexX, GridIndexY, GridNode.mUID))
		{
			continue;
		}

		const FVector GridEdgePos = BeginEdgePos
			+ FVector(static_cast<float>(GridIndexX) * mGridNodeLength, static_cast<float>(GridIndexY) * mGridNodeLength, 0.f);
		const FVector GridCenterPos = GridEdgePos + GridCenterOffsetV;

		FColor GridNodeColor = FColor::White;
		switch (GridNode.mState)
		{
		case EAIAssist_DungeonGridState::Space:
			GridNodeColor = FColor::Silver;
			break;
		case EAIAssist_DungeonGridState::Road:
			GridNodeColor = FColor::Black;
			break;
		default:
			GridNodeColor = SpaceColor[GridNode.mGridSpaceIndex % SpaceColorNum];
			break;
		}
		DrawDebugBox(GetWorld(), GridCenterPos, GridDrawExtentV, GridNodeColor, false, -1.f, 0, 2.f);
	}
}

void	AAIAssist_DungeonMaker::DrawCanvasQuadrangle(UCanvas* InCanvas, const FVector2D& InCenterPos, const FVector2D& InExtent, const FLinearColor InColor)
{
	const uint32 PointListSize = 4;
	const FVector2D PointList[PointListSize] = {
		FVector2D(InCenterPos.X - InExtent.X, InCenterPos.Y + InExtent.Y),//左上
		FVector2D(InCenterPos.X - InExtent.X, InCenterPos.Y - InExtent.Y),//左下
		FVector2D(InCenterPos.X + InExtent.X, InCenterPos.Y - InExtent.Y),//右下
		FVector2D(InCenterPos.X + InExtent.X, InCenterPos.Y + InExtent.Y)//右上
	};
	for (uint32 i = 0; i < PointListSize; ++i)
	{
		DrawDebugCanvas2DLine(InCanvas, PointList[i], PointList[(i + 1) % PointListSize], InColor);
	}
}
#endif

void AAIAssist_DungeonMaker::EditorClearDungeon()
{
// #if WITH_EDITOR
// 	mEditorStepBeginIndex = FAIAssist_DungeonGridNodeIndex(0, 0);
// 	mEditorStepEndIndex = FAIAssist_DungeonGridNodeIndex(mGridNodeXNum - 1, mGridNodeYNum - 1);
// 	GenerateGridNodeList();
// #endif
}

void AAIAssist_DungeonMaker::EditorGenerateDungeonStep()
{
// #if WITH_EDITOR
// 	GenerateDungeon_CutGridNode(mEditorStepBeginIndex, mEditorStepEndIndex);
// #endif
}

void AAIAssist_DungeonMaker::EditorSwitchDraw2DGrid()
{
#if UE_BUILD_DEVELOPMENT
	mbDebugDraw2DGrid = !mbDebugDraw2DGrid;
#endif
}

void AAIAssist_DungeonMaker::EditorSwitchDraw2DGridSpace()
{
#if UE_BUILD_DEVELOPMENT
	mbDebugDraw2DGridSpace = !mbDebugDraw2DGridSpace;
#endif
}

void AAIAssist_DungeonMaker::EditorSwitchDraw3DGrid()
{
#if UE_BUILD_DEVELOPMENT
	mbDebugDraw3DGrid = !mbDebugDraw3DGrid;
#endif
}