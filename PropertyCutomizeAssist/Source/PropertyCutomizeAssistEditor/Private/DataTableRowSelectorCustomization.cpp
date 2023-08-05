// Copyright 2022 SensyuGames.


#include "DataTableRowSelectorCustomization.h"
#include "PropertyEditing.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "DataTableRowSelector.h"

#define LOCTEXT_NAMESPACE "FPropertyCutomizeAssistEditor"

FDataTableRowSelectorCustomization::FDataTableRowSelectorCustomization()
{
}

void FDataTableRowSelectorCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle,
	class FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	mRowNameHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDataTableRowSelector, mRowName));
	mDataTableHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDataTableRowSelector, mEdtiorDataTableAssetList));
	
	if (mDataTableHandle.IsValid())
	{
		mDataTableHandle->SetOnChildPropertyValueChanged(
			FSimpleDelegate::CreateSP(this, &FDataTableRowSelectorCustomization::OnChangeSelectDataTable)
		);
	}

	SetupDisplayName(StructPropertyHandle);
	SetupRowNameList(StructPropertyHandle);
}

void FDataTableRowSelectorCustomization::CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle,
	class IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	if(mbUseDataTableProperty)
	{
		StructBuilder.AddProperty(mDataTableHandle.ToSharedRef());
	}

	if (mRowNameList.Num() <= 0)
	{
		return;
	}

	if (mPropertyTitleName.IsEmpty())
	{
		mPropertyTitleName = FString(TEXT("DataTableRowName"));
	}
	CheckActiveRowName();

	StructBuilder.AddCustomRow(LOCTEXT("TypeRow", "TypeRow"))
	.NameContent()
	[
		SNew(STextBlock)
		.Text(FText::Format(LOCTEXT("TypeTitle", "{0}"), FText::FromString(mPropertyTitleName)))
	]
	.ValueContent()
	[
		SAssignNew(mComboBox, SComboBox<TSharedPtr<FString>>)
		.OptionsSource(&mRowNameList)
		.InitiallySelectedItem(mRowNameList[GetSelectIndex()])
		.OnSelectionChanged(this, &FDataTableRowSelectorCustomization::OnSelectionChanged)
		.OnGenerateWidget(this, &FDataTableRowSelectorCustomization::OnGenerateWidget)
		[
			SNew(STextBlock)
			.Text(this, &FDataTableRowSelectorCustomization::GetSelectedTypeText)
		]
	];
}


void FDataTableRowSelectorCustomization::OnSelectionChanged(TSharedPtr<FString> Type, ESelectInfo::Type SelectionType)
{
	if (mRowNameHandle.IsValid()
		&& Type.IsValid())
	{
		mRowNameHandle->SetValue(*Type);
	}
}

TSharedRef<SWidget> FDataTableRowSelectorCustomization::OnGenerateWidget(TSharedPtr<FString> Type)
{
	return SNew(STextBlock).Text(FText::FromString(*Type));
}
FText FDataTableRowSelectorCustomization::GetSelectedTypeText() const
{
	TSharedPtr<FString> SelectedType = mComboBox->GetSelectedItem();
	if(SelectedType.IsValid())
	{
		return FText::FromString(*SelectedType);
	}
	return FText::FromString(TEXT("None"));
}
int32 FDataTableRowSelectorCustomization::GetSelectIndex() const
{
	if (!mRowNameHandle.IsValid())
	{
		return 0;
	}
	FName RowName;
	if (mRowNameHandle->GetValue(RowName) != FPropertyAccess::Success)
	{
		return 0;
	}

	for (int32 i = 0; i < mRowNameList.Num(); ++i)
	{
		if (*mRowNameList[i] == RowName.ToString())
		{
			return i;
		}
	}
	return 0;
}

void FDataTableRowSelectorCustomization::SetupDisplayName(TSharedRef<IPropertyHandle> StructPropertyHandle)
{
	const TSharedPtr<IPropertyHandle> DisplayNameHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDataTableRowSelector, mEditorDisplayName));
	if (DisplayNameHandle.IsValid())
	{
		FString DisplayName;
		if (DisplayNameHandle->GetValue(DisplayName) == FPropertyAccess::Success)
		{
			if (!DisplayName.IsEmpty())
			{
				mPropertyTitleName = DisplayName;
			}
		}
	}
}

void FDataTableRowSelectorCustomization::SetupRowNameList(TSharedRef<IPropertyHandle> StructPropertyHandle)
{
	TArray<FString> RowNameList;
	if (!AppendRowNameListByPath(RowNameList, StructPropertyHandle))
	{
		//if (!AppendRowNameListByStruct(RowNameList, StructPropertyHandle))
		{
			mbUseDataTableProperty = true;
			if (!AppendRowNameListByDataTable(RowNameList))
			{
				mRowNameList.Add(MakeShareable(new FString(TEXT("--None--"))));
				return;
			}
		}
	}

	RowNameList.Sort([](const FString& A, const FString& B) {
		return A < B;
		});

	mRowNameList.Add(MakeShareable(new FString(TEXT("--None--"))));
	for (const FString& RowName : RowNameList)
	{
		mRowNameList.Add(MakeShareable(new FString(RowName)));
	}
}

bool FDataTableRowSelectorCustomization::AppendRowNameListByPath(TArray<FString>& OutList, TSharedRef<IPropertyHandle> StructPropertyHandle)
{
	const TSharedPtr<IPropertyHandle> DataTablePathListHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDataTableRowSelector, mEditorDataTablePathList));
	if (!DataTablePathListHandle.IsValid())
	{
		return false;
	}

	TSharedPtr<IPropertyHandleArray> HandleArray = DataTablePathListHandle->AsArray();
	uint32 NumItem = 0;
	if (HandleArray->GetNumElements(NumItem) != FPropertyAccess::Success)
	{
		return false;
	}

	TArray<FString> RowNameList;
	for (uint32 i = 0; i < NumItem; ++i)
	{
		TSharedRef<IPropertyHandle> ElementHandle = HandleArray->GetElement(i);
		FString DataTablePath;
		if (ElementHandle->GetValue(DataTablePath) != FPropertyAccess::Success)
		{
			return false;
		}
		if (mPropertyTitleName.IsEmpty())
		{
			FString LeftString;
			DataTablePath.Split(TEXT("."), &LeftString, &mPropertyTitleName);
		}
		if (const UDataTable* DataTable = LoadObject<UDataTable>(nullptr, (*DataTablePath), nullptr, LOAD_None, nullptr))
		{
			for (const FName& RowName : DataTable->GetRowNames())
			{
				OutList.Add(RowName.ToString());
			}
		}
	}

	return OutList.Num() > 0;
}

//ìØÇ∂Moduleì‡Ç©ï ìrÉçÅ[ÉhçœÇ›ÇÃDataTableÇ∂Ç·Ç»Ç¢Ç∆UDataTableÇÃåüçıÇ≈Ç´Ç»Ç¢Ç¡Ç€Ç¢
// bool FDataTableRowSelectorCustomization::AppendRowNameListByStruct(TArray<FString>& OutList, TSharedRef<IPropertyHandle> StructPropertyHandle)
// {
// 	const TSharedPtr<IPropertyHandle> DataTableStructHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDataTableRowSelector, mEditorDataTableStruct));
// 	if (!DataTableStructHandle.IsValid())
// 	{
// 		return false;
// 	}
// 	UObject* TargetObject;
// 	if (DataTableStructHandle->GetValue(TargetObject) != FPropertyAccess::Success)
// 	{
// 		return false;
// 	}
// 	const UStruct* DataTableStruct = Cast<UStruct>(TargetObject);
// 	if (DataTableStruct == nullptr)
// 	{
// 		return false;
// 	}
// 
// 	if (mPropertyTitleName.IsEmpty())
// 	{
// 		mPropertyTitleName = DataTableStruct->GetName();
// 	}
// 
// 	for (TObjectIterator<UDataTable> It; It; ++It)
// 	{
// 		const UDataTable* DataTableAsset = *It;
// 		if (DataTableAsset == nullptr)
// 		{
// 			continue;
// 		}
// 		if (DataTableAsset->RowStruct->IsChildOf(DataTableStruct))
// 		{
// 			for (const FName& RowName : DataTableAsset->GetRowNames())
// 			{
// 				OutList.Add(RowName.ToString());
// 			}
// 		}
// 	}
// 	return OutList.Num() > 0;
// }

bool FDataTableRowSelectorCustomization::AppendRowNameListByDataTable(TArray<FString>& OutList)
{
	TSharedPtr<IPropertyHandleArray> HandleArray = mDataTableHandle->AsArray();
	uint32 NumItem = 0;
	if (HandleArray->GetNumElements(NumItem) != FPropertyAccess::Success)
	{
		return false;
	}

	TArray<FString> RowNameList;
	for (uint32 i = 0; i < NumItem; ++i)
	{
		TSharedRef<IPropertyHandle> ElementHandle = HandleArray->GetElement(i);
		UObject* TargetObject;
		if (ElementHandle->GetValue(TargetObject) != FPropertyAccess::Success)
		{
			continue;
		}
		const UDataTable* DataTable = Cast<UDataTable>(TargetObject);
		if (DataTable == nullptr)
		{
			continue;
		}
		for (const FName& RowName : DataTable->GetRowNames())
		{
			OutList.Add(RowName.ToString());
		}
	}
	return OutList.Num() > 0;
}

void FDataTableRowSelectorCustomization::CheckActiveRowName()
{
	if (!mRowNameHandle.IsValid())
	{
		return;
	}

	FName RowName;
	mRowNameHandle->GetValue(RowName);
	if (RowName.IsNone())
	{
		mRowNameHandle->SetValue(*mRowNameList[0]);
	}
	else
	{
		bool bInvalidName = true;
		for (int32 i = 0; i < mRowNameList.Num(); ++i)
		{
			if (*mRowNameList[i] == RowName.ToString())
			{
				bInvalidName = false;
				break;
			}
		}
		if (bInvalidName)
		{
			mRowNameHandle->SetValue(*mRowNameList[0]);
		}
	}
}

void FDataTableRowSelectorCustomization::OnChangeSelectDataTable()
{
	TArray<FString> RowNameList;
	AppendRowNameListByDataTable(RowNameList);
	RowNameList.Sort([](const FString& A, const FString& B) {
		return A < B;
		});

	FString SelectedString;
	TSharedPtr<FString> SelectedType = mComboBox->GetSelectedItem();
	if (SelectedType.IsValid())
	{
		SelectedString = *SelectedType;
	}

	int32 SelectIndex = 0;
	mRowNameList.Empty();
	mRowNameList.Add(MakeShareable(new FString(TEXT("--None--"))));
	for (const FString& RowName : RowNameList)
	{
		if (SelectedString == RowName)
		{
			SelectIndex = mRowNameList.Num();
		}
		mRowNameList.Add(MakeShareable(new FString(RowName)));
	}

	mComboBox->SetSelectedItem(mRowNameList[SelectIndex]);
}

#undef LOCTEXT_NAMESPACE