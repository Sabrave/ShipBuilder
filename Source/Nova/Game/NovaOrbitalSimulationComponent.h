// Nova project - Gwennaël Arbona

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NovaOrbitalSimulationTypes.h"
#include "Nova/Game/NovaGameTypes.h"

#include "NovaOrbitalSimulationComponent.generated.h"

/** Orbital simulation component that ticks orbiting spacecraft */
UCLASS(ClassGroup = (Nova))
class UNovaOrbitalSimulationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNovaOrbitalSimulationComponent();

	/*----------------------------------------------------
	    Gameplay
	----------------------------------------------------*/

public:
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Build trajectory parameters */
	FNovaTrajectoryParameters PrepareTrajectory(
		const class UNovaArea* Source, const class UNovaArea* Destination, double DeltaTime = 0) const;

	/** Compute a trajectory */
	TSharedPtr<FNovaTrajectory> ComputeTrajectory(const FNovaTrajectoryParameters& Parameters, float PhasingAltitude);

	/** Check if this trajectory can be committed to players */
	bool CanCommitTrajectory(const TSharedPtr<FNovaTrajectory>& Trajectory) const;

	/** Commit the trajectory decided by players */
	void CommitTrajectory(const TSharedPtr<FNovaTrajectory>& Trajectory, const TArray<FGuid>& SpacecraftIdentifiers);

	/** Get the player trajectory */
	const FNovaTrajectory* GetCommittedPlayerTrajectory() const;

	/** Complete the current trajectory of ships */
	void CompleteTrajectory(const TArray<FGuid>& SpacecraftIdentifiers);

	/** Get the orbit & position data for all areas */
	const TMap<const class UNovaArea*, FNovaOrbitalLocation>& GetAreasOrbitalLocation() const
	{
		return AreasOrbitalLocation;
	}

	/** Get the orbit & position data for all spacecraft */
	const TMap<FGuid, FNovaOrbitalLocation>& GetSpacecraftOrbitalLocation() const
	{
		return SpacecraftOrbitalLocation;
	}

	/** Get the current time in minutes */
	double GetCurrentTime() const;

	/*----------------------------------------------------
	    Internals
	----------------------------------------------------*/

protected:
	/** Update all area's position */
	void ProcessAreas();

	/** Update the current orbit of spacecraft */
	void ProcessSpacecraftOrbits();

	/** Update the current trajectory of spacecraft */
	void ProcessSpacecraftTrajectories();

	/** Compute the parameters of a Hohmann transfer orbit */
	static FNovaHohmannTransfer ComputeHohmannTransfer(
		const double GravitationalParameter, const double SourceRadius, const double DestinationRadius)
	{
		FNovaHohmannTransfer Result;

		Result.StartDeltaV =
			abs(sqrt(GravitationalParameter / SourceRadius) * (sqrt((2.0 * DestinationRadius) / (SourceRadius + DestinationRadius)) - 1.0));
		Result.EndDeltaV =
			abs(sqrt(GravitationalParameter / DestinationRadius) * (1.0 - sqrt((2.0 * SourceRadius) / (SourceRadius + DestinationRadius))));

		Result.TotalDeltaV = Result.StartDeltaV + Result.EndDeltaV;

		Result.Duration = PI * sqrt(pow(SourceRadius + DestinationRadius, 3.0) / (8.0 * GravitationalParameter)) / 60;

		return Result;
	}

	/** Compute the period of a stable circular orbit in minutes */
	static double GetCircularOrbitPeriod(const double GravitationalParameter, const double Radius)
	{
		return 2.0 * PI * sqrt(pow(Radius, 3.0) / GravitationalParameter) / 60.0;
	}

	/** Get the current phase of an area */
	double GetAreaPhase(const class UNovaArea* Area, double DeltaTime = 0) const;

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

private:
	// Replicated orbit database
	UPROPERTY(Replicated)
	FNovaOrbitDatabase OrbitDatabase;

	// Replicated trajectory database
	UPROPERTY(Replicated)
	FNovaTrajectoryDatabase TrajectoryDatabase;

	// Local state
	TArray<const class UNovaArea*>                     Areas;
	TMap<const class UNovaArea*, FNovaOrbitalLocation> AreasOrbitalLocation;
	TMap<FGuid, FNovaOrbitalLocation>                  SpacecraftOrbitalLocation;
};
