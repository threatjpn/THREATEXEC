// ============================================================================
// THREATEXEC_FileUtils.cpp
// Implementation of path & JSON IO helpers.
// ============================================================================

#include "THREATEXEC_FileUtils.h"

#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/PlatformProcess.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Policies/CondensedJsonPrintPolicy.h"

DEFINE_LOG_CATEGORY(LogTHREATEXEC_IO);

// ============================================================================
// PATH HANDLING
// ============================================================================

FString TE_PathUtils::ResolveSavedDir(const FString& UserPath, const FString& DefaultSubfolder)
{
	// Base directory
	FString Result;
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();

	if (UserPath.IsEmpty())
	{
		// Saved/<DefaultSubfolder>
		const FString ProjectSavedPath = FPaths::Combine(FPaths::ProjectSavedDir(), DefaultSubfolder);
		Result = ProjectSavedPath;

#if !WITH_EDITOR
		// Packaged build fallback near executable:
		// <PackageRoot>/Saved/<DefaultSubfolder>
		const FString PackagedSavedPath = FPaths::ConvertRelativePathToFull(
			FPaths::Combine(FPlatformProcess::BaseDir(), TEXT("../../../Saved"), DefaultSubfolder));

		if (PF.DirectoryExists(*PackagedSavedPath))
		{
			Result = PackagedSavedPath;
		}
#endif
	}
	else if (FPaths::IsRelative(UserPath))
	{
		// Saved/<UserPath>
		const FString ProjectSavedPath = FPaths::Combine(FPaths::ProjectSavedDir(), UserPath);
		Result = ProjectSavedPath;

#if !WITH_EDITOR
		// In packaged builds, users commonly drop JSONs next to the packaged executable
		// under <PackageRoot>/Saved. If that directory exists, prefer it for reads/lists.
		// This keeps editor behavior unchanged while improving shipping discoverability.
		const FString PackagedSavedPath = FPaths::ConvertRelativePathToFull(
			FPaths::Combine(FPlatformProcess::BaseDir(), TEXT("../../../Saved"), UserPath));

		if (PF.DirectoryExists(*PackagedSavedPath))
		{
			Result = PackagedSavedPath;
		}
#endif
	}
	else
	{
		// Absolute
		Result = UserPath;
	}

	// Normalise and ensure existence
	Result = FPaths::ConvertRelativePathToFull(Result);

	if (!PF.DirectoryExists(*Result))
	{
		if (!PF.CreateDirectoryTree(*Result))
		{
			UE_LOG(LogTHREATEXEC_IO, Warning, TEXT("Failed to create directory: %s"), *Result);
		}
	}

	return Result;
}

FString TE_PathUtils::ResolveFile(const FString& UserPath, const FString& FileName, const FString& DefaultSubfolder)
{
	FString Clean = FileName;
	FPaths::MakeValidFileName(Clean);

	const FString BaseDir = ResolveSavedDir(UserPath, DefaultSubfolder);
	return FPaths::Combine(BaseDir, Clean);
}

// ============================================================================
// JSON SAVE / LOAD
// ============================================================================

bool TE_FileUtils::SaveJson(const FString& AbsPath, const TSharedRef<FJsonObject>& JsonObject)
{
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();

	// Ensure directory exists
	const FString Dir = FPaths::GetPath(AbsPath);
	if (!PF.DirectoryExists(*Dir))
	{
		if (!PF.CreateDirectoryTree(*Dir))
		{
			UE_LOG(LogTHREATEXEC_IO, Warning, TEXT("SaveJson: Failed to create directory %s"), *Dir);
			return false;
		}
	}

	// Serialize JSON
	FString OutString;
	auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&OutString);
	if (!FJsonSerializer::Serialize(JsonObject, Writer))
	{
		UE_LOG(LogTHREATEXEC_IO, Warning, TEXT("SaveJson: Serialization failed (%s)"), *AbsPath);
		return false;
	}

	// Write file
	if (!FFileHelper::SaveStringToFile(OutString, *AbsPath))
	{
		UE_LOG(LogTHREATEXEC_IO, Warning, TEXT("SaveJson: Failed to write %s"), *AbsPath);
		return false;
	}

	UE_LOG(LogTHREATEXEC_IO, Log, TEXT("SaveJson OK: %s"), *AbsPath);
	return true;
}

bool TE_FileUtils::LoadTextFile(const FString& AbsPath, FString& OutString)
{
	if (!FPaths::FileExists(AbsPath))
	{
		UE_LOG(LogTHREATEXEC_IO, Warning, TEXT("LoadTextFile: File not found: %s"), *AbsPath);
		return false;
	}

	if (!FFileHelper::LoadFileToString(OutString, *AbsPath))
	{
		UE_LOG(LogTHREATEXEC_IO, Warning, TEXT("LoadTextFile: Failed to read %s"), *AbsPath);
		return false;
	}

	return true;
}

bool TE_FileUtils::LoadJson(const FString& AbsPath, TSharedPtr<FJsonObject>& OutObject)
{
	FString Text;
	if (!LoadTextFile(AbsPath, Text))
	{
		return false;
	}

	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
	if (!FJsonSerializer::Deserialize(Reader, OutObject) || !OutObject.IsValid())
	{
		UE_LOG(LogTHREATEXEC_IO, Warning, TEXT("LoadJson: Failed to parse JSON (%s)"), *AbsPath);
		return false;
	}

	return true;
}
