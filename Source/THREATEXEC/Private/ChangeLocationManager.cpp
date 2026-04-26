// ============================================================================
// ChangeLocationManager.cpp
// Controls location changes, camera targets, fades, and scene visibility groups.
// ============================================================================

#include "ChangeLocationManager.h"

#include "Components/ActorComponent.h"
#include "Components/LightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

/** Initialises the manager as a non-ticking actor because updates are event-driven. */

// Sets default components, ticking, collision, and editable values.
AChangeLocationManager::AChangeLocationManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

/** Builds the initial variant cache and optionally applies the configured startup state. */

// Initialises runtime state when play begins.
void AChangeLocationManager::BeginPlay()
{
    Super::BeginPlay();

    RefreshVariantCache();

    if (bApplyInitialVariantOnBeginPlay && InitialVariantID != NAME_None)
    {
        SwitchToVariant(InitialVariantID);
        return;
    }

    if (bHideInactiveVariantsOnBeginPlay && VariantGroups.Num() > 0)
    {
        bool bFirst = true;

        for (TPair<FName, FChangeLocationVariantGroup>& Pair : VariantGroups)
        {
            const bool bShouldBeActive = bFirst;

            for (AActor* Actor : Pair.Value.Actors)
            {
                SetActorVariantActive(Actor, bShouldBeActive);
            }

            if (bFirst)
            {
                CurrentVariantID = Pair.Key;
                bFirst = false;
            }
        }

        if (CurrentVariantID != NAME_None)
        {
            UE_LOG(LogTemp, Log, TEXT("ChangeLocationManager: Defaulted to first detected variant '%s'"), *CurrentVariantID.ToString());
        }
    }
}

/** Scans the world for tagged variant roots and records their attached actor hierarchies. */

// Rebuilds cached output from the current source data.
void AChangeLocationManager::RefreshVariantCache()
{
    VariantGroups.Empty();

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("ChangeLocationManager: No valid world"));
        return;
    }

    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);

    for (AActor* Actor : AllActors)
    {
        if (!Actor || Actor == this)
        {
            continue;
        }

        FName VariantID = NAME_None;
        if (!TryGetVariantIDFromRootActor(Actor, VariantID))
        {
            continue;
        }

        FChangeLocationVariantGroup Group;
        Group.RootActor = Actor;

        TArray<AActor*> GatheredActors;
        GatherAttachedActorsRecursive(Actor, GatheredActors);

        for (AActor* Gathered : GatheredActors)
        {
            Group.Actors.Add(Gathered);
        }

        VariantGroups.Add(VariantID, Group);

        UE_LOG(LogTemp, Log, TEXT("ChangeLocationManager: Registered root '%s' for variant '%s' with %d actor(s)"),
            *Actor->GetName(),
            *VariantID.ToString(),
            Group.Actors.Num());
    }

    if (VariantGroups.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("ChangeLocationManager: No variant roots found with prefix '%s'"), *VariantRootPrefix);
    }
}

// Handles try get variant idfrom root actor.
bool AChangeLocationManager::TryGetVariantIDFromRootActor(const AActor* Actor, FName& OutVariantID) const
{
    if (!Actor)
    {
        return false;
    }

    for (const FName& Tag : Actor->Tags)
    {
        const FString TagString = Tag.ToString();
        if (TagString.StartsWith(VariantRootPrefix))
        {
            OutVariantID = Tag;
            return true;
        }
    }

    return false;
}

// Handles gather attached actors recursive.
void AChangeLocationManager::GatherAttachedActorsRecursive(AActor* RootActor, TArray<AActor*>& OutActors) const
{
    if (!RootActor)
    {
        return;
    }

    if (!OutActors.Contains(RootActor))
    {
        OutActors.Add(RootActor);
    }

    TArray<AActor*> AttachedActors;
    RootActor->GetAttachedActors(AttachedActors);

    for (AActor* ChildActor : AttachedActors)
    {
        if (!ChildActor)
        {
            continue;
        }

        if (!OutActors.Contains(ChildActor))
        {
            OutActors.Add(ChildActor);
        }

        GatherAttachedActorsRecursive(ChildActor, OutActors);
    }
}

// Sets whether a location actor variant is active.
void AChangeLocationManager::SetActorVariantActive(AActor* Actor, bool bActive) const
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

/** Enables the requested variant hierarchy while disabling all other cached variants. */

// Handles switch to variant.
void AChangeLocationManager::SwitchToVariant(FName VariantID)
{
    if (VariantID == NAME_None)
    {
        UE_LOG(LogTemp, Warning, TEXT("ChangeLocationManager: SwitchToVariant called with NAME_None"));
        return;
    }

    if (VariantGroups.Num() == 0)
    {
        RefreshVariantCache();
    }

    FChangeLocationVariantGroup* TargetGroup = VariantGroups.Find(VariantID);
    if (!TargetGroup)
    {
        UE_LOG(LogTemp, Warning, TEXT("ChangeLocationManager: Variant '%s' not found"), *VariantID.ToString());
        return;
    }

    for (TPair<FName, FChangeLocationVariantGroup>& Pair : VariantGroups)
    {
        const bool bShouldBeActive = (Pair.Key == VariantID);

        for (AActor* Actor : Pair.Value.Actors)
        {
            SetActorVariantActive(Actor, bShouldBeActive);
        }
    }

    CurrentVariantID = VariantID;

    UE_LOG(LogTemp, Log, TEXT("ChangeLocationManager: Switched to variant '%s'"), *CurrentVariantID.ToString());
}
