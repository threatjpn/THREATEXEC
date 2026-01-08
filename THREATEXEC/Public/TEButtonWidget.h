#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "TEButtonWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTEButtonClicked);

UCLASS()
class THREATEXEC_API UTEButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Click event to bind in parent widgets
	UPROPERTY(BlueprintAssignable, Category = "TEButton")
	FTEButtonClicked OnTEClicked;

	// Visual configuration (set per instance)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TEButton")
	TObjectPtr<UTexture2D> ButtonTexture = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TEButton")
	FLinearColor GlowColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TEButton")
	bool bGlowOnHover = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TEButton")
	bool bShowReflection = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TEButton", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReflectionStrength = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TEButton")
	float GlowRadius = 0.002f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TEButton")
	float GlowSoftness = 2.0f;

	// Disabled state (custom, avoids engine conflict)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TEButton")
	bool bTEEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TEButton", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DisabledOpacity = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TEButton")
	FLinearColor DisabledTint = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TEButton")
	bool bAllowGlowWhenDisabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TEButton")
	bool bUseMaterialDisabledParam = true;

	UFUNCTION(BlueprintCallable, Category = "TEButton")
	void SetTEEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "TEButton")
	bool IsTEEnabled() const { return bTEEnabled; }

protected:
	// Required named widgets in the Widget Blueprint
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_Main = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_Reflection = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> ReflectionContainer = nullptr;

	// Assign M_UI_ButtonGlow in BP defaults
	UPROPERTY(EditDefaultsOnly, Category = "TEButton")
	TObjectPtr<UMaterialInterface> ButtonMaterial = nullptr;

	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MID_Button = nullptr;

	void ApplyVisuals();
	void ApplyEnabledState();
	void SetGlowStrength(float Strength);

	UFUNCTION()
	void HandleHovered();

	UFUNCTION()
	void HandleUnhovered();

	UFUNCTION()
	void HandleClicked();
};
