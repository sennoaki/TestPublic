

#include "PropertyCutomizeAssistEditor.h"
#include "DataTableRowSelector.h"
#include "DataTableRowSelectorCustomization.h"

#define LOCTEXT_NAMESPACE "FPropertyCutomizeAssistEditorModule"

void FPropertyCutomizeAssistEditorModule::StartupModule()
{
	auto& ModuleManager = FModuleManager::Get();
	if (ModuleManager.IsModuleLoaded("PropertyEditor"))
	{
		auto& PropertyEditorModule = ModuleManager.LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

		PropertyEditorModule.RegisterCustomPropertyTypeLayout(
			("DataTableRowSelector"),
			FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FDataTableRowSelectorCustomization::MakeInstance)
		);

		PropertyEditorModule.NotifyCustomizationModuleChanged();
	}
}

void FPropertyCutomizeAssistEditorModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPropertyCutomizeAssistEditorModule, PropertyCutomizeAssistEditor)