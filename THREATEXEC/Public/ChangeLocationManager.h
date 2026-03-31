#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChangeLocationManager.generated.h"

class AActor;
class UPrimitiveComponent;
class ULightComponent;

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
    FString VariantTagPrefix = TEXT("CL_");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Change Location")
    FName InitialVariantID = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Change Location")
    bool bApplyInitialVariantOnBeginPlay = false;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Change Location", meta = (AllowPrivateAccess = "true"))
    FName CurrentVariantID = NAME_None;

    TMap<FName, TArray<TObjectPtr<AActor>>> VariantActorMap;

    void SetActorVariantActive(AActor* Actor, bool bActive);
    bool TryExtractVariantIDFromActor(const AActor* Actor, FName& OutVariantID) const;
};