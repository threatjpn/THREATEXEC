#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChangeLocationManager.generated.h"

class AActor;
class UPrimitiveComponent;
class ULightComponent;

USTRUCT()
struct FChangeLocationVariantGroup
{
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<AActor> RootActor = nullptr;

    UPROPERTY()
    TArray<TObjectPtr<AActor>> Actors;
};

UCLASS()
class THREATEXEC_API AChangeLocationManager : public AActor
{
    GENERATED_BODY()

public:
    AChangeLocationManager();

    UFUNCTION(BlueprintCallable, Category = "Change Location")
    void RefreshVariantCache();

    UFUNCTION(BlueprintCallable, Category = "Change Location")
    void SwitchToVariant(FName VariantID);

    UFUNCTION(BlueprintCallable, Category = "Change Location")
    FName GetCurrentVariantID() const { return CurrentVariantID; }

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Change Location")
    FString VariantRootPrefix = TEXT("CL_");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Change Location")
    FName InitialVariantID = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Change Location")
    bool bApplyInitialVariantOnBeginPlay = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Change Location")
    bool bHideInactiveVariantsOnBeginPlay = true;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Change Location", meta = (AllowPrivateAccess = "true"))
    FName CurrentVariantID = NAME_None;

    UPROPERTY()
    TMap<FName, FChangeLocationVariantGroup> VariantGroups;

    void GatherAttachedActorsRecursive(AActor* RootActor, TArray<AActor*>& OutActors) const;
    void SetActorVariantActive(AActor* Actor, bool bActive) const;
    bool TryGetVariantIDFromRootActor(const AActor* Actor, FName& OutVariantID) const;
};