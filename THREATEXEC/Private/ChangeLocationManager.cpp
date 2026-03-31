#include "ChangeLocationManager.h"

#include "Components/LightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

AChangeLocationManager::AChangeLocationManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AChangeLocationManager::BeginPlay()
{
    Super::BeginPlay();

    RefreshVariantCache();

    if (bApplyInitialVariantOnBeginPlay && InitialVariantID != NAME_None)
    {
        SwitchToVariant(InitialVariantID);
    }
}

void AChangeLocationManager::RefreshVariantCache()
{
    VariantActorMap.Empty();

    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), AllActors);

    for (AActor* Actor : AllActors)
    {
        if (!Actor || Actor == this)
        {
            continue;
        }

        FName VariantID = NAME_None;
        if (!TryExtractVariantIDFromActor(Actor, VariantID))
        {
            continue;
        }

        VariantActorMap.FindOrAdd(VariantID).Add(Actor);
    }

    for (const TPair<FName, TArray<TObjectPtr<AActor>>>& Pair : VariantActorMap)
    {
        UE_LOG(LogTemp, Log, TEXT("ChangeLocationManager: Variant '%s' has %d actor(s)"),
            *Pair.Key.ToString(),
            Pair.Value.Num());
    }
}

bool AChangeLocationManager::TryExtractVariantIDFromActor(const AActor* Actor, FName& OutVariantID) const
{
    if (!Actor)
    {
        return false;
    }

    for (const FName& Tag : Actor->Tags)
    {
        const FString TagString = Tag.ToString();

        if (TagString.StartsWith(VariantTagPrefix))
        {
            const FString Suffix = TagString.RightChop(VariantTagPrefix.Len());
            if (!Suffix.IsEmpty())
            {
                OutVariantID = FName(*Suffix);
                return true;
            }
        }
    }

    return false;
}

void AChangeLocationManager::SetActorVariantActive(AActor* Actor, bool bActive)
{
    if (!Actor)
    {
        return;
    }

    Actor->SetActorHiddenInGame(!bActive);
    Actor->SetActorEnableCollision(bActive);
    Actor->SetActorTickEnabled(bActive);

    TArray<UActorComponent*> Components = Actor->GetComponents().Array();
    for (UActorComponent* Component : Components)
    {
        if (!Component)
        {
            continue;
        }

        Component->SetComponentTickEnabled(bActive);

        if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
        {
            Primitive->SetHiddenInGame(!bActive, true);
            Primitive->SetVisibility(bActive, true);
            Primitive->SetCollisionEnabled(bActive ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
        }

        if (ULightComponent* Light = Cast<ULightComponent>(Component))
        {
            Light->SetVisibility(bActive, true);
        }
    }
}

void AChangeLocationManager::SwitchToVariant(FName VariantID)
{
    if (VariantID == NAME_None)
    {
        UE_LOG(LogTemp, Warning, TEXT("ChangeLocationManager: SwitchToVariant called with NAME_None"));
        return;
    }

    if (VariantActorMap.Num() == 0)
    {
        RefreshVariantCache();
    }

    if (!VariantActorMap.Contains(VariantID))
    {
        UE_LOG(LogTemp, Warning, TEXT("ChangeLocationManager: Variant '%s' not found"), *VariantID.ToString());
        return;
    }

    for (TPair<FName, TArray<TObjectPtr<AActor>>>& Pair : VariantActorMap)
    {
        const bool bShouldBeActive = (Pair.Key == VariantID);

        for (AActor* Actor : Pair.Value)
        {
            SetActorVariantActive(Actor, bShouldBeActive);
        }
    }

    CurrentVariantID = VariantID;

    UE_LOG(LogTemp, Log, TEXT("ChangeLocationManager: Switched to variant '%s'"), *CurrentVariantID.ToString());
}