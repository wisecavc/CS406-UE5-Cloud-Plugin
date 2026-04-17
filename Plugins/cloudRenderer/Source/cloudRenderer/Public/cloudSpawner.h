// cloudSpawner.h
#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "CloudSpawner.generated.h"

UCLASS()
class CLOUDRENDERER_API UCloudSpawner : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
    static void SpawnCloud(UObject* WorldContextObject);
};