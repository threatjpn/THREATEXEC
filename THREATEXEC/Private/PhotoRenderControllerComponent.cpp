/**
 * File: PhotoRenderControllerComponent.cpp
 * Summary: Implementation of runtime photo capture, long-exposure accumulation, texture creation and file export helpers.
 * Note: Comments added for maintainability only. Behaviour and public API remain unchanged.
 */

#include "PhotoRenderControllerComponent.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

/** Enables ticking because long-exposure accumulation is time-based. */

UPhotoRenderControllerComponent::UPhotoRenderControllerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

/** Resets any persisted accumulation state at component startup. */

void UPhotoRenderControllerComponent::BeginPlay()
{
	Super::BeginPlay();
	ResetAccumulation();
}

void UPhotoRenderControllerComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bLongExposureActive)
	{
		return;
	}

	LongExposureElapsed += DeltaTime;
	TimeUntilNextSample -= DeltaTime;

	const bool bReachedMaxDuration =
		(MaxLongExposureSeconds > 0.0f) && (LongExposureElapsed >= MaxLongExposureSeconds);

	while (bLongExposureActive && TimeUntilNextSample <= 0.0f)
	{
		AccumulateSample();
		TimeUntilNextSample += (1.0f / FMath::Max(LongExposureSampleRate, 1.0f));

		if (bReachedMaxDuration)
		{
			StopLongExposureAndSave();
			break;
		}
	}
}

/** Verifies that the capture component and render target configuration is valid before capture. */

bool UPhotoRenderControllerComponent::ValidateCaptureSetup() const
{
	if (!CaptureComponent)
	{
		LogPhotoMessage(TEXT("PhotoRenderController: CaptureComponent is not assigned."));
		return false;
	}

	if (!CaptureRenderTarget)
	{
		LogPhotoMessage(TEXT("PhotoRenderController: CaptureRenderTarget is not assigned."));
		return false;
	}

	if (CaptureComponent->TextureTarget != CaptureRenderTarget)
	{
		LogPhotoMessage(TEXT("PhotoRenderController: CaptureComponent TextureTarget does not match CaptureRenderTarget."));
		return false;
	}

	return true;
}

/** Clears all long-exposure state so a fresh accumulation can begin safely. */

void UPhotoRenderControllerComponent::ResetAccumulation()
{
	bLongExposureActive = false;
	LongExposureElapsed = 0.0f;
	TimeUntilNextSample = 0.0f;
	AccumulatedFrameCount = 0;
	AccumulatedPixels.Reset();
}

bool UPhotoRenderControllerComponent::CaptureRenderTargetPixels(
	TArray<FColor>& OutPixels,
	int32& OutWidth,
	int32& OutHeight) const
{
	OutPixels.Reset();
	OutWidth = 0;
	OutHeight = 0;

	if (!ValidateCaptureSetup())
	{
		return false;
	}

	CaptureComponent->CaptureScene();

	FTextureRenderTargetResource* RTResource =
		CaptureRenderTarget->GameThread_GetRenderTargetResource();

	if (!RTResource)
	{
		LogPhotoMessage(TEXT("PhotoRenderController: Failed to get render target resource."));
		return false;
	}

	OutWidth = CaptureRenderTarget->SizeX;
	OutHeight = CaptureRenderTarget->SizeY;

	FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
	ReadFlags.SetLinearToGamma(false);

	if (!RTResource->ReadPixels(OutPixels, ReadFlags))
	{
		LogPhotoMessage(TEXT("PhotoRenderController: ReadPixels failed."));
		return false;
	}

	if (OutPixels.Num() != OutWidth * OutHeight)
	{
		LogPhotoMessage(TEXT("PhotoRenderController: Pixel count mismatch after capture."));
		return false;
	}

	if (bFlipVerticallyBeforeSave)
	{
		TArray<FColor> Flipped;
		Flipped.SetNumUninitialized(OutPixels.Num());

		for (int32 Y = 0; Y < OutHeight; ++Y)
		{
			const int32 SrcY = OutHeight - 1 - Y;
			const int32 DstRow = Y * OutWidth;
			const int32 SrcRow = SrcY * OutWidth;

			for (int32 X = 0; X < OutWidth; ++X)
			{
				Flipped[DstRow + X] = OutPixels[SrcRow + X];
			}
		}

		OutPixels = MoveTemp(Flipped);
	}

	return true;
}

/** Captures the current render target and adds its pixels into the accumulation buffer. */

void UPhotoRenderControllerComponent::AccumulateSample()
{
	TArray<FColor> Pixels;
	int32 Width = 0;
	int32 Height = 0;

	if (!CaptureRenderTargetPixels(Pixels, Width, Height))
	{
		LogPhotoMessage(TEXT("PhotoRenderController: AccumulateSample skipped due to capture failure."));
		return;
	}

	const int32 ExpectedPixelCount = Width * Height;

	if (AccumulatedPixels.Num() == 0)
	{
		AccumulatedPixels.SetNumZeroed(ExpectedPixelCount);
	}
	else if (AccumulatedPixels.Num() != ExpectedPixelCount)
	{
		LogPhotoMessage(TEXT("PhotoRenderController: Render target size changed during long exposure. Resetting accumulation."));
		ResetAccumulation();
		return;
	}

	for (int32 Index = 0; Index < ExpectedPixelCount; ++Index)
	{
		const FLinearColor LinearSample = FLinearColor(Pixels[Index]);
		AccumulatedPixels[Index] += LinearSample;
	}

	++AccumulatedFrameCount;

	if (bVerboseLogging)
	{
		LogPhotoMessage(FString::Printf(
			TEXT("PhotoRenderController: accumulated frame %d"),
			AccumulatedFrameCount));
	}
}

/** Creates a transient texture from raw pixel data for previewing or forwarding to other systems. */

bool UPhotoRenderControllerComponent::BuildTextureFromPixels(
	const TArray<FColor>& Pixels,
	int32 Width,
	int32 Height,
	UTexture2D*& OutTexture) const
{
	OutTexture = nullptr;

	if (Pixels.Num() != Width * Height || Width <= 0 || Height <= 0)
	{
		return false;
	}

	UTexture2D* NewTexture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
	if (!NewTexture)
	{
		return false;
	}

#if WITH_EDITORONLY_DATA
	NewTexture->MipGenSettings = TMGS_NoMipmaps;
#endif

	NewTexture->SRGB = true;

	FTexture2DMipMap& Mip = NewTexture->GetPlatformData()->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Data, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
	Mip.BulkData.Unlock();

	NewTexture->UpdateResource();

	OutTexture = NewTexture;
	return true;
}

bool UPhotoRenderControllerComponent::BuildTextureFromAccumulation(UTexture2D*& OutTexture)
{
	OutTexture = nullptr;

	if (AccumulatedFrameCount <= 0 || AccumulatedPixels.Num() == 0)
	{
		LogPhotoMessage(TEXT("PhotoRenderController: No accumulated frames to build texture from."));
		return false;
	}

	if (!CaptureRenderTarget)
	{
		return false;
	}

	const int32 Width = CaptureRenderTarget->SizeX;
	const int32 Height = CaptureRenderTarget->SizeY;

	if (AccumulatedPixels.Num() != Width * Height)
	{
		LogPhotoMessage(TEXT("PhotoRenderController: Accumulation buffer size mismatch."));
		return false;
	}

	TArray<FColor> FinalPixels;
	FinalPixels.SetNumUninitialized(AccumulatedPixels.Num());

	const float InvFrameCount = 1.0f / static_cast<float>(AccumulatedFrameCount);

	for (int32 Index = 0; Index < AccumulatedPixels.Num(); ++Index)
	{
		FLinearColor Average = AccumulatedPixels[Index] * InvFrameCount;
		Average.A = 1.0f;
		FinalPixels[Index] = Average.ToFColor(true);
	}

	return BuildTextureFromPixels(FinalPixels, Width, Height, OutTexture);
}

bool UPhotoRenderControllerComponent::SavePixelsToPng(
	const TArray<FColor>& Pixels,
	int32 Width,
	int32 Height,
	FString& OutSavedPath) const
{
	OutSavedPath.Reset();

	if (Pixels.Num() != Width * Height || Width <= 0 || Height <= 0)
	{
		LogPhotoMessage(TEXT("PhotoRenderController: Invalid pixel buffer for PNG save."));
		return false;
	}

	const FString FullPath = BuildTimestampedOutputPath();
	const FString Directory = FPaths::GetPath(FullPath);

	IFileManager::Get().MakeDirectory(*Directory, true);

	TArray64<uint8> PngData64;
	FImageUtils::PNGCompressImageArray(Width, Height, Pixels, PngData64);

	if (PngData64.Num() == 0)
	{
		LogPhotoMessage(TEXT("PhotoRenderController: PNG compression failed."));
		return false;
	}

	TArray<uint8> PngData;
	PngData.Append(PngData64.GetData(), static_cast<int32>(PngData64.Num()));

	if (!FFileHelper::SaveArrayToFile(PngData, *FullPath))
	{
		LogPhotoMessage(FString::Printf(
			TEXT("PhotoRenderController: Failed to save PNG to %s"),
			*FullPath));
		return false;
	}

	OutSavedPath = FullPath;
	LogPhotoMessage(FString::Printf(
		TEXT("PhotoRenderController: saved image to %s"),
		*OutSavedPath));
	return true;
}

FString UPhotoRenderControllerComponent::BuildTimestampedOutputPath() const
{
	const FString SafePrefix = FilePrefix.IsEmpty() ? TEXT("TE_Photo") : FilePrefix;
	const FString Timestamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
	const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), OutputSubfolder);
	return FPaths::Combine(Directory, FString::Printf(TEXT("%s_%s.png"), *SafePrefix, *Timestamp));
}

void UPhotoRenderControllerComponent::LogPhotoMessage(const FString& Message) const
{
	if (!bVerboseLogging)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("%s"), *Message);
}

bool UPhotoRenderControllerComponent::RenderHighQualityPhoto()
{
	TArray<FColor> Pixels;
	int32 Width = 0;
	int32 Height = 0;

	if (!CaptureRenderTargetPixels(Pixels, Width, Height))
	{
		return false;
	}

	UTexture2D* BuiltTexture = nullptr;
	if (!BuildTextureFromPixels(Pixels, Width, Height, BuiltTexture))
	{
		LogPhotoMessage(TEXT("PhotoRenderController: Failed to build texture from HQ capture."));
		return false;
	}

	FString SavedPath;
	if (!SavePixelsToPng(Pixels, Width, Height, SavedPath))
	{
		return false;
	}

	LastCapturedTexture = BuiltTexture;
	LastSavedFilePath = SavedPath;
	return true;
}

void UPhotoRenderControllerComponent::StartLongExposure()
{
	if (!ValidateCaptureSetup())
	{
		return;
	}

	ResetAccumulation();
	bLongExposureActive = true;
	TimeUntilNextSample = 0.0f;

	LogPhotoMessage(TEXT("PhotoRenderController: long exposure started."));
}

bool UPhotoRenderControllerComponent::StopLongExposureAndSave()
{
	if (!bLongExposureActive && AccumulatedFrameCount <= 0)
	{
		LogPhotoMessage(TEXT("PhotoRenderController: StopLongExposureAndSave called with no active exposure."));
		return false;
	}

	bLongExposureActive = false;

	UTexture2D* BuiltTexture = nullptr;
	if (!BuildTextureFromAccumulation(BuiltTexture))
	{
		return false;
	}

	const int32 Width = CaptureRenderTarget ? CaptureRenderTarget->SizeX : 0;
	const int32 Height = CaptureRenderTarget ? CaptureRenderTarget->SizeY : 0;

	TArray<FColor> FinalPixels;
	FinalPixels.SetNumUninitialized(AccumulatedPixels.Num());

	const float InvFrameCount = 1.0f / static_cast<float>(FMath::Max(AccumulatedFrameCount, 1));

	for (int32 Index = 0; Index < AccumulatedPixels.Num(); ++Index)
	{
		FLinearColor Average = AccumulatedPixels[Index] * InvFrameCount;
		Average.A = 1.0f;
		FinalPixels[Index] = Average.ToFColor(true);
	}

	FString SavedPath;
	if (!SavePixelsToPng(FinalPixels, Width, Height, SavedPath))
	{
		return false;
	}

	LastCapturedTexture = BuiltTexture;
	LastSavedFilePath = SavedPath;

	LogPhotoMessage(FString::Printf(
		TEXT("PhotoRenderController: long exposure saved with %d frames."),
		AccumulatedFrameCount));

	return true;
}

void UPhotoRenderControllerComponent::CancelLongExposure()
{
	ResetAccumulation();
	LogPhotoMessage(TEXT("PhotoRenderController: long exposure cancelled."));
}

bool UPhotoRenderControllerComponent::SaveLastPhotoToGallery()
{
	if (!LastCapturedTexture)
	{
		LogPhotoMessage(TEXT("PhotoRenderController: no LastCapturedTexture available for gallery save."));
		return false;
	}

	const FString SuggestedName = FPaths::GetBaseFilename(LastSavedFilePath);
	return BP_SaveLastPhotoToPluginGallery(LastCapturedTexture, SuggestedName);
}

void UPhotoRenderControllerComponent::OpenPluginGallery()
{
	BP_OpenPluginGallery();
}