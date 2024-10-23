// SparseOctree
// AAIAssist_OctreeÇåpè≥ÇµÇƒégóp
// OctreeÇ…éùÇΩÇπÇΩÇ¢èÓïÒÇÕ FAIAssist_OctreeData Çåpè≥ÇµÇƒàµÇ§
// AAIAssist_Octreeåpè≥ÉNÉâÉXÇ≈Å™ÇÃÉäÉXÉgÇï€éùÇ∑ÇÈ
// GetDataListNum(), GetData() Ç≈éQè∆Ç≈Ç´ÇÈÇÊÇ§Ç…Ç∑ÇÈ
// EditorCreateOctreeData() Ç≈éñëOÇ…OctreeÇ…éùÇΩÇπÇÈèÓïÒÇçÏê¨
// ÉQÅ[ÉÄãNìÆå„Ç…BuildOctree()Ç≈OctreeÇç\ê¨

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AIAssist_Octree.generated.h"

class UCanvas;

//OctreeÇç\ê¨Ç∑ÇÈÉmÅ[Éh(Voxel)
struct AIASSIST_API FAIAssist_OctreeNode
{
	static const int32 msChildCount = 8;

	TArray<FAIAssist_OctreeNode*> mChildNodeList;
	int32 mDataListIndex = 0;

	FAIAssist_OctreeNode();
	~FAIAssist_OctreeNode();
	
	FAIAssist_OctreeNode* GetChild(const int32 InIndex) const;
	void CreateChild(const int32 InIndex);
	bool IsLeaf() const;
};

//OctreeÇ≈àµÇ¢ÇΩÇ¢Data
USTRUCT(BlueprintType)
struct AIASSIST_API FAIAssist_OctreeData
{
	GENERATED_BODY()

	UPROPERTY()
	uint32	mNodeIndex = 0;
};

UCLASS(Abstract, BlueprintType)
class AIASSIST_API AAIAssist_Octree : public AActor
{
	GENERATED_BODY()

	typedef uint32 tNodeIndex;//äKëwÇ≤Ç∆Ç…3bitÇ∏Ç¬äiî[Ç≥ÇÍÇΩIndex(10äKëwÇ™å¿äE)

public:
	AAIAssist_Octree(const FObjectInitializer& ObjectInitializer);

	//virtual void Tick(float DeltaTime) override;

	const FAIAssist_OctreeNode* GetNode(const int32 InX, const int InY, const int InZ, const int32 InTargetDepth) const;
	const FAIAssist_OctreeNode* Intersect(const FVector& InBasePos, const FVector& InTargetPos) const;

protected:
	virtual void PostInitProperties() override;
	virtual void PostLoad() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void BeginDestroy() override;

	virtual int32 GetDataListNum(){return 0;}
	virtual const FAIAssist_OctreeData* GetData(const int32 InIndex) const {return nullptr;}
	
	void BuildOctree();
	FAIAssist_OctreeNode* Insert(const int32 InX, const int32 InY, const int32 InZ);
	int32 GetChildIndex(const int32 InX, const int32 InY, const int32 InZ, const int32 InDepth) const;
	FVector CalcNodeCenter(const int32 InX, const int32 InY, const int32 InZ, const int32 InDepth) const;
	tNodeIndex CalcNodeIndex(const FVector& InPos, const int32 InDepth) const;
	tNodeIndex CalcNodeIndex(const int32 InX, const int32 InY, const int32 InZ, const int32 InDepth) const;
	bool CalcNodeIndex(int32& OutX, int32& OutY, int32& OutZ, const int32 InDepth, const tNodeIndex InIndex) const;
	const FAIAssist_OctreeNode* GetNodeFromIndex(const tNodeIndex InNodendex, const int32 InDepth) const;
	float CalcVoxelExtentLength(const int32 InDepth) const;
	int32 CalcVoxelEdgeNum(const int32 InDepth) const;
	int32 GetMaxDepth() const{return mMaxDepth;}

	struct FRay
	{
		FVector mOrigin;   // ÉåÉCÇÃå¥ì_
		FVector mDirection; // ÉåÉCÇÃï˚å¸
	};
	const FAIAssist_OctreeNode* RayCastIterative(const FRay& InRay, const float InMin, const float InMax) const;
	bool RayIntersectsAABB(const FRay& InRay, const FVector& InVoxelCenterPos, const float InVoxelSize, float InOutMin=0.f, float InOutMax=FLT_MAX) const;

protected:
	FAIAssist_OctreeNode* mRoot = nullptr;
	//UPROPERTY()
	TArray<FAIAssist_OctreeNode> mNodeList;
	UPROPERTY(EditInstanceOnly, Category = AAIAssist_Octree)
	FVector mAroundBoxExtentV = FVector(100.f,100.f,100.f);
	UPROPERTY(EditInstanceOnly, Category = AAIAssist_Octree)
	float mRootLength = 500.f;
	UPROPERTY(EditInstanceOnly, Category = AAIAssist_Octree, meta = (ClampMin = 1, ClampMax = 10))
	int32 mMaxDepth = 4;

#if USE_CSDEBUG
public:
	void DebugSetDrawInfo(const bool bInDraw) { mbDebugDrawInfo = bInDraw; }
	void DebugSetDrawAroundBox(const bool bInDraw) { mbDebugDrawAroundBox = bInDraw; }
	void DebugSetDrawActiveVoxel(const bool bInDraw) { mbDebugDrawActiveVoxel = bInDraw; }

protected:
	void DeubgRequestDraw(const bool bInActive);
	void DebugDraw(UCanvas* InCanvas, class APlayerController* InPlayerController);
	void DebugDrawInfo(UCanvas* InCanvas);
	void DebugDrawAroundBox();
	void DebugDrawActiveVoxel(UCanvas* InCanvas, const int32 InDepth) const;
	void DebugDrawVoxelDetail(UCanvas* InCanvas, const int32 InX, const int32 InY, const int32 InZ) const;
protected:
	FDelegateHandle	mDebugDrawHandle;
	bool mbDebugDrawInfo = false;
	bool mbDebugDrawAroundBox = false;
	bool mbDebugDrawActiveVoxel = false;
#endif

#if WITH_EDITOR
public:
	UFUNCTION(CallInEditor, Category = AAIAssist_Octree_Editor)
	void EditorTestCreate();

	//åpè≥êÊÇ≈éñëOÇ…FAIAssist_OctreeDataÇópà”
	virtual void EditorCreateOctreeData(){}

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
protected:
	void EditorUpdateAroundBox();
	void EditorDrawTestRayCast();
	void EditorDebugDraw(UCanvas* InCanvas);
#endif
#if WITH_EDITORONLY_DATA
protected:
	UPROPERTY(Transient)
	class UBoxComponent* mEditorVoxelBox = nullptr;
	UPROPERTY(EditInstanceOnly, Category = AAIAssist_Octree_Editor, meta = (DisplayPriority = 2, EditCondition = "mbEditorTestRayCast"))
	FVector mEditorTestRayCastBasePos = FVector::ZeroVector;
	UPROPERTY(EditInstanceOnly, Category = AAIAssist_Octree_Editor, meta = (DisplayPriority = 2, EditCondition = "mbEditorTestRayCast"))
	FVector mEditorTestRayCastTargetPos = FVector(500.f,500.f,500.f);
	UPROPERTY(EditInstanceOnly, Category = AAIAssist_Octree_Editor, meta = (DisplayPriority = 2))
	int32 mEditorDrawVoxelIndexX = 0;
	UPROPERTY(EditInstanceOnly, Category = AAIAssist_Octree_Editor, meta = (DisplayPriority = 2))
	int32 mEditorDrawVoxelIndexY = 0;
	UPROPERTY(EditInstanceOnly, Category = AAIAssist_Octree_Editor, meta = (DisplayPriority = 2))
	int32 mEditorDrawVoxelIndexZ = 0;
	UPROPERTY(EditInstanceOnly, Category = AAIAssist_Octree_Editor, meta = (DisplayPriority = 2))
	bool mbEditorDrawActiveVoxel = false;
	UPROPERTY(EditInstanceOnly, Category = AAIAssist_Octree_Editor, meta = (DisplayPriority = 2))
	bool mbEditorDrawVoxelDetail = false;
	UPROPERTY(EditInstanceOnly, Category = AAIAssist_Octree_Editor, meta = (InlineEditConditionToggle))
	bool mbEditorTestRayCast = false;
private:
#endif
};