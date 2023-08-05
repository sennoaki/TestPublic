// Copyright 2022 SensyuGames.

#pragma once

#include "CoreMinimal.h"
#include "DataTableRowSelector.generated.h"

class UDataTable;

USTRUCT(BlueprintType)
struct FDataTableRowSelector
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "DataTableRowSelector")
	FName mRowName;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "DataTableRowSelector")
	TArray<FString> mEditorDataTablePathList;
// 	UPROPERTY(EditAnywhere, Category = "DataTableRowSelector")
// 	UStruct* mEditorDataTableStruct;
	UPROPERTY(EditAnywhere, Category = "DataTableRowSelector")
	FString mEditorDisplayName;
	UPROPERTY(EditAnywhere, Category = "DataTableRowSelector")
	TArray<UDataTable*> mEdtiorDataTableAssetList;
#endif

	FDataTableRowSelector() {}
	FDataTableRowSelector(const FString& InDataTablePath)
	{
		mEditorDataTablePathList.Add(InDataTablePath);
	}
};
