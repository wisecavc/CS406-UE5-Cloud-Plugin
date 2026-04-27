// AtmosSubSys.cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "AtmosSubSys.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"

// World startup sequences
void UAtmosSubSys::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    bInitializedFog = false;
    bSpawnedFog = false;
    FogActor = nullptr;
    FogComponent = nullptr;
}

// Clean up and nullify tracking pointers
void UAtmosSubSys::Deinitialize()
{
    UWorld* World = GetWorld();

    // If subsystem spawned the fog actor, clean it up.
    if (bSpawnedFog && FogActor && IsValid(FogActor))
    {
        FogActor->Destroy();
    }

    // Fallback cleanup in case pointer was invalidated but managed fog still exists.
    if (World)
    {
        for (TActorIterator<AExponentialHeightFog> It(World); It; ++It)
        {
            AExponentialHeightFog* Candidate = *It;
            if (Candidate && Candidate->Tags.Contains(ManagedFogTag))
            {
                Candidate->Destroy();
            }
        }
    }

    FogActor = nullptr;
    FogComponent = nullptr;
    bInitializedFog = false;
    bSpawnedFog = false;
    
    Super::Deinitialize();
}

bool UAtmosSubSys::ShouldCreateSubsystem(UObject* Outer) const
{
    UWorld* World = Cast<UWorld>(Outer);
    if (!World)
    {
        return false;
    }

    // Runtime-only worlds
    return World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE;
}

void UAtmosSubSys::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    bInitializedFog = SetupFog();
}

// Handles structured world setup logic once
bool UAtmosSubSys::SetupFog()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    if (!World->IsGameWorld())
    {
        return false;
    }

    // If currently tracked actor/component are still valid, keep them and avoid re-scanning.
    if (FogActor && IsValid(FogActor))
    {
        FogComponent = FogActor->GetComponent();
        if (FogComponent)
        {
            return true;
        }
    }

    AExponentialHeightFog* ExistingTaggedFog = nullptr;
    AExponentialHeightFog* ExistingAnyFog = nullptr;

    // Search for an existing managed fog first, otherwise keep first available fog as fallback.
    for (TActorIterator<AExponentialHeightFog> It(World); It; ++It)
    {
        AExponentialHeightFog* Candidate = *It;
        if (!Candidate)
        {
            continue;
        }

        if (!ExistingAnyFog)
        {
            ExistingAnyFog = Candidate;
        }

        if (Candidate->Tags.Contains(ManagedFogTag))
        {
            ExistingTaggedFog = Candidate;
            break;
        }
    }

    FogActor = ExistingTaggedFog ? ExistingTaggedFog : ExistingAnyFog;

    // Spawn only if no previous fog elements existed in the scene natively
    if (!FogActor)
    {
        FogActor = World->SpawnActor<AExponentialHeightFog>();
        if (!FogActor)
        {
            return false;
        }

        FogActor->Tags.AddUnique(ManagedFogTag);
        bSpawnedFog = true;
    }
    else
    {
        bSpawnedFog = FogActor->Tags.Contains(ManagedFogTag);
    }

    // Guard pointer
    if (FogActor)
    {
        FogComponent = FogActor->GetComponent();
        
        if (FogComponent)
        {
            FogComponent->bEnableVolumetricFog = true;

            // Fog structural values
            FogComponent->SetFogDensity(TargetBaseDensity);
            FogComponent->SetFogHeightFalloff(TargetFogFalloff);
            FogComponent->SetStartDistance(TargetStartDistance);
            FogComponent->SetFogMaxOpacity(TargetFogMaxOpacity);

            // Color Overrides
            FogComponent->SetFogInscatteringColor(FogTone);
            FogComponent->SetDirectionalInscatteringColor(FogTone);

            // Volumetrics configuration
            FogComponent->SetVolumetricFogScatteringDistribution(FogScatteringDistribution);
            FogComponent->SetVolumetricFogExtinctionScale(ExtinctionScale);
            FogComponent->SetVolumetricFogDistance(TargetVolumetricDistance);
            FogComponent->SetVolumetricFogAlbedo(FogTone.ToFColor(true));

            return true;
        }
    }

    return false;
}

bool UAtmosSubSys::IsTickable() const
{
    return bEnableFogAnimation && bInitializedFog && FogComponent != nullptr;
}

void UAtmosSubSys::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld())
    {
        return;
    }

    if (!bInitializedFog || !FogComponent)
    {
        return;
    }

    const float Time = World->TimeSeconds;

    // Calculate sinewave for subtle ambient shifting underwater murk.
    const float ExactWaveOffset = FMath::Sin(Time * FogPulseSpeed) * FogPulseAmplitude;
    const float NewDensity = TargetBaseDensity + ExactWaveOffset;

    // Update only when value changes enough to matter.
    if (!FMath::IsNearlyEqual(FogComponent->FogDensity, NewDensity, UE_KINDA_SMALL_NUMBER))
    {
        FogComponent->SetFogDensity(NewDensity);
    }
}

TStatId UAtmosSubSys::GetStatId() const
{
    // Required profile stat mapping
    RETURN_QUICK_DECLARE_CYCLE_STAT(UAtmosSubSys, STATGROUP_Tickables);
}
