// Copyright 2022 SensyuGames.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"

class FDataTableRowSelectorCustomization : public IPropertyTypeCustomization
{
public:
	FDataTableRowSelectorCustomization();

	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FDataTableRowSelectorCustomization);
	}

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle,
		class FDetailWidgetRow& HeaderRow,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle,
		class IDetailChildrenBuilder& StructBuilder,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;


protected:
	void OnSelectionChanged(TSharedPtr<FString> Type, ESelectInfo::Type SelectionType);
	TSharedRef<SWidget> OnGenerateWidget(TSharedPtr<FString> Type);
	FText GetSelectedTypeText() const;
	int32 GetSelectIndex() const;
	void SetupDisplayName(TSharedRef<IPropertyHandle> StructPropertyHandle);
	void SetupRowNameList(TSharedRef<IPropertyHandle> StructPropertyHandle);
	bool AppendRowNameListByPath(TArray<FString>& OutList, TSharedRef<IPropertyHandle> StructPropertyHandle);
	//bool AppendRowNameListByStruct(TArray<FString>& OutList, TSharedRef<IPropertyHandle> StructPropertyHandle);
	bool AppendRowNameListByDataTable(TArray<FString>& OutList);
	void CheckActiveRowName();

	void OnChangeSelectDataTable();

private:
	TSharedPtr<IPropertyHandle> mRowNameHandle;
	TSharedPtr<IPropertyHandle> mDataTableHandle;

	TArray< TSharedPtr<FString> > mRowNameList;
	TSharedPtr< SComboBox< TSharedPtr<FString> > > mComboBox;
	FString mPropertyTitleName;
	bool mbUseDataTableProperty = false;
};