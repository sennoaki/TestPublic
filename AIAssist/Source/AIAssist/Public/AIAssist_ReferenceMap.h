// AIが地形認知するためのデータ解析＆提供

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Async/AsyncWork.h"
#include "AIAssist_ReferenceMap.generated.h"

#if WITH_EDITOR
	#define AIASSIST_DEBUGVOXEL 0
#else
	#define AIASSIST_DEBUGVOXEL 0
#endif

class UCanvas;

//Voxelが持つ情報
USTRUCT(BlueprintType)
struct AIASSIST_API FAIAssist_ReferenceMapVoxel
{
	GENERATED_BODY()

	//Visible判定範囲内の有効Voxel列の可視判定結果のbit配列だったり、Indexを直接格納してたり
	UPROPERTY()
	TArray<int32>	mArroundVoxelVisibleUtilList;
	UPROPERTY()
	int32	mListIndex = INDEX_NONE;
	//一番高いコリジョン一をVoxelの下から正規化して、FAIAssist_CompressOneFloatで圧縮した値
	UPROPERTY()
	uint8	mGroundHeightRatio_Compress = 0;
	UPROPERTY()
	uint8	mbTouchGround : 1;//Navmeshにすべき?
	//mArroundVoxelVisibleUtilListを見えてるリストとして扱う
	UPROPERTY()
	uint8	mbUseVisbleBit : 1;
	//mArroundVoxelVisibleUtilListを見えてるリストとして扱う
	UPROPERTY()
	uint8	mbUseVisbleIndexList : 1;
	//mArroundVoxelVisibleUtilListを見えてないリストとして扱う
	UPROPERTY()
	uint8	mbUseReverseVisbleIndexList : 1;

	TMap<int32,bool>	mArroundVoxelVisibleMap;

#if AIASSIST_DEBUGVOXEL
	TArray<FVector> mEditorGroundPosList;
#endif

	FAIAssist_ReferenceMapVoxel()
		:mbTouchGround(false)
		, mbUseVisbleBit(false)
		, mbUseVisbleIndexList(false)
		, mbUseReverseVisbleIndexList(false)
	{}
};

#if WITH_EDITOR
//Voxel情報生成の非同期処理用
class FAIAssist_ReferenceMapAsyncTask : public FNonAbandonableTask
{
	friend class FAsyncTask<FAIAssist_ReferenceMapAsyncTask>;
public:
	FAIAssist_ReferenceMapAsyncTask(TFunction<void()> InWork)
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
class AIASSIST_API AAIAssist_ReferenceMap : public AActor
{
	GENERATED_BODY()

	static constexpr int32 smInvalidIndex = -1;

public:
	AAIAssist_ReferenceMap(const FObjectInitializer& ObjectInitializer);

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
	FVector	CalcArroundBoxExtentV() const;
	FVector	CalcArroundBoxCenterPos() const;
	FVector	CalcArroundBoxBeginPos() const;
	FVector	CalcVoxelCenterPos(const int32 InX, const int32 InY, const int32 InZ) const;
	int32	CalcVoxelListIndex(const int32 InX, const int32 InY, const int32 InZ) const;
	int32	CalcVoxelListIndex(const FVector& InPos) const;
	void	CalcVoxelXYZIndex(int32& OutX, int32& OutY, int32& OutZ, const int32 InListIndex) const;
	void	CalcVoxelXYZIndex(int32& OutX, int32& OutY, int32& OutZ, const FVector& InPos) const;
	bool	IsOwnVoxelList(const int32 InX, const int32 InY, const int32 InZ) const;
	bool	IsOwnVoxelList(const int32 InListIndex) const;
	void	CollectVisibleCheckIndexList(TArray<int32>& OutList, const int32 InIndex) const;
	bool	CheckVisibleVoxel(const int32 InBaseIndex, const int32 InTargetIndex) const;

protected:
	UPROPERTY()
	TArray<FAIAssist_ReferenceMapVoxel>	mActiveVoxelList;
	UPROPERTY(EditDefaultsOnly, Category = AIAssist_ReferenceMap, meta = (ClampMin = "1.0", UIMin = "1.0"))
	float	mVoxelLength = 200.f;
	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category = AIAssist_ReferenceMap, meta = (ClampMin = "1.0", UIMin = "1.0"))
	float	mCheckVisibleDistance = 3000.f;
	UPROPERTY(EditInstanceOnly, Category = AIAssist_ReferenceMap, meta = (ClampMin = "1", UIMin = "1"))
	int32	mVoxelXNum = 10;
	UPROPERTY(EditInstanceOnly, Category = AIAssist_ReferenceMap, meta = (ClampMin = "1", UIMin = "1"))
	int32	mVoxelYNum = 10;
	UPROPERTY(EditInstanceOnly, Category = AIAssist_ReferenceMap, meta = (ClampMin = "1", UIMin = "1"))
	int32	mVoxelZNum = 3;

private:
	TMap<int32, FAIAssist_ReferenceMapVoxel*>	mVoxelMap;
	FVector	mArroundBoxCenterPos = FVector::ZeroVector;
	FVector mArroundBoxExtentV = FVector::OneVector;
	FVector mArroundBoxBeginPos = FVector::ZeroVector;
	int32	mVisbleCheckVoxelNum = 0;

#if UE_BUILD_DEVELOPMENT
public:
	void	DebugSetSelectVoxelIndex(const int32 InIndex);
	void	DebugSetSelectVoxelIndex(const int32 InX, const int32 InY, const int32 InZ);
	void	DebugSetDispInfo(const bool bInDisp){ mbDebugDispInfo = bInDisp;}
	void	DebugSetDispArroundBox(const bool bInDisp){mbDebugDispArroundBox = bInDisp;}
	void	DebugSetDispVoxelList(const bool bInDisp){mbDebugDispVoxelList = bInDisp;}
	void	DebugSetDispDetailSelectVoxel(const bool bInDisp){ mbDebugDispDetailSelectVoxel = bInDisp;}
protected:
	void	DeubgRequestDraw(const bool bInActive);
	void	DebugDraw(UCanvas* InCanvas, class APlayerController* InPlayerController);
	void	DebugDrawInfo(UCanvas* InCanvas);
	void	DebugDrawArroundBox();
	void	DebugDrawVoxelList(UCanvas* InCanvas);
	void	DebugDrawPlayerVisibleVoxel();
	void	DebugTestPerformance();
protected:
	FDelegateHandle	mDebugDrawHandle;
	int32	mDebugSelectVoxelIndex = -1;
	bool	mbDebugDispInfo = false;
	bool	mbDebugDispArroundBox = false;
	bool	mbDebugDispVoxelList = false;
	bool	mbDebugDispDetailSelectVoxel = false;
	bool	mbDebugDispPlayerVisibleVoxel = false;
#endif

#if WITH_EDITOR
public:
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;

	void	EditorRequestCreateAIAssist_ReferenceMapData()
	{
		EditorCreateAIAssist_ReferenceMapData();
	}
	void	EditorSetCollectCheckVisibleBaseIndex(const int32 InIndex)
	{
		mEditorCollectCheckVisibleBaseIndex = InIndex;
	}
	void	EditorSetCollectCheckVisibleTargetIndex(const int32 InIndex)
	{
		mEditorCollectCheckVisibleTargetIndex = InIndex;
	}
protected:
	void	RunEditorTick(const bool bInRun);
	bool	EditorTick(float InDeltaSecond);
	UFUNCTION(CallInEditor, Category = AIAssist_ReferenceMapDebug)
	void	EditorCreateAIAssist_ReferenceMapData();
	void	EditorCancelCreateMapData();
	void	EditorCreateVoxelList();
	void	EditorCreateVoxelTouchGround(const int32 InX, const int32 InY, const int32 InZ);
	void	EditorCreateVoxelGroundUp();
	void	EditorCreateVoxelGroundUp(const FAIAssist_ReferenceMapVoxel& InGroundVoxel);
	void	EditorCheckVoxelVisibility();
	void	EditorCheckVoxelVisibility(FAIAssist_ReferenceMapVoxel& OutVoxel);
	void	EditorUpdateArroundBox();
#endif
#if WITH_EDITORONLY_DATA
protected:
	//Voxel生成する地面からの高さ
	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category = AIAssist_ReferenceMap)
	float	mEditorCheckCreateVoxelGroundUpHeight = 400.f;
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
	FAsyncTask<FAIAssist_ReferenceMapAsyncTask>* mEditorAsyncTask;
	TArray<FEditorCheckVisibleLog>	mEditorCheckVisibleLogList;
	TSharedPtr<SNotificationItem> mEditorNotificationItem;
	FString	mEditorAsyncTaskState;
	int32	mEditorCollectCheckVisibleBaseIndex = smInvalidIndex;
	int32	mEditorCollectCheckVisibleTargetIndex = smInvalidIndex;
	bool	mbEditorRequestCancelAsyncTask = false;
#endif
};