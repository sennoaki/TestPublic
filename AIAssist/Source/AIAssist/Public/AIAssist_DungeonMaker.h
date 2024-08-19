// 複数の部屋とそれをつなぐ道をランダムで生成する

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AIAssist_DungeonMaker.generated.h"

enum class EAIAssist_DungeonGridState : uint8
{
	Invalid,
	Space,
	Room,
	Road,
};
struct FAIAssist_DungeonGridNode
{
	int32 mUID = INDEX_NONE;
	int32 mGridSpaceIndex = INDEX_NONE;
	int32 mRoadIndex = INDEX_NONE;
	EAIAssist_DungeonGridState mState = EAIAssist_DungeonGridState::Invalid;
	union WallFlagUnion {
		uint8 mFlag = 0;
		struct {
			uint8 mbFront : 1;
			uint8 mbBack : 1;
			uint8 mbRight : 1;
			uint8 mbLeft : 1;
		};
	};
	WallFlagUnion mWallFlag;
};
struct FAIAssist_DungeonGridNodeIndex
{
	int32 mX = 0;
	int32 mY = 0;

	FAIAssist_DungeonGridNodeIndex(){}
	FAIAssist_DungeonGridNodeIndex(const int32 InX, const int32 InY)
		:mX(InX)
		, mY(InY)
	{}
};
struct FAIAssist_DungeonGridSpace
{
	FAIAssist_DungeonGridNodeIndex mBeginIndex;
	FAIAssist_DungeonGridNodeIndex mEndIndex;
	int32 mIndex = INDEX_NONE;

	FAIAssist_DungeonGridSpace(){}
	FAIAssist_DungeonGridSpace(const FAIAssist_DungeonGridNodeIndex& InBegin, const FAIAssist_DungeonGridNodeIndex& InEnd)
	:mBeginIndex(InBegin)
	,mEndIndex(InEnd)
	{}
};
struct FAIAssist_DungeonRoom
{
	FAIAssist_DungeonGridNodeIndex mBeginIndex;
	FAIAssist_DungeonGridNodeIndex mEndIndex;
	int32 mGridSpaceIndex = INDEX_NONE;
	TArray<int32> mConnectRoadGridNodeUIDList;

	FAIAssist_DungeonRoom() {}
	FAIAssist_DungeonRoom(const FAIAssist_DungeonGridNodeIndex& InBegin, const FAIAssist_DungeonGridNodeIndex& InEnd)
		:mBeginIndex(InBegin)
		,mEndIndex(InEnd)
	{}
};
struct FAIAssist_DungeonRoad
{
	TArray<int32> mGridNodeUIDList;
};

UCLASS(BlueprintType)
class AIASSIST_API AAIAssist_DungeonMaker : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAIAssist_DungeonMaker();
	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void GenerateDungeon();
	void GenerateGridNodeList();
	void GenerateGridSpace();
	void AssignGridSpaceIndex();
	void GenerateRoom();
	void GenerateRoom(const FAIAssist_DungeonGridSpace& InGridSpace);
	void AssignGridRoom();
	void SetupRoadGridSpace();
	void SetupRoadGridSpace(FAIAssist_DungeonRoom& InRoom);
	void SetupRoadGridSpaceX(FAIAssist_DungeonRoom& InRoom, const bool bInPlusDirection);
	void SetupRoadGridSpaceY(FAIAssist_DungeonRoom& InRoom, const bool bInPlusDirection);
	void SetupRoadConnect();
	void SetupRoadConnectDirectionX(const FAIAssist_DungeonGridNode& InBaseGridNode, const int32 InOffsetIndex);
	void SetupRoadConnectDirectionY(const FAIAssist_DungeonGridNode& InBaseGridNode, const int32 InOffsetIndex);
	void SetupRoadList();
	void SetupRoadList(const int32 InBaseGridNodeUID);
	void SetupGridWallFlag();
	void SetupGridWallFlag(FAIAssist_DungeonGridNode& InGridNode);
	void CreateStage();
	//void CreateWall(const FVector& InGridPos, );
	bool IsFloorGrid(const int32 InX, const int32 InY) const;
	bool CheckAddRoadList(FAIAssist_DungeonRoad& OutRoad, const int32 InCheckIndexX, const int32 InCheckIndexY);
	void CreateFloorActor(const FTransform& InTransform);
	void CreateWallActor(const FTransform& InTransform);

	int32 CalcGridNodeUID(const int32 InX, const int32 InY) const;
	bool CalcGridNodeXY(int32& OutX, int32& OutY, const int32 InUID) const;
	FAIAssist_DungeonGridNode* GetGridNode(const int32 InUID);
	FAIAssist_DungeonGridNode* GetGridNode(const int32 InX, const int32 InY);
	const FAIAssist_DungeonGridNode* GetGridNode(const int32 InX, const int32 InY) const;
	FVector CalcDungeonExtentV() const;
	FVector CalcDungeonBeginEdgePos() const;
	FVector CalcGridEdgePos(const int32 InX, const int32 InY) const;
	FVector CalcGridCenterPos(const int32 InX, const int32 InY) const;

public:	
	UPROPERTY(EditInstanceOnly, Category = AIAssist_DungeonMaker)
	float mGridNodeLength = 200.f;
	UPROPERTY(EditInstanceOnly, Category = AIAssist_DungeonMaker)
	float mWallLength = 400.f;
	//全体グリッド数x
	UPROPERTY(EditInstanceOnly, Category = AIAssist_DungeonMaker, meta = (ClampMin = "1", UIMin = "1", ClampMax = "256", UIMax = "256"))
	int32 mGridNodeXNum = 100;
	//全体グリッド数y
	UPROPERTY(EditInstanceOnly, Category = AIAssist_DungeonMaker, meta = (ClampMin = "1", UIMin = "1", ClampMax = "256", UIMax = "256"))
	int32 mGridNodeYNum = 100;
	//空間分割時の最小グリッド数
	UPROPERTY(EditInstanceOnly, Category = AIAssist_DungeonMaker, meta = (ClampMin = "1", UIMin = "1", ClampMax = "256", UIMax = "256"))
	int32 mGridSpaceLenMin = 6;
	//空間分割時の最大グリッド数
	UPROPERTY(EditInstanceOnly, Category = AIAssist_DungeonMaker, meta = (ClampMin = "1", UIMin = "1", ClampMax = "256", UIMax = "256"))
	int32 mGridSpaceLenMax = 20;
	//空間に部屋生成時の最小グリッド数
	UPROPERTY(EditInstanceOnly, Category = AIAssist_DungeonMaker, meta = (ClampMin = "1", UIMin = "1", ClampMax = "256", UIMax = "256"))
	int32 mGridRoomLenMin = 4;
	//空間に部屋生成時の空間境界から離す最小グリッド数
	UPROPERTY(EditInstanceOnly, Category = AIAssist_DungeonMaker, meta = (ClampMin = "1", UIMin = "1", ClampMax = "256", UIMax = "256"))
	int32 mGridRoomSpaceOffsetMin = 1;
	//基点が中心にある1辺が100cmの床用Actor
	UPROPERTY(EditInstanceOnly, Category = AIAssist_DungeonMaker)
	TSoftClassPtr<AActor> mFloorActorClass;
	//基点が中心にあるX軸に並行な面の1辺が100cmの壁用Actor
	UPROPERTY(EditInstanceOnly, Category = AIAssist_DungeonMaker)
	TSoftClassPtr<AActor> mWallActorClass;

private:
	UPROPERTY()
	TArray<AActor*> mStageActorList;

	TArray<FAIAssist_DungeonGridNode> mGridNodeList;
	TArray<FAIAssist_DungeonGridSpace> mGridSpaceList;
	TArray<FAIAssist_DungeonRoom> mRoomList;
	TArray<FAIAssist_DungeonRoad> mRoadList;
	TArray<FAIAssist_DungeonGridNode*> mCheckConnectRoadXGridNodeList;
	TArray<FAIAssist_DungeonGridNode*> mCheckConnectRoadYGridNodeList;

#if UE_BUILD_DEVELOPMENT
protected:
	void DeubgRequestDraw(const bool bInActive);
	void DebugDraw(UCanvas* InCanvas, class APlayerController* InPlayerController);
	void DebugDraw2DMap(UCanvas* InCanvas);
	void DebugDraw3DMap(UCanvas* InCanvas);
	void DebugDrawCanvasQuadrangle(UCanvas* InCanvas, const FVector2D& InCenterPos, const FVector2D& InExtent, const FLinearColor InColor);
	FString DebugGetGridStateString(const EAIAssist_DungeonGridState InState) const;
private:
	FDelegateHandle	mDebugDrawHandle;
	bool mbDebugDraw2DGrid = false;
	bool mbDebugDraw2DGridSpace = false;
	bool mbDebugDraw3DGrid = false;
#endif

public:
	UFUNCTION(BlueprintCallable, meta = (DevelopmentOnly))
	void EditorRepeatGenerate()
	{
		GenerateDungeon();
	}
	UFUNCTION(BlueprintCallable, meta = (DevelopmentOnly))
	void EditorClearDungeon();
	UFUNCTION(BlueprintCallable, meta = (DevelopmentOnly))
	void EditorGenerateDungeonStep();
	UFUNCTION(BlueprintCallable, meta = (DevelopmentOnly))
	void EditorSwitchDraw2DGrid();
	UFUNCTION(BlueprintCallable, meta = (DevelopmentOnly))
	void EditorSwitchDraw2DGridSpace();
	UFUNCTION(BlueprintCallable, meta = (DevelopmentOnly))
	void EditorSwitchDraw3DGrid();
};
