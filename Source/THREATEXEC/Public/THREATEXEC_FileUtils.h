/**
 * File: THREATEXEC_FileUtils.h
 * Summary: Shared Saved-folder path and JSON file helpers for the THREATEXEC module.
 * Note: Comments added for maintainability only. Behaviour and public API remain unchanged.
 */

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

// Simple logging category for IO operations
DECLARE_LOG_CATEGORY_EXTERN(LogTHREATEXEC_IO, Log, All);

namespace TE_PathUtils
{
	/** Path-normalisation helpers used to resolve relative Saved-folder destinations. */
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
	/** JSON and text-file helper functions used by the runtime export/import pipeline. */
	// Save JSON object to file (condensed writer)
	bool SaveJson(const FString& AbsPath, const TSharedRef<FJsonObject>& JsonObject);

	// Load JSON string (not parsed) from file
	bool LoadTextFile(const FString& AbsPath, FString& OutString);

	// Load and parse JSON into a new JsonObject
	bool LoadJson(const FString& AbsPath, TSharedPtr<FJsonObject>& OutObject);
}
