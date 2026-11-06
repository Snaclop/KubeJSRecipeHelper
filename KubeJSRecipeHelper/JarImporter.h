#pragma once
#include "RecipeCatalog.h"
#include <map>
#include <string>
#include <vector>

struct JarImportResult
{
	std::vector<std::wstring> arrNames;
	RecipeTypeMap mapEntryTypes;
	std::map<std::wstring, std::string> mapTexturePaths;
};

// Each call replaces result. Tags are catalog entries, not concrete names.
bool ImportJar(const std::wstring& strJarPath, JarImportResult& result, std::wstring& strError);

// Merge imports in first-seen order, combining kinds and updating texture paths.
void MergeJarImportResult(JarImportResult& accumulatedResult, const JarImportResult& importedResult);
