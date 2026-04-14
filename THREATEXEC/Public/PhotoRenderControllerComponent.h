#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PhotoRenderControllerComponent.generated.h"

class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UTexture2D;

/**
 * Runtime photo rendering helper.
 *
 * Add this component to the same pawn that owns the photo mode plugin component.
 * Assign a SceneCaptureComponent2D and RenderTarget.
 *
 * Responsibilities:
 * - single high quality render capture
 * - long exposure accumulation
 * - saving PNG files to Project/Saved/Photos
 * - holding the last captured texture
 * - forwarding saves to a Blueprint/plugin gallery bridge
 */
UCLASS(ClassGroup = (Custom), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class THREATEXEC_API UPhotoRenderControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPhotoRenderControllerComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Photo|Capture")
	bool RenderHighQualityPhoto();

	UFUNCTION(BlueprintCallable, Category = "Photo|LongExposure")
	void StartLongExposure();

	UFUNCTION(BlueprintCallable, Category = "Photo|LongExposure")
	bool StopLongExposureAndSave();

	UFUNCTION(BlueprintCallable, Category = "Photo|LongExposure")
	void CancelLongExposure();

	UFUNCTION(BlueprintCallable, Category = "Photo|Gallery")
	bool SaveLastPhotoToGallery();

	UFUNCTION(BlueprintCallable, Category = "Photo|Gallery")
	void OpenPluginGallery();

	UFUNCTION(BlueprintPure, Category = "Photo|LongExposure")
	bool IsLongExposureActive() const { return bLongExposureActive; }

	UFUNCTION(BlueprintPure, Category = "Photo|Capture")
	UTexture2D* GetLastCapturedTexture() const { return LastCapturedTexture; }

	UFUNCTION(BlueprintPure, Category = "Photo|Capture")
	FString GetLastSavedFilePath() const { return LastSavedFilePath; }

	UFUNCTION(BlueprintPure, Category = "Photo|LongExposure")
	int32 GetAccumulatedFrameCount() const { return AccumulatedFrameCount; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo|Setup")
	TObjectPtr<USceneCaptureComponent2D> CaptureComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo|Setup")
	TObjectPtr<UTextureRenderTarget2D> CaptureRenderTarget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo|Output")
	FString OutputSubfolder = TEXT("Photos");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo|Output")
	FString FilePrefix = TEXT("TE_Photo");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo|Output")
	bool bFlipVerticallyBeforeSave = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo|LongExposure", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float LongExposureSampleRate = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo|LongExposure", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxLongExposureSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo|Debug")
	bool bVerboseLogging = true;

	/**
	 * Implement this in the pawn Blueprint to forward the produced texture into your plugin gallery logic.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Photo|PluginBridge")
	bool BP_SaveLastPhotoToPluginGallery(UTexture2D* Texture, const FString& SuggestedName);

	UFUNCTION(BlueprintImplementableEvent, Category = "Photo|PluginBridge")
	void BP_OpenPluginGallery();

private:
	bool bLongExposureActive = false;
	float LongExposureElapsed = 0.0f;
	float TimeUntilNextSample = 0.0f;
	int32 AccumulatedFrameCount = 0;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> LastCapturedTexture = nullptr;

	UPROPERTY(Transient)
	FString LastSavedFilePath;

	TArray<FLinearColor> AccumulatedPixels;

	bool ValidateCaptureSetup() const;
	void ResetAccumulation();

	bool CaptureRenderTargetPixels(TArray<FColor>& OutPixels, int32& OutWidth, int32& OutHeight) const;
	void AccumulateSample();
	bool BuildTextureFromAccumulation(UTexture2D*& OutTexture);
	bool BuildTextureFromPixels(const TArray<FColor>& Pixels, int32 Width, int32 Height, UTexture2D*& OutTexture) const;
	bool SavePixelsToPng(const TArray<FColor>& Pixels, int32 Width, int32 Height, FString& OutSavedPath) const;

	FString BuildTimestampedOutputPath() const;
	void LogPhotoMessage(const FString& Message) const;
};