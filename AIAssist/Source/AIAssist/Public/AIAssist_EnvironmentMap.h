// AIが地形認知するためのデータ解析＆提供(遮蔽の事前計算なし版)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AIAssist_EnvironmentMap.generated.h"

class UCanvas;
class ATargetPoint;
struct FCSDebugInfoWindowText;

//VoxelのIndexを24bitでUIDとX,Y,Zで表現
//IndexXYZからUIDを求めるのは掛け算よりちょい遅いが
//UIDからIndexXYZを求めるのは3倍ぐらい速い
union FAIAssist_EnvironmentMapVoxelIndex
{
	static constexpr int32 smVoxelLineMax = 256;//x,y,zそれぞれで8bitで24bit制限
	int32	mUID = 0;
	struct IndexXYZ
	{
		int32	mX : 8;
		int32	mY : 8;
		int32	mZ : 8;
		int32 : 6;
	};
	IndexXYZ	mIndex;

	FAIAssist_EnvironmentMapVoxelIndex()
	{}
	FAIAssist_EnvironmentMapVoxelIndex(const int32 InUID)
	{
		check(InUID <= 16777216)//2^24
		mUID = InUID;
	}
	FAIAssist_EnvironmentMapVoxelIndex(const int32 InX, const int32 InY, const int32 InZ)
	{
		check(InX <= smVoxelLineMax && InY <= smVoxelLineMax && InZ <= smVoxelLineMax);
		mIndex.mX = InX;
		mIndex.mY = InY;
		mIndex.mZ = InZ;
	}
};

//Voxelが持つ情報
USTRUCT(BlueprintType)
struct AIASSIST_API FAIAssist_EnvironmentMapVoxel
{
	GENERATED_BODY()

	//FAIAssist_EnvironmentMapVoxelIndexとして扱うIndex(UID)
	UPROPERTY()
	int32	mVoxelIndex = INDEX_NONE;//24bitだけ使用中
	//隣接するVoxelへの遮蔽判定
	UPROPERTY()
	int32	mAroundVoxelCurtainBit = 0;//27Bit(3*3*3)だけ使用中
	//一番高いコリジョン一をVoxelの下から正規化して、FAIAssist_CompressOneFloatで圧縮した値
	UPROPERTY()
	uint8	mGroundHeightRatio_Compress = 0;
	UPROPERTY()
	uint8	mbTouchGround : 1;

	FAIAssist_EnvironmentMapVoxel()
		: mbTouchGround(false)
	{}
};

#if WITH_EDITOR
//Voxel情報生成の非同期処理用
class FAIAssist_EnvironmentMapAsyncTask : public FNonAbandonableTask
{
	friend class FAsyncTask<FAIAssist_EnvironmentMapAsyncTask>;
public:
	FAIAssist_EnvironmentMapAsyncTask(TFunction<void()> InWork)
		: mWork(InWork)
	{
	}

	void DoWork()
	{
		mWork();
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FAsyncExecTask, STATGROUP_ThreadPoolAsyncTasks);
	}

private:
	TFunction<void()> mWork;
};
#endif

UCLASS(Abstract, BlueprintType)
class AIASSIST_API AAIAssist_EnvironmentMap : public AActor
{
	GENERATED_BODY()

public:
	AAIAssist_EnvironmentMap(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void PostInitProperties() override;
	virtual void PostLoad() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void BeginDestroy() override;

public:
	virtual void Tick(float DeltaTime) override;

protected:
	void	SetupParam();
	void	SetupVoxelMap();
	FVector	CalcAroundBoxExtentV() const;
	FVector	CalcAroundBoxCenterPos() const;
	FVector	CalcAroundBoxBeginPos() const;
	FVector	CalcVoxelCenterPos(const int32 InX, const int32 InY, const int32 InZ) const;
	int32	CalcVoxelUID(const int32 InX, const int32 InY, const int32 InZ) const;
	int32	CalcVoxelUID(const FVector& InPos) const;
	bool	CalcVoxelIndexXYZ(int32& OutX, int32& OutY, int32& OutZ, const int32 InUID) const;
	bool	CalcVoxelIndexXYZ(int32& OutX, int32& OutY, int32& OutZ, const FVector& InPos) const;
	bool	IsOwnVoxel(const int32 InX, const int32 InY, const int32 InZ) const;
	bool	IsOwnVoxel(const int32 InUID) const;
	bool	CheckCurtain(const int32 InBaseIndex, const int32 InTargetIndex) const;

protected:
	UPROPERTY()
	TArray<FAIAssist_EnvironmentMapVoxel>	mActiveVoxelList;
	UPROPERTY(EditDefaultsOnly, Category = AIAssist_EnvironmentMap, meta = (ClampMin = "1.0", UIMin = "1.0"))
	float	mVoxelLength = 200.f;
	UPROPERTY(EditInstanceOnly, Category = AIAssist_EnvironmentMap, meta = (ClampMin = "1", UIMin = "1", ClampMax = "256", UIMax = "256"))
	int32	mVoxelXNum = 10;
	UPROPERTY(EditInstanceOnly, Category = AIAssist_EnvironmentMap, meta = (ClampMin = "1", UIMin = "1", ClampMax = "256", UIMax = "256"))
	int32	mVoxelYNum = 10;
	UPROPERTY(EditInstanceOnly, Category = AIAssist_EnvironmentMap, meta = (ClampMin = "1", UIMin = "1", ClampMax = "256", UIMax = "256"))
	int32	mVoxelZNum = 3;

private:
	TMap<int32, FAIAssist_EnvironmentMapVoxel*>	mVoxelMap;//UIDをキーにしたMap
	FVector	mAroundBoxCenterPos = FVector::ZeroVector;
	FVector mAroundBoxExtentV = FVector::OneVector;
	FVector mAroundBoxBeginPos = FVector::ZeroVector;

#if UE_BUILD_DEVELOPMENT
public:
	void	DebugSetSelectVoxelIndex(const int32 InIndex);
	void	DebugSetSelectVoxelIndex(const int32 InX, const int32 InY, const int32 InZ);
	void	DebugSetDispInfo(const bool bInDisp){ DeubgRequestDraw(true); mbDebugDispInfo = bInDisp;}
	void	DebugSetDispAroundBox(const bool bInDisp){mbDebugDispAroundBox = bInDisp;}
	void	DebugSetDispVoxelList(const bool bInDisp){mbDebugDispVoxelList = bInDisp;}
	void	DebugSetDispDetailSelectVoxel(const bool bInDisp) { mbDebugDispDetailSelectVoxel = bInDisp; }
	void	DebugSetDispDetailNearCamera(const bool bInDisp) { mbDebugDispDetailNearCamera = bInDisp; }
protected:
	void	DeubgRequestDraw(const bool bInActive);
	void	DebugDraw(UCanvas* InCanvas, class APlayerController* InPlayerController);
	void	DebugDrawInfo(UCanvas* InCanvas);
	void	DebugDrawAroundBox();
	void	DebugDrawVoxelList(UCanvas* InCanvas);
	void	DebugDrawPlayerVisibleVoxel();
protected:
	FDelegateHandle	mDebugDrawHandle;
	int32	mDebugSelectVoxelIndex = INDEX_NONE;
	bool	mbDebugDispInfo = false;
	bool	mbDebugDispAroundBox = false;
	bool	mbDebugDispVoxelList = false;
	bool	mbDebugDispDetailSelectVoxel = false;
	bool	mbDebugDispDetailNearCamera = false;
	bool	mbDebugDispPlayerVisibleVoxel = false;
#endif

#if WITH_EDITOR
public:
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;

	void	EditorRequestCreateAIAssist_EnvironmentMapData()
	{
		EditorCreateAIAssist_EnvironmentMapData();
	}
	void	EditorSetCollectCheckVisibleBaseIndex(const int32 InIndex)
	{
		mEditorCollectCheckVisibleBaseIndex = InIndex;
	}
	void	EditorSetCollectCheckVisibleTargetIndex(const int32 InIndex)
	{
		mEditorCollectCheckVisibleTargetIndex = InIndex;
	}
	void	EditorSetCreateOnlySelectVoxel(const bool bInOnly)
	{
		mbEditorCreateOnlySelectVoxel = bInOnly;
	}
	void	EditorSetTestCurtain(const bool bInTest)
	{
		mbEditorTestCurtain = bInTest;
	}
	void	EditorSetTestPerformance(const bool bInTest)
	{
		mbEditorTestPerformance = bInTest;
	}
protected:
	void	RunEditorTick(const bool bInRun);
	bool	EditorTick(float InDeltaSecond);
	UFUNCTION(CallInEditor, Category = AIAssist_EnvironmentMapDebug)
	void	EditorCreateAIAssist_EnvironmentMapData();
	void	EditorCancelCreateMapData();
	void	EditorCreateVoxelList();
	void	EditorCreateVoxel(const int32 InX, const int32 InY, const int32 InZ);
	bool	EditorIsCuartain(const FVector& InBaseVoxelPos, const FVector& InTargetVoxelPos) const;
	void	EditorUpdateAroundBox();
	void	EditorTestCheckCurtain(UCanvas* InCanvas);
	void	EditorTestPerformance(FCSDebugInfoWindowText& OutInfoWindow) const;
#endif
#if WITH_EDITORONLY_DATA
protected:
	UPROPERTY(EditInstanceOnly, Category = AIAssist_EnvironmentMapDebug)
	ATargetPoint*	mEditorTestBasePoint = nullptr;
	UPROPERTY(EditInstanceOnly, Category = AIAssist_EnvironmentMapDebug)
	ATargetPoint*	mEditorTestTargetPoint = nullptr;
	UPROPERTY(Transient)
	class UBoxComponent* mEditorVoxelBox = nullptr;
private:
	struct FEditorCheckVisibleLog
	{
		FVector	mBasePos = FVector::ZeroVector;
		FVector	mTargetPos = FVector::ZeroVector;
		bool	mbHit = false;
	};
	FDelegateHandle	mEditorTickHandle;
	TFunction<void()> mEditorAsyncFunc;
	FAsyncTask<FAIAssist_EnvironmentMapAsyncTask>* mEditorAsyncTask;
	TSharedPtr<SNotificationItem> mEditorNotificationItem;
	mutable TArray<FHitResult>	mEditorCreateVoxelCheckCollisionLog;
	FString	mEditorAsyncTaskState;
	int32	mEditorCollectCheckVisibleBaseIndex = INDEX_NONE;
	int32	mEditorCollectCheckVisibleTargetIndex = INDEX_NONE;
	bool	mbEditorRequestCancelAsyncTask = false;
	bool	mbEditorCreateOnlySelectVoxel = false;
	bool	mbEditorTestCurtain = false;
	bool	mbEditorTestPerformance = false;
#endif
};