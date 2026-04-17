// cloudSpawner.cpp
#include "CloudSpawner.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/VolumetricCloudComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Asteroid.h"

// Spawns a new cloud actor into the world
void UCloudSpawner::SpawnCloud(UObject* WorldContextObject)
{
    // Make sure context object is valid
    if (!WorldContextObject) return;

    // Get the game world from the context object
    UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);

    // Spawn placeholder Asteroid actor to act as the base for the cloud
    AActor* CloudActor = World->SpawnActor<AAsteroid>();

    // Check if spawned successfully
    if (!CloudActor) return;

    // Attach stock unreal engine cloud to actor (hidden inside asteroid, legacy)
    UVolumetricCloudComponent* CloudComp =
        NewObject<UVolumetricCloudComponent>(CloudActor);

    // Register the cloud so the engine starts rendering and updating it
    CloudComp->RegisterComponent();
    // Add the cloud to the actor's instance component list
    CloudActor->AddInstanceComponent(CloudComp);

    // Set the initial location of the actor in the world
    CloudActor->SetActorLocation(FVector(0, 0, 3000));
}