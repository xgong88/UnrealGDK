#pragma once

#include "SpatialCommonTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "DebugInterface.generated.h"

class UWorld;
class AActor;
class ULayeredLBStrategy;

UCLASS()
class SPATIALGDK_API USpatialGDKDebugInterface : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	static ULayeredLBStrategy* GetLoadBalancingStrategy(UWorld* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Spatial GDK Debug")
	static void AddTag(AActor* Actor, FName Tag);

	UFUNCTION(BlueprintCallable, Category = "Spatial GDK Debug")
	static void RemoveTag(AActor* Actor, FName Tag);

	UFUNCTION(BlueprintCallable, Category = "Spatial GDK Debug")
	static bool IsActorReady(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "Spatial GDK Debug", meta = (WorldContext = "WorldContextObject"))
	static void AddInterestOnTag(UObject* WorldContextObject, FName Tag);

	UFUNCTION(BlueprintCallable, Category = "Spatial GDK Debug", meta = (WorldContext = "WorldContextObject"))
	static void RemoveInterestOnTag(UObject* WorldContextObject, FName Tag);

	UFUNCTION(BlueprintCallable, Category = "Spatial GDK Debug")
	static void KeepActorOnCurrentWorker(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "Spatial GDK Debug", meta = (WorldContext = "WorldContextObject"))
	static void DelegateTagToWorker(UObject* WorldContextObject, FName Tag, int32 WorkerId);

	UFUNCTION(BlueprintCallable, Category = "Spatial GDK Debug", meta = (WorldContext = "WorldContextObject"))
	static void Reset(UObject* WorldContextObject);
};
