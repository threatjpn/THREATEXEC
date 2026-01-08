#include "TEButtonWidget.h"
#include "Materials/MaterialInstanceDynamic.h"

void UTEButtonWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Btn)
	{
		Btn->OnHovered.AddDynamic(this, &UTEButtonWidget::HandleHovered);
		Btn->OnUnhovered.AddDynamic(this, &UTEButtonWidget::HandleUnhovered);
		Btn->OnClicked.AddDynamic(this, &UTEButtonWidget::HandleClicked);
	}

	ApplyVisuals();
}

void UTEButtonWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	ApplyVisuals();
}

void UTEButtonWidget::ApplyVisuals()
{
	if (!Img_Main)
	{
		return;
	}

	// Ensure images do not block input
	Img_Main->SetVisibility(ESlateVisibility::HitTestInvisible);

	if (!MID_Button)
	{
		if (ButtonMaterial)
		{
			MID_Button = UMaterialInstanceDynamic::Create(ButtonMaterial, this);
			Img_Main->SetBrushFromMaterial(MID_Button);
		}
		else
		{
			MID_Button = Img_Main->GetDynamicMaterial();
		}
	}

	if (MID_Button)
	{
		if (ButtonTexture)
		{
			MID_Button->SetTextureParameterValue(TEXT("ButtonTex"), ButtonTexture);
		}

		MID_Button->SetVectorParameterValue(TEXT("GlowColor"), GlowColor);
		MID_Button->SetScalarParameterValue(TEXT("GlowRadius"), GlowRadius);
		MID_Button->SetScalarParameterValue(TEXT("GlowSoftness"), GlowSoftness);

		SetGlowStrength(0.0f);
	}

	if (Img_Reflection)
	{
		Img_Reflection->SetVisibility(ESlateVisibility::HitTestInvisible);

		if (ButtonTexture)
		{
			Img_Reflection->SetBrushFromTexture(ButtonTexture, false);
		}

		FLinearColor RefC = Img_Reflection->ColorAndOpacity;
		RefC.A = ReflectionStrength;
		Img_Reflection->SetColorAndOpacity(RefC);
	}

	if (ReflectionContainer)
	{
		ReflectionContainer->SetVisibility(
			bShowReflection ? ESlateVisibility::Visible : ESlateVisibility::Collapsed
		);
	}

	ApplyEnabledState();
}

void UTEButtonWidget::ApplyEnabledState()
{
	if (Btn)
	{
		Btn->SetIsEnabled(bTEEnabled);
	}

	const float TargetOpacity = bTEEnabled ? 1.0f : DisabledOpacity;

	if (Img_Main)
	{
		FLinearColor Tint = bTEEnabled ? FLinearColor::White : DisabledTint;
		Tint.A *= TargetOpacity;
		Img_Main->SetColorAndOpacity(Tint);
	}

	if (Img_Reflection)
	{
		FLinearColor RefTint = FLinearColor::White;
		RefTint.A = bTEEnabled
			? ReflectionStrength
			: ReflectionStrength * DisabledOpacity;
		Img_Reflection->SetColorAndOpacity(RefTint);
	}

	if (!bTEEnabled && !bAllowGlowWhenDisabled)
	{
		SetGlowStrength(0.0f);
	}

	if (MID_Button && bUseMaterialDisabledParam)
	{
		MID_Button->SetScalarParameterValue(
			TEXT("IsDisabled"),
			bTEEnabled ? 0.0f : 1.0f
		);
	}
}

void UTEButtonWidget::SetGlowStrength(float Strength)
{
	if (MID_Button)
	{
		MID_Button->SetScalarParameterValue(TEXT("GlowStrength"), Strength);
	}
}

void UTEButtonWidget::SetTEEnabled(bool bEnabled)
{
	bTEEnabled = bEnabled;
	ApplyEnabledState();
}

void UTEButtonWidget::HandleHovered()
{
	if (!bTEEnabled && !bAllowGlowWhenDisabled)
	{
		return;
	}

	if (bGlowOnHover)
	{
		SetGlowStrength(1.0f);
	}
}

void UTEButtonWidget::HandleUnhovered()
{
	SetGlowStrength(0.0f);
}

void UTEButtonWidget::HandleClicked()
{
	if (!bTEEnabled)
	{
		return;
	}

	OnTEClicked.Broadcast();
}
