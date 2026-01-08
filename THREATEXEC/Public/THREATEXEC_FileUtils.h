#pragma once

// ============================================================================
// THREATEXEC_FileUtils.h
// Centralised helpers for JSON IO, directory creation, and Saved-folder
// path normalization. Used by BezierCurve2DActor and BezierCurve3DActor.
// ============================================================================

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

// Simple logging category for IO operations
DECLARE_LOG_CATEGORY_EXTERN(LogTHREATEXEC_IO, Log, All);

namespace TE_PathUtils
{
	// Resolve a user path to an absolute directory path under Project/Saved
	// - If UserPath is empty → Saved/Bezier/
	// - If relative → Saved/UserPath/
	// - If absolute → used as-is
	// Ensures the directory exists.
	FString ResolveSavedDir(const FString& UserPath, const FString& DefaultSubfolder = TEXT("Bezier"));

	// Resolve full file path inside the resolved directory.
	FString ResolveFile(const FString& UserPath, const FString& FileName, const FString& DefaultSubfolder = TEXT("Bezier"));
}

namespace TE_FileUtils
{
	// Save JSON object to file (condensed writer)
	bool SaveJson(const FString& AbsPath, const TSharedRef<FJsonObject>& JsonObject);

	// Load JSON string (not parsed) from file
	bool LoadTextFile(const FString& AbsPath, FString& OutString);

	// Load and parse JSON into a new JsonObject
	bool LoadJson(const FString& AbsPath, TSharedPtr<FJsonObject>& OutObject);
}
