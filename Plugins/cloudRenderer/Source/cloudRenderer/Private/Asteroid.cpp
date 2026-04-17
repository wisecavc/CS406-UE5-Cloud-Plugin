// Asteroid.cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "Asteroid.h"
#include "UObject/ConstructorHelpers.h"
#include "ProceduralMeshComponent.h"
#include "Engine/World.h"

// Filepath to materials in content drawer
namespace
{
    static constexpr TCHAR AsteroidMaterialPathPrimary[] = TEXT("/Game/Material/M_Asteroid.M_Asteroid");
    static constexpr TCHAR AsteroidMaterialPathSecondary[] = TEXT("/Game/Materials/M_Asteroid.M_Asteroid");
}

// Constructor
AAsteroid::AAsteroid()
{
    // Disable per-frame update to save performance since the asteroid is static
    PrimaryActorTick.bCanEverTick = false;

    // Create procedural mesh component and make it the root
    Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;

    // Attempt to find and load the asteroid material
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> AsteroidMaterialRef(AsteroidMaterialPathPrimary);
    if (AsteroidMaterialRef.Succeeded())
    {
        AsteroidMaterial = AsteroidMaterialRef.Object;
    }
    else
    {
        static ConstructorHelpers::FObjectFinder<UMaterialInterface> AsteroidMaterialRefFallback(AsteroidMaterialPathSecondary);
        if (AsteroidMaterialRefFallback.Succeeded())
        {
            AsteroidMaterial = AsteroidMaterialRefFallback.Object;
        }
        else
        {
            static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultMaterial(
                TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")
            );
            if (DefaultMaterial.Succeeded())
            {
                AsteroidMaterial = DefaultMaterial.Object;
            }
        }
    }
}

// Called when the game starts or when spawned
void AAsteroid::BeginPlay()
{
    Super::BeginPlay();

    // Load the material dynamically if it wasn't found in the constructor
    if (!AsteroidMaterial)
    {
        AsteroidMaterial = LoadObject<UMaterialInterface>(nullptr, AsteroidMaterialPathPrimary);
    }

    // Generate asteroid geometry once at startup
    GenerateMesh();
}

// Generates the 3D procedural geometry for the asteroid
void AAsteroid::GenerateMesh()
{
    // Vertex and triangle buffers
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector2D> UVs;

    // Controls mesh density and radius size
    int Resolution = 24;
    float Radius = 420.0f;

    // Per-asteroid randomization
    FRandomStream ShapeRng(FMath::Rand());

    const FVector WarpOffsetA(
        ShapeRng.FRandRange(-20.0f, 20.0f),
        ShapeRng.FRandRange(-20.0f, 20.0f),
        ShapeRng.FRandRange(-20.0f, 20.0f)
    );
    const FVector WarpOffsetB(
        ShapeRng.FRandRange(-20.0f, 20.0f),
        ShapeRng.FRandRange(-20.0f, 20.0f),
        ShapeRng.FRandRange(-20.0f, 20.0f)
    );
    const FVector WarpOffsetC(
        ShapeRng.FRandRange(-20.0f, 20.0f),
        ShapeRng.FRandRange(-20.0f, 20.0f),
        ShapeRng.FRandRange(-20.0f, 20.0f)
    );

    // Lambda builds a face of a cube, projected onto sphere for randomized asteroid topology
    auto AddFace = [&](FVector localUp)
        {
            // Generate two face axes
            FVector axisA = FVector::CrossProduct(localUp, FVector(0, 1, 0));
            if (axisA.SizeSquared() < KINDA_SMALL_NUMBER)
            {
                axisA = FVector::CrossProduct(localUp, FVector(1, 0, 0));
            }
            axisA.Normalize();

            FVector axisB = FVector::CrossProduct(localUp, axisA);

            int startIndex = Vertices.Num();

            // Generate grid of vertices on this face
            for (int y = 0; y <= Resolution; y++)
            {
                for (int x = 0; x <= Resolution; x++)
                {
                    // Normalize coordinates
                    FVector2D percent = FVector2D(x, y) / Resolution;

                    // Map grid point onto cube face
                    FVector pointOnCube =
                        localUp +
                        (percent.X - 0.5f) * 2 * axisA +
                        (percent.Y - 0.5f) * 2 * axisB;

                    // Project cube point onto sphere
                    FVector pointOnSphere = pointOnCube.GetSafeNormal();

                    // ===== ABSTRACT SHAPE + NOISE LAYERS =====

                    // Bias the sphere into an ellipsoid
                    float axisScale =
                        1.0f +
                        (pointOnSphere.X * 0.20f) +
                        (pointOnSphere.Y * -0.13f) +
                        (pointOnSphere.Z * 0.17f);
                    float baseRadius = Radius * axisScale;

                    // Domain warp to scramble noise
                    FVector warp = FVector(
                        FMath::PerlinNoise3D(pointOnSphere * 0.9f + WarpOffsetA),
                        FMath::PerlinNoise3D(pointOnSphere * 1.0f + WarpOffsetB),
                        FMath::PerlinNoise3D(pointOnSphere * 1.1f + WarpOffsetC)
                    ) * 0.32f;

                    FVector warpedSample = pointOnSphere * 1.35f + warp;

                    // Noise macros
                    float macroNoise = FMath::PerlinNoise3D(warpedSample * 0.75f);
                    float ridgeNoise = 1.0f - FMath::Abs(FMath::PerlinNoise3D(warpedSample * 1.8f));
                    ridgeNoise *= ridgeNoise;
                    float mediumNoise = FMath::PerlinNoise3D(warpedSample * 2.8f);
                    float detailNoise = FMath::PerlinNoise3D(warpedSample * 6.2f) * 0.22f;

                    // Carve craters
                    float craterMask = FMath::PerlinNoise3D(warpedSample * 2.2f);
                    craterMask = FMath::Clamp((craterMask + 0.62f) / 0.38f, 0.0f, 1.0f);
                    craterMask = FMath::SmoothStep(0.0f, 1.0f, craterMask);

                    float totalNoise =
                        macroNoise * 44.0f +
                        ridgeNoise * 22.0f +
                        mediumNoise * 16.0f +
                        detailNoise * 4.5f -
                        craterMask * 10.0f;

                    // Clamp to avoid self-intersections and extreme thin triangles
                    totalNoise = FMath::Clamp(totalNoise, -58.0f, 84.0f);

                    // Apply displacement acrosss surface by referencing sphere radius
                    FVector finalPoint = pointOnSphere * (baseRadius + totalNoise);

                    // Smooth to reduce artifacts
                    finalPoint = FMath::Lerp(pointOnSphere * baseRadius, finalPoint, 0.87f);

                    Vertices.Add(finalPoint);

                    UVs.Add(percent);
                }
            }

            // Build two triangles per quad
            for (int y = 0; y < Resolution; y++)
            {
                for (int x = 0; x < Resolution; x++)
                {
                    int i = startIndex + x + y * (Resolution + 1);

                    // Triangle 1
                    Triangles.Add(i);
                    Triangles.Add(i + Resolution + 1);
                    Triangles.Add(i + 1);

                    // Triangle 2
                    Triangles.Add(i + 1);
                    Triangles.Add(i + Resolution + 1);
                    Triangles.Add(i + Resolution + 2);
                }
            }
        };

    // Build all 6 faces of the cube
    AddFace(FVector::UpVector);
    AddFace(-FVector::UpVector);
    AddFace(FVector::RightVector);
    AddFace(-FVector::RightVector);
    AddFace(FVector::ForwardVector);
    AddFace(-FVector::ForwardVector);

    // Merge seam vertices shared by cube faces
    TMap<FIntVector, int32> WeldMap;
    TArray<FVector> WeldedVertices;
    TArray<FVector2D> WeldedUVs;
    TArray<int32> WeldCounts;
    TArray<int32> Remap;
    Remap.SetNum(Vertices.Num());

    const float WeldScale = 100000.0f;
    for (int32 i = 0; i < Vertices.Num(); ++i)
    {
        const FVector Dir = Vertices[i].GetSafeNormal();
        const FIntVector Key(
            FMath::RoundToInt(Dir.X * WeldScale),
            FMath::RoundToInt(Dir.Y * WeldScale),
            FMath::RoundToInt(Dir.Z * WeldScale)
        );

        if (int32* Existing = WeldMap.Find(Key))
        {
            const int32 Idx = *Existing;
            Remap[i] = Idx;
            WeldedVertices[Idx] += Vertices[i];
            WeldedUVs[Idx] += UVs[i];
            WeldCounts[Idx] += 1;
        }
        else
        {
            const int32 NewIdx = WeldedVertices.Add(Vertices[i]);
            WeldedUVs.Add(UVs[i]);
            WeldCounts.Add(1);
            WeldMap.Add(Key, NewIdx);
            Remap[i] = NewIdx;
        }
    }

    for (int32 i = 0; i < WeldedVertices.Num(); ++i)
    {
        const float Count = static_cast<float>(WeldCounts[i]);
        WeldedVertices[i] /= Count;
        WeldedUVs[i] /= Count;
    }

    for (int32 i = 0; i < Triangles.Num(); ++i)
    {
        Triangles[i] = Remap[Triangles[i]];
    }

    Vertices = MoveTemp(WeldedVertices);
    UVs = MoveTemp(WeldedUVs);

    // Smooth radial displacement over connections to remove jagged triangular edges
    TArray<TArray<int32>> VertexNeighbors;
    VertexNeighbors.SetNum(Vertices.Num());

    auto AddNeighbor = [&](int32 A, int32 B)
        {
            VertexNeighbors[A].AddUnique(B);
            VertexNeighbors[B].AddUnique(A);
        };

    for (int32 i = 0; i + 2 < Triangles.Num(); i += 3)
    {
        const int32 i0 = Triangles[i];
        const int32 i1 = Triangles[i + 1];
        const int32 i2 = Triangles[i + 2];

        AddNeighbor(i0, i1);
        AddNeighbor(i1, i2);
        AddNeighbor(i2, i0);
    }

    TArray<float> Radii;
    TArray<float> NextRadii;
    Radii.SetNum(Vertices.Num());
    NextRadii.SetNum(Vertices.Num());

    for (int32 i = 0; i < Vertices.Num(); ++i)
    {
        Radii[i] = Vertices[i].Size();
    }

    const int32 SmoothIterations = 2;
    const float SmoothAlpha = 0.24f;
    const float MaxStepPerIteration = 6.0f;

    for (int32 Iter = 0; Iter < SmoothIterations; ++Iter)
    {
        for (int32 i = 0; i < Vertices.Num(); ++i)
        {
            const TArray<int32>& Neighbors = VertexNeighbors[i];
            if (Neighbors.Num() == 0)
            {
                NextRadii[i] = Radii[i];
                continue;
            }

            float Sum = 0.0f;
            for (int32 NeighborIndex : Neighbors)
            {
                Sum += Radii[NeighborIndex];
            }

            const float Average = Sum / static_cast<float>(Neighbors.Num());
            const float Target = FMath::Lerp(Radii[i], Average, SmoothAlpha);

            NextRadii[i] = FMath::Clamp(
                Target,
                Radii[i] - MaxStepPerIteration,
                Radii[i] + MaxStepPerIteration
            );
        }

        Swap(Radii, NextRadii);
    }

    const float MinRadius = Radius * 0.58f;
    const float MaxRadius = Radius * 1.36f;
    for (int32 i = 0; i < Vertices.Num(); ++i)
    {
        const FVector Dir = Vertices[i].GetSafeNormal();
        const float SmoothedRadius = FMath::Clamp(Radii[i], MinRadius, MaxRadius);
        Vertices[i] = Dir * SmoothedRadius;
    }

    // ===== NORMAL GENERATION =====
    // Compute per-vertex normals from triangle faces

    TArray<FVector> Normals;
    TArray<FProcMeshTangent> Tangents;
    Normals.Init(FVector::ZeroVector, Vertices.Num());

    for (int32 t = 0; t + 2 < Triangles.Num(); t += 3)
    {
        const int32 i0 = Triangles[t];
        const int32 i1 = Triangles[t + 1];
        const int32 i2 = Triangles[t + 2];

        const FVector& v0 = Vertices[i0];
        const FVector& v1 = Vertices[i1];
        const FVector& v2 = Vertices[i2];

        FVector faceNormal = FVector::CrossProduct(v1 - v0, v2 - v0);
        const FVector faceCenter = (v0 + v1 + v2) / 3.0f;
        if (FVector::DotProduct(faceNormal, faceCenter) < 0.0f)
        {
            faceNormal *= -1.0f;
        }

        Normals[i0] += faceNormal;
        Normals[i1] += faceNormal;
        Normals[i2] += faceNormal;
    }

    for (int32 i = 0; i < Normals.Num(); ++i)
    {
        Normals[i] = Normals[i].GetSafeNormal();
        if (Normals[i].IsNearlyZero())
        {
            Normals[i] = Vertices[i].GetSafeNormal();
        }
    }

    // Create mesh section
    Mesh->CreateMeshSection(
        0,
        Vertices,
        Triangles,
        Normals,
        UVs,
        TArray<FColor>(),
        Tangents,
        true
    );

    // Apply material
    Mesh->SetMaterial(0, AsteroidMaterial);
}

// Static helper function to spawn a new asteroid into the world
AAsteroid* AAsteroid::SpawnAsteroid(UWorld* World)
{
    if (!World) return nullptr;

    FActorSpawnParameters Params;
    // Spawn the asteroid at the origin
    return World->SpawnActor<AAsteroid>(AAsteroid::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
}