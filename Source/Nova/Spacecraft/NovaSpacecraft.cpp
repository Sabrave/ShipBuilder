// Nova project - Gwennaël Arbona

#include "NovaSpacecraft.h"
#include "Nova/Game/NovaOrbitalSimulationTypes.h"
#include "Nova/System/NovaAssetManager.h"
#include "Nova/Nova.h"

#include "Dom/JsonObject.h"

#define LOCTEXT_NAMESPACE "NovaSpacecraft"

// Constants
constexpr float StandardGravity           = 9.807f;
constexpr float SkirtPropellantMultiplier = 1.1f;

/*----------------------------------------------------
    Spacecraft compartment
----------------------------------------------------*/

FNovaCompartmentModule::FNovaCompartmentModule()
	: Description(nullptr)
	, ForwardBulkheadType(ENovaBulkheadType::None)
	, AftBulkheadType(ENovaBulkheadType::None)
	, SkirtPipingType(ENovaSkirtPipingType::None)
	, NeedsWiring(false)
{}

FNovaCompartment::FNovaCompartment()
	: Description(nullptr)
	, HullType(ENovaHullType::None)
	, Modules{FNovaCompartmentModule()}
	, Equipments{nullptr}
	, NeedsOuterSkirt(false)
	, NeedsMainPiping(false)
	, NeedsMainWiring(false)
{}

FNovaCompartment::FNovaCompartment(const class UNovaCompartmentDescription* K) : FNovaCompartment()
{
	Description = K;
}

bool FNovaCompartment::operator==(const FNovaCompartment& Other) const
{
	if (Description != Other.Description || HullType != Other.HullType)
	{
		return false;
	}
	else
	{
		for (int32 ModuleIndex = 0; ModuleIndex < ENovaConstants::MaxModuleCount; ModuleIndex++)
		{
			if (Modules[ModuleIndex] != Other.Modules[ModuleIndex])
			{
				return false;
			}
		}
		for (int32 EquipmentIndex = 0; EquipmentIndex < ENovaConstants::MaxEquipmentCount; EquipmentIndex++)
		{
			if (Equipments[EquipmentIndex] != Other.Equipments[EquipmentIndex])
			{
				return false;
			}
		}

		return true;
	}
}

const UNovaModuleDescription* FNovaCompartment::GetModuleBySocket(FName SocketName) const
{
	if (Description)
	{
		for (int32 ModuleIndex = 0; ModuleIndex < ENovaConstants::MaxModuleCount; ModuleIndex++)
		{
			if (Description->GetModuleSlot(ModuleIndex).SocketName == SocketName)
			{
				return Modules[ModuleIndex].Description;
			}
		}
	}
	return nullptr;
}

const UNovaEquipmentDescription* FNovaCompartment::GetEquipmentySocket(FName SocketName) const
{
	if (Description)
	{
		for (int32 EquipmentIndex = 0; EquipmentIndex < ENovaConstants::MaxEquipmentCount; EquipmentIndex++)
		{
			if (Description->GetEquipmentSlot(EquipmentIndex).SocketName == SocketName)
			{
				return Equipments[EquipmentIndex];
			}
		}
	}
	return nullptr;
}

/*----------------------------------------------------
    Spacecraft compartment metrics
----------------------------------------------------*/

FNovaSpacecraftCompartmentMetrics::FNovaSpacecraftCompartmentMetrics(const FNovaSpacecraft& Spacecraft, int32 CompartmentIndex)
	: ModuleCount(0)
	, EquipmentCount(0)
	, DryMass(0.0f)
	, PropellantMassCapacity(0.0f)
	, CargoMassCapacity(0.0f)
	, Thrust(0.0f)
	, TotalEngineISPTimesThrust(0.0f)
{
	const FNovaCompartment& Compartment = Spacecraft.Compartments[CompartmentIndex];

	if (Compartment.IsValid())
	{
		DryMass = Compartment.Description->Mass;

		// Iterate over modules
		for (int32 ModuleIndex = 0; ModuleIndex < ENovaConstants::MaxModuleCount; ModuleIndex++)
		{
			const FNovaCompartmentModule& Module = Compartment.Modules[ModuleIndex];
			if (IsValid(Module.Description))
			{
				ModuleCount++;
				DryMass += Module.Description->Mass;

				// Handle propellant modules
				const UNovaPropellantModuleDescription* PropellantModule = Cast<UNovaPropellantModuleDescription>(Module.Description);
				if (PropellantModule)
				{
					float PropellantMass = PropellantModule->PropellantMass;

					if (Spacecraft.IsSameModuleInNextCompartment(CompartmentIndex, ModuleIndex))
					{
						PropellantMass *= SkirtPropellantMultiplier;
					}

					PropellantMassCapacity += PropellantMass;
				}

				// Handle cargo modules
				const UNovaCargoModuleDescription* CargoModule = Cast<UNovaCargoModuleDescription>(Module.Description);
				if (CargoModule)
				{
					CargoMassCapacity += CargoModule->CargoMass;
				}
			}
		}

		// Iterate over equipments
		for (const UNovaEquipmentDescription* Equipment : Compartment.Equipments)
		{
			if (IsValid(Equipment))
			{
				EquipmentCount++;
				DryMass += Equipment->Mass;

				// Handle engine equipments
				const UNovaEngineDescription* Engine = Cast<UNovaEngineDescription>(Equipment);
				if (Engine)
				{
					Thrust += Engine->Thrust;
					TotalEngineISPTimesThrust += Engine->SpecificImpulse * Engine->Thrust;
				}
			}
		}
	}
}

TArray<FText> FNovaSpacecraftCompartmentMetrics::GetDescription() const
{
	TArray<FText> Result = INovaDescriptibleInterface::GetDescription();

	Result.Add(FText::FormatNamed(
		LOCTEXT("CompartmentMassFormat", "<img src=\"/Text/Mass\"/> {mass}T"), TEXT("mass"), FText::AsNumber(FMath::RoundToInt(DryMass))));

	if (ModuleCount)
	{
		Result.Add(FText::FormatNamed(
			LOCTEXT("CompartmentModulesFormat", "<img src=\"/Text/Module\"/> {modules} {modules}|plural(one=module,other=modules)"),
			TEXT("modules"), FText::AsNumber(ModuleCount)));
	}

	if (EquipmentCount)
	{
		Result.Add(
			FText::FormatNamed(LOCTEXT("CompartmentEquipmentsFormat",
								   "<img src=\"/Text/Equipment\"/> {equipments} {equipments}|plural(one=equipment,other=equipments)"),
				TEXT("equipments"), FText::AsNumber(EquipmentCount)));
	}

	if (PropellantMassCapacity)
	{
		Result.Add(
			FText::FormatNamed(LOCTEXT("CompartmentPropellantFormat", "<img src=\"/Text/Propellant\"/> {propellant} T propellant capacity"),
				TEXT("propellant"), FText::AsNumber(FMath::RoundToInt(PropellantMassCapacity))));
	}

	if (CargoMassCapacity)
	{
		Result.Add(FText::FormatNamed(LOCTEXT("CompartmentCargoFormat", "<img src=\"/Text/Cargo\"/> {cargo} T cargo capacity"),
			TEXT("cargo"), FText::AsNumber(FMath::RoundToInt(CargoMassCapacity))));
	}

	return Result;
}

/*----------------------------------------------------
    Spacecraft constructor & operators
----------------------------------------------------*/

bool FNovaSpacecraft::operator==(const FNovaSpacecraft& Other) const
{
	if (Identifier != Other.Identifier)
	{
		return false;
	}
	else if (Compartments.Num() != Other.Compartments.Num())
	{
		return false;
	}
	else
	{
		for (int32 CompartmentIndex = 0; CompartmentIndex < Compartments.Num(); CompartmentIndex++)
		{
			if (Compartments[CompartmentIndex] != Other.Compartments[CompartmentIndex])
			{
				return false;
			}
		}

		return true;
	}
}

/*----------------------------------------------------
    Spacecraft interface
----------------------------------------------------*/

bool FNovaSpacecraft::IsValid(FText* Details) const
{
	if (PropulsionMetrics.Thrust <= 0)
	{
		*Details = LOCTEXT("InsufficientThrust", "This spacecraft has no engine");
		return false;
	}
	else if (PropulsionMetrics.PropellantMassCapacity <= 0)
	{
		*Details = LOCTEXT("InsufficientPropellant", "This spacecraft has no propellant tank");
		return false;
	}
	else if (PropulsionMetrics.MaximumDeltaV < 100)
	{
		*Details = LOCTEXT("InsufficientDeltaV", "This spacecraft does not have enough Delta-V");
		return false;
	}

	// Check for invalid equipment pairings
	else
	{
		for (int32 CompartmentIndex = 0; CompartmentIndex < Compartments.Num(); CompartmentIndex++)
		{
			const FNovaCompartment& Compartment = Compartments[CompartmentIndex];
			NCHECK(::IsValid(Compartment.Description));

			for (int32 EquipmentIndex = 0; EquipmentIndex < ENovaConstants::MaxEquipmentCount; EquipmentIndex++)
			{
				const UNovaEquipmentDescription* Equipment = Compartment.Equipments[EquipmentIndex];
				if (Equipment)
				{
					for (int32 GroupedIndex : Compartment.Description->GetGroupedEquipmentSlotsIndices(EquipmentIndex))
					{
						if (Compartment.Equipments[GroupedIndex] != Equipment)
						{
							*Details = FText::FormatNamed(LOCTEXT("InvalidPairing",
															  "The equipment in slot {slot} of compartment {compartment} is not "
															  "correctly paired with identical equipments"),
								TEXT("slot"), Compartment.Description->GetEquipmentSlot(EquipmentIndex).DisplayName, TEXT("compartment"),
								FText::AsNumber(CompartmentIndex + 1));
							return false;
						}
					}
				}
			}
		}
	}

	return true;
}

void FNovaSpacecraft::SerializeJson(TSharedPtr<FNovaSpacecraft>& This, TSharedPtr<FJsonObject>& JsonData, ENovaSerialize Direction)
{
	// Write an asset description to JSON
	auto SaveAsset = [](TSharedPtr<FJsonObject> Save, FString Name, const UNovaAssetDescription* Asset)
	{
		if (Asset)
		{
			Save->SetStringField(Name, Asset->Identifier.ToString(EGuidFormats::Short));
		}
	};

	// Get an asset description from JSON
	auto LoadAsset = [](TSharedPtr<FJsonObject> Save, FString Name)
	{
		const UNovaAssetDescription* Asset = nullptr;

		FString IdentifierString;
		if (Save->TryGetStringField(Name, IdentifierString))
		{
			FGuid AssetIdentifier;
			if (FGuid::Parse(IdentifierString, AssetIdentifier))
			{
				Asset = UNovaAssetManager::Get()->GetAsset(AssetIdentifier);
			}
		}

		return Asset;
	};

	// Writing to JSON
	if (Direction == ENovaSerialize::DataToJson)
	{
		JsonData = MakeShared<FJsonObject>();

		// Spacecraft
		JsonData->SetStringField("I", This->Identifier.ToString(EGuidFormats::Short));

		// Systems
		JsonData->SetNumberField("P", This->SystemState.InitialPropellantMass);

		// Compartments
		TArray<TSharedPtr<FJsonValue>> SavedCompartments;
		for (const FNovaCompartment& Compartment : This->Compartments)
		{
			TSharedPtr<FJsonObject> CompartmentJsonData = MakeShared<FJsonObject>();

			if (Compartment.Description)
			{
				// Compartment
				SaveAsset(CompartmentJsonData, "D", Compartment.Description);
				CompartmentJsonData->SetNumberField("H", static_cast<uint8>(Compartment.HullType));

				// Modules
				for (int32 Index = 0; Index < ENovaConstants::MaxModuleCount; Index++)
				{
					SaveAsset(CompartmentJsonData, FString("M") + FString::FromInt(Index), Compartment.Modules[Index].Description);
				}

				// Equipments
				for (int32 Index = 0; Index < ENovaConstants::MaxEquipmentCount; Index++)
				{
					SaveAsset(CompartmentJsonData, FString("E") + FString::FromInt(Index), Compartment.Equipments[Index]);
				}

				SavedCompartments.Add(MakeShared<FJsonValueObject>(CompartmentJsonData));
			}
		}
		JsonData->SetArrayField("C", SavedCompartments);
	}

	// Reading from JSON
	else
	{
		This = MakeShared<FNovaSpacecraft>();
		This->Create();

		// Spacecraft
		FGuid Identifier;
		if (FGuid::Parse(JsonData->GetStringField("I"), Identifier))
		{
			This->Identifier = Identifier;
		}

		// Systems
		double InitialPropellantMass = 0;
		if (JsonData->TryGetNumberField("P", InitialPropellantMass))
		{
			This->SystemState.InitialPropellantMass = InitialPropellantMass;
		}

		// Compartments
		const TArray<TSharedPtr<FJsonValue>>* SavedCompartments;
		if (JsonData->TryGetArrayField("C", SavedCompartments))
		{
			for (TSharedPtr<FJsonValue> CompartmentObject : *SavedCompartments)
			{
				FNovaCompartment        Compartment;
				TSharedPtr<FJsonObject> CompartmentJsonData = CompartmentObject->AsObject();

				// Compartment
				Compartment.Description = Cast<UNovaCompartmentDescription>(LoadAsset(CompartmentJsonData, "D"));
				NCHECK(Compartment.Description);
				Compartment.HullType = static_cast<ENovaHullType>(CompartmentJsonData->GetNumberField("H"));

				// Modules
				for (int32 Index = 0; Index < ENovaConstants::MaxModuleCount; Index++)
				{
					Compartment.Modules[Index].Description =
						Cast<UNovaModuleDescription>(LoadAsset(CompartmentJsonData, FString("M") + FString::FromInt(Index)));
				}

				// Equipments
				for (int32 Index = 0; Index < ENovaConstants::MaxEquipmentCount; Index++)
				{
					Compartment.Equipments[Index] =
						Cast<UNovaEquipmentDescription>(LoadAsset(CompartmentJsonData, FString("E") + FString::FromInt(Index)));
				}

				This->Compartments.Add(Compartment);
			}
		}

		This->SetDirty();
	}
}

TArray<const UNovaCompartmentDescription*> FNovaSpacecraft::GetCompatibleCompartments(int32 CompartmentIndex) const
{
	TArray<const UNovaCompartmentDescription*> CompartmentDescriptions;

	for (const UNovaCompartmentDescription* Description : UNovaAssetManager::Get()->GetAssets<UNovaCompartmentDescription>())
	{
		CompartmentDescriptions.Add(Description);
	}

	return CompartmentDescriptions;
}

TArray<const class UNovaModuleDescription*> FNovaSpacecraft::GetCompatibleModules(int32 CompartmentIndex, int32 SlotIndex) const
{
	TArray<const UNovaModuleDescription*> ModuleDescriptions;
	TArray<const UNovaModuleDescription*> AllModuleDescriptions = UNovaAssetManager::Get()->GetAssets<UNovaModuleDescription>();
	const FNovaCompartment&               Compartment           = Compartments[CompartmentIndex];

	ModuleDescriptions.Add(nullptr);
	if (Compartment.IsValid() && SlotIndex < Compartment.Description->ModuleSlots.Num())
	{
		for (const UNovaModuleDescription* ModuleDescription : AllModuleDescriptions)
		{
			ModuleDescriptions.AddUnique(ModuleDescription);
		}
	}

	return ModuleDescriptions;
}

TArray<const UNovaEquipmentDescription*> FNovaSpacecraft::GetCompatibleEquipments(int32 CompartmentIndex, int32 SlotIndex) const
{
	TArray<const UNovaEquipmentDescription*> EquipmentDescriptions;
	TArray<const UNovaEquipmentDescription*> AllEquipmentDescriptions = UNovaAssetManager::Get()->GetAssets<UNovaEquipmentDescription>();
	const FNovaCompartment&                  Compartment              = Compartments[CompartmentIndex];

	EquipmentDescriptions.Add(nullptr);
	if (Compartment.IsValid() && SlotIndex < Compartment.Description->EquipmentSlots.Num())
	{
		for (const UNovaEquipmentDescription* EquipmentDescription : AllEquipmentDescriptions)
		{
			const TArray<ENovaEquipmentType>& SupportedTypes = Compartment.Description->EquipmentSlots[SlotIndex].SupportedTypes;
			if (SupportedTypes.Num() == 0 || SupportedTypes.Contains(EquipmentDescription->EquipmentType))
			{
				if (EquipmentDescription->EquipmentType == ENovaEquipmentType::Engine && !IsLastCompartment(CompartmentIndex))
				{
					continue;
				}
				else
				{
					EquipmentDescriptions.AddUnique(EquipmentDescription);
				}
			}
		}
	}

	return EquipmentDescriptions;
}

/*----------------------------------------------------
    Internals
----------------------------------------------------*/

void FNovaSpacecraft::UpdateProceduralElements()
{
	for (int32 CompartmentIndex = 0; CompartmentIndex < Compartments.Num(); CompartmentIndex++)
	{
		FNovaCompartment& Compartment = Compartments[CompartmentIndex];

		if (Compartment.IsValid())
		{
			// TODO : outer skirt would be used for compartment that have side modules, when the following (aft) compartment does not
			Compartment.NeedsOuterSkirt = false;

			// TODO : review whether main piping & wiring can ever be undesired - maybe on extremities with no module ?
			Compartment.NeedsMainPiping = true;
			Compartment.NeedsMainWiring = true;

			// Process modules
			for (int32 ModuleIndex = 0; ModuleIndex < ENovaConstants::MaxModuleCount; ModuleIndex++)
			{
				FNovaCompartmentModule& Module = Compartment.Modules[ModuleIndex];

				// Reset state
				Module.ForwardBulkheadType = ENovaBulkheadType::Standard;
				Module.AftBulkheadType     = ENovaBulkheadType::Standard;
				Module.SkirtPipingType     = ENovaSkirtPipingType::None;
				Module.NeedsWiring         = false;

				// Handle forced piping
				if (Compartment.Description->GetModuleSlot(ModuleIndex).ForceSkirtPiping)
				{
					Module.SkirtPipingType = ENovaSkirtPipingType::Simple;
				}

				if (Module.Description)
				{
					Module.NeedsWiring = true;
					// NLOG("FNovaSpacecraft::UpdateProceduralElements : compartment %d, module %d", CompartmentIndex, ModuleIndex);

					// Define bulkheads
					if (IsFirstCompartment(CompartmentIndex))
					{
						Module.ForwardBulkheadType = ENovaBulkheadType::Outer;
						// NLOG("FNovaSpacecraft::UpdateProceduralElements : -> forward is Outer");
					}
					else if (IsSameModuleInPreviousCompartment(CompartmentIndex, ModuleIndex))
					{
						Module.ForwardBulkheadType = ENovaBulkheadType::Skirt;
						Module.NeedsWiring         = false;
						// NLOG("FNovaSpacecraft::UpdateProceduralElements : -> forward is Connected");
					}

					if (IsLastCompartment(CompartmentIndex))
					{
						Module.AftBulkheadType = ENovaBulkheadType::Outer;
						// NLOG("FNovaSpacecraft::UpdateProceduralElements : -> aft is Outer");
					}
					else if (IsSameModuleInNextCompartment(CompartmentIndex, ModuleIndex))
					{
						Module.AftBulkheadType = ENovaBulkheadType::Skirt;
						// NLOG("FNovaSpacecraft::UpdateProceduralElements : -> aft is Connected");
					}

					// Define piping
					if (Module.Description->NeedsPiping && !IsSameModuleInNextCompartment(CompartmentIndex, ModuleIndex))
					{
						Module.SkirtPipingType = ENovaSkirtPipingType::Connection;
						// NLOG("FNovaSpacecraft::UpdateProceduralElements : -> skirt piping is Connection");
					}
					else
					{
						Module.SkirtPipingType = ENovaSkirtPipingType::Simple;
						// NLOG("FNovaSpacecraft::UpdateProceduralElements : -> skirt piping is Simple");
					}
				}
			}
		}
	}
}

void FNovaSpacecraft::UpdatePropulsionMetrics()
{
	PropulsionMetrics               = FNovaSpacecraftPropulsionMetrics();
	float TotalEngineISPTimesThrust = 0;

	// Iterate over compartments
	for (int32 CompartmentIndex = 0; CompartmentIndex < Compartments.Num(); CompartmentIndex++)
	{
		FNovaSpacecraftCompartmentMetrics Metrics(*this, CompartmentIndex);

		PropulsionMetrics.DryMass += Metrics.DryMass;
		PropulsionMetrics.PropellantMassCapacity += Metrics.PropellantMassCapacity;
		PropulsionMetrics.CargoMassCapacity += Metrics.CargoMassCapacity;
		PropulsionMetrics.Thrust += Metrics.Thrust;
		TotalEngineISPTimesThrust += Metrics.TotalEngineISPTimesThrust;
	}

	// Compute metrics
	PropulsionMetrics.MaximumMass =
		PropulsionMetrics.DryMass + PropulsionMetrics.PropellantMassCapacity + PropulsionMetrics.CargoMassCapacity;
	if (PropulsionMetrics.Thrust > 0)
	{
		PropulsionMetrics.SpecificImpulse = TotalEngineISPTimesThrust / PropulsionMetrics.Thrust;
		PropulsionMetrics.ExhaustVelocity = StandardGravity * PropulsionMetrics.SpecificImpulse;
		PropulsionMetrics.PropellantRate  = PropulsionMetrics.Thrust / PropulsionMetrics.ExhaustVelocity;
		PropulsionMetrics.MaximumDeltaV =
			PropulsionMetrics.ExhaustVelocity * log((PropulsionMetrics.MaximumMass) / PropulsionMetrics.DryMass);
		PropulsionMetrics.MaximumBurnTime = PropulsionMetrics.PropellantMassCapacity / PropulsionMetrics.PropellantRate;
	}

#if 0
	NLOG("--------------------------------------------------------------------------------");
	NLOG("Mass specifications : dry %.1fT, propellant %.1fT, cargo %.1fT, total %.1fT", PropulsionMetrics.DryMass, PropulsionMetrics.PropellantMass,
		PropulsionMetrics.CargoMass, PropulsionMetrics.TotalMass);
	NLOG("Engine specifications : thrust %.1fkN, ISP %.1fm/s, EV %.1fm/s", PropulsionMetrics.Thrust, PropulsionMetrics.SpecificImpulse,
		PropulsionMetrics.ExhaustVelocity);
	NLOG("Delta-V specifications : total %.1fm/s, burn time %.1fs", PropulsionMetrics.TotalDeltaV, PropulsionMetrics.TotalBurnTime);
	NLOG("--------------------------------------------------------------------------------");
#endif
}

bool FNovaSpacecraft::IsFirstCompartment(int32 CompartmentIndex) const
{
	for (int32 Index = 0; Index < CompartmentIndex; Index++)
	{
		if (Compartments[Index].IsValid())
		{
			return false;
		}
	}
	return true;
}

bool FNovaSpacecraft::IsLastCompartment(int32 CompartmentIndex) const
{
	for (int32 Index = CompartmentIndex + 1; Index < Compartments.Num(); Index++)
	{
		if (Compartments[Index].IsValid())
		{
			return false;
		}
	}
	return true;
}

bool FNovaSpacecraft::IsSameModuleInPreviousCompartment(int32 CompartmentIndex, int32 ModuleIndex) const
{
	const FNovaCompartment&       Compartment             = Compartments[CompartmentIndex];
	const FNovaCompartmentModule& Module                  = Compartment.Modules[ModuleIndex];
	FName                         CurrentModuleSocketName = Compartment.Description->GetModuleSlot(ModuleIndex).SocketName;

	for (int32 Index = CompartmentIndex - 1; Index >= 0; Index--)
	{
		const FNovaCompartment& PreviousCompartment = Compartments[Index];
		if (PreviousCompartment.IsValid())
		{
			return PreviousCompartment.GetModuleBySocket(CurrentModuleSocketName) == Module.Description;
		}
	}

	return false;
};

bool FNovaSpacecraft::IsSameModuleInNextCompartment(int32 CompartmentIndex, int32 ModuleIndex) const
{
	const FNovaCompartment&       Compartment             = Compartments[CompartmentIndex];
	const FNovaCompartmentModule& Module                  = Compartment.Modules[ModuleIndex];
	FName                         CurrentModuleSocketName = Compartment.Description->GetModuleSlot(ModuleIndex).SocketName;

	for (int32 Index = CompartmentIndex + 1; Index < Compartments.Num(); Index++)
	{
		const FNovaCompartment& NextCompartment = Compartments[Index];
		if (NextCompartment.IsValid())
		{
			return NextCompartment.GetModuleBySocket(CurrentModuleSocketName) == Module.Description;
		}
	}

	return false;
};

#undef LOCTEXT_NAMESPACE
