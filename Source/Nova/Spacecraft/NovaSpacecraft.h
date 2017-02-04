// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Nova/Game/NovaGameTypes.h"
#include "NovaSpacecraft.generated.h"


/*----------------------------------------------------
	General spacecraft types
----------------------------------------------------*/

/** Equipment requirement types */
UENUM()
enum class ENovaEquipmentType : uint8
{
	Standard,
	Engine
};

/** Type of bulkhead to use */
UENUM()
enum class ENovaAssemblyBulkheadType : uint8
{
	None,
	Standard,
	Skirt,
	Outer,
};

/** Type of skirt piping to use */
UENUM()
enum class ENovaAssemblySkirtPipingType : uint8
{
	None,
	Simple,
	Connection
};

/** Possible hull styles */
UENUM()
enum class ENovaAssemblyHullType : uint8
{
	None,
	PlasticFabric,
	MetalFabric
};

/** Possible construction element types */
enum class ENovaAssemblyElementType : uint8
{
	Module,
	Structure,
	Equipment,
	Wiring,
	Hull
};

/** Single construction element */
struct FNovaAssemblyElement
{
	FNovaAssemblyElement()
	{}

	FNovaAssemblyElement(ENovaAssemblyElementType T)
		: Type(T)
	{}

	FSoftObjectPath           Asset;
	ENovaAssemblyElementType  Type;
	class INovaMeshInterface* Mesh = nullptr;
};

/** Compartment processing delegate */
DECLARE_DELEGATE_TwoParams(FNovaAssemblyCallback, FNovaAssemblyElement&, TSoftObjectPtr<UObject>);


/*----------------------------------------------------
	Spacecraft description types
----------------------------------------------------*/

/** Module slot metadata */
USTRUCT()
struct FNovaModuleSlot
{
	GENERATED_BODY()

public:

	// Socket to attach to on the structure mesh
	UPROPERTY(Category = Compartment, EditDefaultsOnly)
	FName SocketName;

	// Whether to force a simple connection pipe on this slot when no module is used
	UPROPERTY(Category = Compartment, EditDefaultsOnly)
	bool ForceSkirtPiping;
};

/** Equipment slot metadata */
USTRUCT()
struct FNovaEquipmentSlot
{
	GENERATED_BODY()

public:

	// Socket to attach to on the structure mesh
	UPROPERTY(Category = Compartment, EditDefaultsOnly)
	FName SocketName;

	// List of equipment types that can be mounted on this slot
	UPROPERTY(Category = Compartment, EditDefaultsOnly)
	TArray<ENovaEquipmentType> SupportedTypes;
};

/** Description of a main compartment asset */
UCLASS(ClassGroup = (Nova))
class UNovaCompartmentDescription : public UNovaAssetDescription
{
	GENERATED_BODY()

public:

	/** Get a list of hull styles supported by this compartment */
	TArray<ENovaAssemblyHullType> GetSupportedHullTypes() const
	{
		TArray<ENovaAssemblyHullType> Result;

		Result.Add(ENovaAssemblyHullType::None);
		Result.Add(ENovaAssemblyHullType::PlasticFabric);
		Result.Add(ENovaAssemblyHullType::MetalFabric);

		return Result;
	}

	/** Get the module setup at this index, if it exists, by index */
	FNovaModuleSlot GetModuleSlot(int32 Index) const
	{
		return Index < ModuleSlots.Num() ? ModuleSlots[Index] : FNovaModuleSlot();
	}

	/** Get the equipment setup at this index, if it exists, by index */
	FNovaEquipmentSlot GetEquipmentSlot(int32 Index) const
	{
		return Index < EquipmentSlots.Num() ? EquipmentSlots[Index] : FNovaEquipmentSlot();
	}

	/** Return the appropriate main piping mesh */
	TSoftObjectPtr<class UStaticMesh> GetMainPiping(bool Enabled) const
	{
		return Enabled ? MainPiping : EmptyMesh;
	}

	/** Return the appropriate skirt piping mesh */
	TSoftObjectPtr<class UStaticMesh> GetSkirtPiping(ENovaAssemblySkirtPipingType Type) const
	{
		switch (Type)
		{
		default:
		case ENovaAssemblySkirtPipingType::None:
			return EmptyMesh;
		case ENovaAssemblySkirtPipingType::Simple:
			return SimpleSkirtPiping;
		case ENovaAssemblySkirtPipingType::Connection:
			return ConnectionSkirtPiping;
		}
	}

	/** Return the appropriate main hull mesh */
	TSoftObjectPtr<class UStaticMesh> GetMainHull(ENovaAssemblyHullType Type) const
	{
		return Type != ENovaAssemblyHullType::None ? MainHull : EmptyMesh;
	}

	/** Return the appropriate outer hull mesh */
	TSoftObjectPtr<class UStaticMesh> GetOuterHull(ENovaAssemblyHullType Type) const
	{
		return Type != ENovaAssemblyHullType::None ? OuterHull : EmptyMesh;
	}

	/** Return the appropriate main wiring mesh */
	TSoftObjectPtr<class UStaticMesh> GetMainWiring(bool Enabled) const
	{
		return Enabled ? MainWiring : EmptyMesh;
	}

	/** Return the appropriate module-connection wiring mesh */
	TSoftObjectPtr<class UStaticMesh> GetConnectionWiring(bool Enabled) const
	{
		return Enabled ? ConnectionWiring : EmptyMesh;
	}

public:
	
	// Main structural element
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> MainStructure = EmptyMesh;
	
	// Skirt structural element
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> OuterStructure = EmptyMesh;
	
	// Main piping element
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> MainPiping = EmptyMesh;
	
	// Simple direct piping (skirt)
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> SimpleSkirtPiping = EmptyMesh;
	
	// Tank-connected piping (skirt)
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> ConnectionSkirtPiping = EmptyMesh;
	
	// Module-connected wiring
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> MainWiring = EmptyMesh;
	
	// Module-connected wiring
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> ConnectionWiring = EmptyMesh;
	
	// Decorative outer hull
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> MainHull = EmptyMesh;
	
	// Decorative outer hull (skirt)
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> OuterHull = EmptyMesh;

	// Metadata for module slots
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	TArray<FNovaModuleSlot> ModuleSlots;

	// Metadata for equipment slots
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	TArray<FNovaEquipmentSlot> EquipmentSlots;
};

/** Description of an optional compartment equipment */
UCLASS(ClassGroup = (Nova))
class UNovaModuleDescription : public UNovaAssetDescription
{
	GENERATED_BODY()

public:

	/** Get the appropriate bulkhead mesh */
	TSoftObjectPtr<class UStaticMesh> GetBulkhead(ENovaAssemblyBulkheadType Style, bool Forward) const
	{
		switch (Style)
		{
		case ENovaAssemblyBulkheadType::None:
			return EmptyMesh;
		case ENovaAssemblyBulkheadType::Standard:
			return Forward ? ForwardBulkhead : AftBulkhead;
		case ENovaAssemblyBulkheadType::Skirt:
			return Forward ? EmptyMesh : SkirtBulkhead;
		case ENovaAssemblyBulkheadType::Outer:
			return Forward ? OuterForwardBulkhead : OuterAftBulkhead;
		default:
			return nullptr;
		}
	}

public:

	// Main module segment
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> Segment = EmptyMesh;

	// Standard forward bulkhead
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> ForwardBulkhead = EmptyMesh;

	// Standard aft bulkhead
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> AftBulkhead = EmptyMesh;

	// Skirt bulkhead - forward side of the module behind will be empty
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> SkirtBulkhead = EmptyMesh;

	// Outer-facing forward bulkhead
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> OuterForwardBulkhead = EmptyMesh;

	// Outer-facing aft external bulkhead
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> OuterAftBulkhead = EmptyMesh;

	// Whether the module needs tank piping
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	bool NeedsPiping = false;

};

/** Description of an optional compartment equipment */
UCLASS(ClassGroup = (Nova))
class UNovaEquipmentDescription : public UNovaAssetDescription
{
	GENERATED_BODY()

public:

	TSoftObjectPtr<class UObject> GetMesh() const
	{
		if (!SkeletalEquipment.IsNull())
		{
			return SkeletalEquipment;
		}
		else if (!StaticEquipment.IsNull())
		{
			return StaticEquipment;
		}
		else
		{
			return EmptyMesh;
		}
	}

public:

	// Animated equipment variant
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class USkeletalMesh> SkeletalEquipment;

	// Equipment animation
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UAnimationAsset> SkeletalAnimation;

	// Static equipment variant
	UPROPERTY(Category = Elements, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> StaticEquipment;

	// Equipment requirement
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	ENovaEquipmentType EquipmentType;

};


/*----------------------------------------------------
	Spacecraft data types
----------------------------------------------------*/

/** Compartment module assembly data */
USTRUCT(Atomic)
struct FNovaCompartmentModule
{
	GENERATED_BODY();

	FNovaCompartmentModule()
		: Description(nullptr)
		, ForwardBulkheadType(ENovaAssemblyBulkheadType::None)
		, AftBulkheadType(ENovaAssemblyBulkheadType::None)
		, SkirtPipingType(ENovaAssemblySkirtPipingType::None)
		, NeedsWiring(false)
	{}

	UPROPERTY()
	const class UNovaModuleDescription* Description;

	UPROPERTY()
	ENovaAssemblyBulkheadType ForwardBulkheadType;

	UPROPERTY()
	ENovaAssemblyBulkheadType AftBulkheadType;

	UPROPERTY()
	ENovaAssemblySkirtPipingType SkirtPipingType;

	UPROPERTY()
	bool NeedsWiring;
};

/** Compartment assembly data */
USTRUCT(Atomic)
struct FNovaCompartment
{
	GENERATED_BODY();

	FNovaCompartment();

	FNovaCompartment(const class UNovaCompartmentDescription* K);

	/** Check if this assembly represents a non-empty compartment */
	bool IsValid() const
	{
		return Description != nullptr;
	}

	/** Get the description of the module residing at a particular socket name */
	const UNovaModuleDescription* GetModuleBySocket(FName SocketName) const
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

	/** Get the description of the equipment residing at a particular socket name */
	const UNovaEquipmentDescription* GetEquipmentySocket(FName SocketName) const
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

	UPROPERTY()
	const class UNovaCompartmentDescription* Description;

	UPROPERTY()
	bool NeedsOuterSkirt;

	UPROPERTY()
	bool NeedsMainPiping;

	UPROPERTY()
	bool NeedsMainWiring;

	UPROPERTY()
	ENovaAssemblyHullType HullType;

	UPROPERTY()
	FNovaCompartmentModule Modules[ENovaConstants::MaxModuleCount];

	UPROPERTY()
	const class UNovaEquipmentDescription* Equipments[ENovaConstants::MaxEquipmentCount];
};

/** Full spacecraft assembly data */
USTRUCT(Atomic)
struct FNovaSpacecraft
{
	GENERATED_BODY();

public:

	/** Update bulkheads, pipes, wiring, based on the current state */
	void UpdateProceduralElements();

	/** Get a safe copy of this spacecraft without empty compartments */
	FNovaSpacecraft GetSafeCopy() const;

	/** Get a shared pointer copy of this spacecraft */
	TSharedPtr<FNovaSpacecraft> GetSharedCopy() const;

	/** Serialize the spacecraft */
	static void SerializeJson(TSharedPtr<FNovaSpacecraft>& This, TSharedPtr<class FJsonObject>& JsonData, ENovaSerialize Direction);


public:

	// Compartment data
	UPROPERTY()
	TArray<FNovaCompartment> Compartments;
};
