// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Nova/Spacecraft/NovaSpacecraft.h"
#include "NovaGameState.generated.h"

/** Time dilation settings */
UENUM()
enum class ENovaTimeDilation : uint8
{
	Normal,
	Level1,
	Level2,
	Level3
};

/** Spacecraft database with fast array replication and fast lookup */
USTRUCT()
struct FNovaSpacecraftDatabase : public FFastArraySerializer
{
	GENERATED_BODY()

	bool Add(const FNovaSpacecraft& Spacecraft)
	{
		return Cache.Add(*this, Array, Spacecraft);
	}

	void Remove(const FGuid& Identifier)
	{
		Cache.Remove(*this, Array, Identifier);
	}

	const FNovaSpacecraft* Get(const FGuid& Identifier) const
	{
		return Cache.Get(Identifier, Array);
	}

	TArray<FNovaSpacecraft>& Get()
	{
		return Array;
	}

	void UpdateCache()
	{
		Cache.Update(Array);
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FNovaSpacecraft, FNovaSpacecraftDatabase>(Array, DeltaParms, *this);
	}

protected:
	UPROPERTY()
	TArray<FNovaSpacecraft> Array;

	TGuidCacheMap<FNovaSpacecraft> Cache;
};

/** Enable fast replication */
template <>
struct TStructOpsTypeTraits<FNovaSpacecraftDatabase> : public TStructOpsTypeTraitsBase2<FNovaSpacecraftDatabase>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

/** Replicated game state class */
UCLASS(ClassGroup = (Nova))
class ANovaGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ANovaGameState();

	/*----------------------------------------------------
	    Loading & saving
	----------------------------------------------------*/

	TSharedPtr<struct FNovaGameStateSave> Save() const;

	void Load(TSharedPtr<struct FNovaGameStateSave> SaveData);

	static void SerializeJson(
		TSharedPtr<struct FNovaGameStateSave>& SaveData, TSharedPtr<class FJsonObject>& JsonData, ENovaSerialize Direction);

	/*----------------------------------------------------
	    General game state
	----------------------------------------------------*/

	virtual void Tick(float DeltaTime) override;

	/** Set the current area to use */
	void SetCurrentArea(const class UNovaArea* Area, bool StartDocked);

	/** Get the current area we are at */
	const class UNovaArea* GetCurrentArea() const
	{
		return CurrentArea;
	}

	/** Get the current sub-level name to use */
	FName GetCurrentLevelName() const;

	/** Signal a shared transition and get optional title text to show */
	TPair<FText, FText> OnSharedTransition();

	/** Whether spacecraft at this area should start docked */
	bool ShouldStartDocked() const
	{
		return StartDocked;
	}

	/** Return the orbital simulation class */
	class UNovaOrbitalSimulationComponent* GetOrbitalSimulation() const
	{
		return OrbitalSimulationComponent;
	}

	/*----------------------------------------------------
	    Spacecraft management
	----------------------------------------------------*/

	/** Register or update a spacecraft */
	void UpdateSpacecraft(const FNovaSpacecraft& Spacecraft, bool IsPlayerSpacecraft);

	/** Return a pointer for a spacecraft by identifier */
	const FNovaSpacecraft* GetSpacecraft(FGuid Identifier) const
	{
		return SpacecraftDatabase.Get(Identifier);
	}

	/** Return the identifier of one of the player spacecraft */
	FGuid GetPlayerSpacecraftIdentifier() const;

	/** Return the identifiers of all of the player spacecraft */
	TArray<FGuid> GetPlayerSpacecraftIdentifiers() const;

	/*----------------------------------------------------
	    Time management
	----------------------------------------------------*/

	/** Get the current game time in minutes */
	double GetCurrentTime() const;

	/** Simulate the world at full speed until an event */
	void FastForward();

	/** Check if we can skip time */
	bool CanFastForward() const;

	/** Check if we are in a time skip */
	bool IsInFastForward() const
	{
		return IsFastForward;
	}

	/** Get the current time dilation factor */
	void SetTimeDilation(ENovaTimeDilation Dilation);

	/** Get time dilation values */
	static float GetTimeDilationValue(ENovaTimeDilation Dilation)
	{
		constexpr float TimeDilationValues[] = {
			1,       // Normal : 1s = 1s
			60,      // Level1 : 1s = 1m
			1200,    // Level2 : 1s = 20m
			7200,    // Level3 : 1s = 2h
		};

		return TimeDilationValues[static_cast<int32>(Dilation)];
	}

	/** Get the current time dilation */
	ENovaTimeDilation GetCurrentTimeDilation() const
	{
		return ServerTimeDilation;
	}

	/** Get the current time dilation */
	double GetCurrentTimeDilationValue() const
	{
		return GetTimeDilationValue(ServerTimeDilation);
	}

	/** Check if we can dilate time */
	bool CanDilateTime(ENovaTimeDilation Dilation) const;

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:
	/** Update the spacecraft database */
	void ProcessSpacecraftDatabase();

	/** Process time */
	bool ProcessTime(double DeltaTimeMinutes);

	/** Server replication event for time reconciliation */
	UFUNCTION()
	void OnServerTimeReplicated();

	/*----------------------------------------------------
	    Properties
	----------------------------------------------------*/

protected:
	// Threshold in seconds above which the client time starts compensating
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float MinimumTimeCorrectionThreshold;

	// Threshold in seconds above which the client time is at maximum compensation
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float MaximumTimeCorrectionThreshold;

	// Maximum time dilation applied to compensate time
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	float TimeCorrectionFactor;

	// Time between simulation updates during fast forward in minutes
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	int32 FastForwardUpdateTime;

	// Number of update steps to run per frame under fast forward
	UPROPERTY(Category = Nova, EditDefaultsOnly)
	int32 FastForwardUpdatesPerFrame;

	/*----------------------------------------------------
	    Components
	----------------------------------------------------*/

protected:
	// Global orbital simulation manager
	UPROPERTY(Category = Nova, VisibleDefaultsOnly, BlueprintReadOnly)
	class UNovaOrbitalSimulationComponent* OrbitalSimulationComponent;

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

private:
	// Current level-based area
	UPROPERTY(Replicated)
	const class UNovaArea* CurrentArea;

	// Replicated spacecraft database
	UPROPERTY(Replicated)
	FNovaSpacecraftDatabase SpacecraftDatabase;

	// Replicated world time value
	UPROPERTY(ReplicatedUsing = OnServerTimeReplicated)
	double ServerTime;

	// Replicated world time dilation
	UPROPERTY(Replicated)
	ENovaTimeDilation ServerTimeDilation;

	// General state
	bool                           StartDocked;
	const class ANovaPlayerState*  CurrentPlayerState;
	TArray<const class UNovaArea*> Areas;

	// Time processing state
	double ClientTime;
	double ClientAdditionalTimeDilation;
	bool   IsFastForward;

	// Shared transition state
	double                 TimeSinceTransition;
	const class UNovaArea* LastTransitionArea;
};
