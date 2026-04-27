// AtmosSubSys.h
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "AtmosSubSys.generated.h"

// Forward declarations to improve compile times
class AExponentialHeightFog;
class UExponentialHeightFogComponent;

/**
 * UAtmosSubSys
 * A world subsystem that manages and updates the global atmospheric fog in a world.
 */
UCLASS()
class CLOUDRENDERER_API UAtmosSubSys
	: public UWorldSubsystem
	, public FTickableGameObject
{
	GENERATED_BODY()
	
public:	
    // Subsystem lifecycle
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

    // Determines if subsystem should exist in the given context
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    // Tickable interface
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual TStatId GetStatId() const override;

private:
    bool SetupFog();

private:
    // Pointers tracked by UE Garbage Collector
    UPROPERTY()
    AExponentialHeightFog* FogActor = nullptr;

    UPROPERTY()
    UExponentialHeightFogComponent* FogComponent = nullptr;

    // Track fog state
    bool bInitializedFog = false;

    // Tracks if this subsystem built the fog object
    bool bSpawnedFog = false;

    // Tag used to identify fog actors created by this subsystem
    const FName ManagedFogTag = TEXT("CloudRenderer_ManagedFog");

    // === CONFIG PARAMS ===
    const FLinearColor FogTone = FLinearColor(0.35f, 0.05f, 0.08f);
    const float TargetBaseDensity = 0.15f;
    const float TargetFogFalloff = 0.01f;
    const float TargetStartDistance = 0.0f;
    const float TargetFogMaxOpacity = 0.9f;
    const float TargetVolumetricDistance = 8000.0f;
    const float ExtinctionScale = 4.0f;
    const float FogScatteringDistribution = 0.6f;

    // === ANIMATION SETTINGS ===
    const bool bEnableFogAnimation = true;
    const float FogPulseSpeed = 0.3f;
    const float FogPulseAmplitude = 0.02f;
};
