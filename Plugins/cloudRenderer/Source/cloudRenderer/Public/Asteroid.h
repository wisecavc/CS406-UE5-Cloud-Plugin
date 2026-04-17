// Asteroid.h
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Asteroid.generated.h"

UCLASS()
class CLOUDRENDERER_API AAsteroid : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAsteroid();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	UPROPERTY(VisibleAnywhere)
	UProceduralMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere)
	UMaterialInterface* AsteroidMaterial;

	UFUNCTION(BlueprintCallable)
	static AAsteroid* SpawnAsteroid(UWorld* World);

	void GenerateMesh();
};
