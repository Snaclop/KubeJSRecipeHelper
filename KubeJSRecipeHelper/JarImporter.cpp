#include "JarImporter.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include "third_party/miniz/miniz.h"
#include "third_party/nlohmann/json.hpp"
#include <iterator>
#include <set>
#include <utility>

namespace
{
	using Json = nlohmann::ordered_json;

	std::string ToUtf8(const std::wstring& strValue)
	{
		const int nSize = WideCharToMultiByte(CP_UTF8, 0, strValue.data(), (int)strValue.size(), nullptr, 0, nullptr, nullptr);
		std::string strResult(nSize, '\0');
		if (nSize > 0)
			WideCharToMultiByte(CP_UTF8, 0, strValue.data(), (int)strValue.size(), strResult.data(), nSize, nullptr, nullptr);
		return strResult;
	}

	std::wstring FromUtf8(const std::string& strValue)
	{
		const int nSize = MultiByteToWideChar(CP_UTF8, 0, strValue.data(), (int)strValue.size(), nullptr, 0);
		std::wstring strResult(nSize, L'\0');
		if (nSize > 0)
			MultiByteToWideChar(CP_UTF8, 0, strValue.data(), (int)strValue.size(), strResult.data(), nSize);
		return strResult;
	}

	const std::string strJsonExt = ".json";
	const std::string strPngExt = ".png";

	// 模型文件的拆分结果
	struct ModelPath
	{
		std::string strNamespace;	// 命名空间
		std::string strKind;		// item 或 block
		std::string strRelative;	// models/<kind>/ 之后的相对路径（含 .json）
	};

	// 是不是以 .json 结尾的文件名
	bool IsJsonFile(const std::string& strName)
	{
		return strName.size() > strJsonExt.size() &&
			strName.compare(strName.size() - strJsonExt.size(), strJsonExt.size(), strJsonExt) == 0;
	}

	// 去掉结尾的 .json
	std::string WithoutJsonExt(const std::string& strName)
	{
		return strName.substr(0, strName.size() - strJsonExt.size());
	}

	// 把 "assets/create/models/item/wrench/item.json" 拆成 (create, item, wrench/item.json)。
	// 不是 assets/<命名空间>/models/<item|block>/… 形式的路径返回 false。
	bool SplitModelPath(const char* pszPath, ModelPath& model)
	{
		const std::string strPath(pszPath);
		const std::string strAssets = "assets/";
		const std::string strModels = "models/";

		if (strPath.compare(0, strAssets.size(), strAssets) != 0)
			return false;

		// 命名空间
		const size_t nNamespaceEnd = strPath.find('/', strAssets.size());
		if (nNamespaceEnd == std::string::npos)
			return false;

		// models/
		const size_t nModels = nNamespaceEnd + 1;
		if (strPath.compare(nModels, strModels.size(), strModels) != 0)
			return false;

		// item/ 或 block/
		const size_t nKind = nModels + strModels.size();
		const size_t nKindEnd = strPath.find('/', nKind);
		if (nKindEnd == std::string::npos)
			return false;

		const std::string strKind = strPath.substr(nKind, nKindEnd - nKind);
		if (strKind != "item" && strKind != "block")
			return false;

		const std::string strRelative = strPath.substr(nKindEnd + 1);
		if (!IsJsonFile(strRelative))
			return false;

		model.strNamespace = strPath.substr(strAssets.size(), nNamespaceEnd - strAssets.size());
		model.strKind = strKind;
		model.strRelative = strRelative;
		return true;
	}

	// 把模型引用（"minecraft:item/clock"、"item/clock"、"create:block/cogwheel"）拆成模型路径。
	// 引用一般不写 .json，没写命名空间时按 minecraft 算，跟游戏里的规则一致；
	// 不是 <item|block>/… 形式的引用（builtin/entity 之类）返回 false。
	bool SplitModelRef(const std::string& strRef, ModelPath& model)
	{
		std::string strNamespace = "minecraft";
		std::string strPath = strRef;

		const size_t nColon = strRef.find(':');
		if (nColon != std::string::npos)
		{
			strNamespace = strRef.substr(0, nColon);
			strPath = strRef.substr(nColon + 1);
		}

		if (IsJsonFile(strPath))
			strPath = WithoutJsonExt(strPath);

		const size_t nSlash = strPath.find('/');
		if (nSlash == std::string::npos)
			return false;

		const std::string strKind = strPath.substr(0, nSlash);
		if (strKind != "item" && strKind != "block")
			return false;

		const std::string strName = strPath.substr(nSlash + 1);
		if (strName.empty())
			return false;

		model.strNamespace = strNamespace;
		model.strKind = strKind;
		model.strRelative = strName + strJsonExt;
		return true;
	}

	// 把 "assets/create/textures/item/wrench.png" 拆成 (create, item/wrench)。
	// 不是 assets/<命名空间>/textures/…png 形式的路径返回 false。
	bool SplitTexturePath(const char* pszPath, std::string& strNamespace, std::string& strRelative)
	{
		const std::string strPath(pszPath);
		const std::string strAssets = "assets/";
		const std::string strTextures = "textures/";

		if (strPath.compare(0, strAssets.size(), strAssets) != 0)
			return false;

		const size_t nNamespaceEnd = strPath.find('/', strAssets.size());
		if (nNamespaceEnd == std::string::npos)
			return false;

		const size_t nTextures = nNamespaceEnd + 1;
		if (strPath.compare(nTextures, strTextures.size(), strTextures) != 0)
			return false;

		// textures/ 下面按用途分目录（item/block/entity/fluid/…），这里不限制
		const std::string strName = strPath.substr(nTextures + strTextures.size());
		if (strName.size() <= strPngExt.size() ||
			strName.compare(strName.size() - strPngExt.size(), strPngExt.size(), strPngExt) != 0)
			return false;

		strNamespace = strPath.substr(strAssets.size(), nNamespaceEnd - strAssets.size());
		strRelative = strName.substr(0, strName.size() - strPngExt.size());
		return true;
	}

	// 从贴图相对路径里认出流体："fluid/<名称>_still.png" 或 "fluid/<名称>_flow.png"。
	// 流体没有模型，只有静止和流动这两张贴图
	bool SplitFluidName(const std::string& strRelative, std::string& strName)
	{
		const std::string strFluid = "fluid/";
		if (strRelative.compare(0, strFluid.size(), strFluid) != 0)
			return false;

		// 流体贴图直接放在 fluid/ 下，出现子目录说明不是
		const std::string strRest = strRelative.substr(strFluid.size());
		if (strRest.find('/') != std::string::npos)
			return false;

		const char* arrSuffix[] = { "_still", "_flow" };
		for (size_t nIndex = 0; nIndex < std::size(arrSuffix); ++nIndex)
		{
			const std::string strSuffix = arrSuffix[nIndex];
			if (strRest.size() > strSuffix.size() &&
				strRest.compare(strRest.size() - strSuffix.size(), strSuffix.size(), strSuffix) == 0)
			{
				strName = strRest.substr(0, strRest.size() - strSuffix.size());
				return true;
			}
		}

		return false;
	}

	// 流体贴图的拆分结果
	struct FluidPath
	{
		std::string strNamespace;	// 命名空间
		std::string strName;		// 流体名（贴图名去掉 _still / _flow）
	};

	// 把 "assets/create/lang/en_us.json" 拆成 (create, en_us.json)
	bool SplitLangPath(const char* pszPath, std::string& strNamespace, std::string& strFile)
	{
		const std::string strPath(pszPath);
		const std::string strAssets = "assets/";
		const std::string strLang = "lang/";

		if (strPath.compare(0, strAssets.size(), strAssets) != 0)
			return false;

		const size_t nNamespaceEnd = strPath.find('/', strAssets.size());
		if (nNamespaceEnd == std::string::npos)
			return false;

		const size_t nLang = nNamespaceEnd + 1;
		if (strPath.compare(nLang, strLang.size(), strLang) != 0)
			return false;

		// langCache 目录下不再有子目录，出现子目录说明不是语言文件
		const std::string strName = strPath.substr(nLang + strLang.size());
		if (!IsJsonFile(strName) || strName.find('/') != std::string::npos)
			return false;

		strNamespace = strPath.substr(strAssets.size(), nNamespaceEnd - strAssets.size());
		strFile = strName;
		return true;
	}

	// 是不是流体标签文件：data/<命名空间>/tags/fluids/**.json
	//
	// 流体没有物品模型，原版的 water / lava 连 textures/fluid/ 都没有（贴图是
	// textures/block/water_still.png，和方块混在一起），只有标签里认得出它们。
	// 标签的成员（minecraft:water）写在文件内容里，这里先只认路径
	bool IsFluidTagPath(const char* pszPath)
	{
		const std::string strPath(pszPath);
		const std::string strData = "data/";
		const char* arrTagDir[] = { "tags/fluids/", "tags/fluid/" };

		if (strPath.compare(0, strData.size(), strData) != 0)
			return false;

		const size_t nNamespaceEnd = strPath.find('/', strData.size());
		if (nNamespaceEnd == std::string::npos)
			return false;

		const size_t nTags = nNamespaceEnd + 1;
		bool bTagDir = false;
		for (size_t nIndex = 0; nIndex < std::size(arrTagDir); ++nIndex)
		{
			const std::string strTagDir = arrTagDir[nIndex];
			if (strPath.compare(nTags, strTagDir.size(), strTagDir) == 0)
			{
				bTagDir = true;
				break;
			}
		}
		if (!bTagDir)
			return false;

		// 标签目录下还能再分子目录，最后的文件名是 json 就行
		return IsJsonFile(strPath);
	}

	// 把流体标签里的一个值拆成 id："minecraft:water" -> (minecraft, water)。
	// 引别的标签的值（"#forge:milk"）和不成 id 的字符串返回 false
	bool SplitFluidId(const std::string& strValue, std::string& strNamespace, std::string& strName)
	{
		if (strValue.empty() || strValue[0] == '#')
			return false;

		const size_t nColon = strValue.find(':');
		if (nColon == std::string::npos || nColon + 1 >= strValue.size())
			return false;

		// 资源名只由小写字母、数字和 _ - . / 组成，免得把别的字符串当成流体 id
		for (size_t nIndex = 0; nIndex < strValue.size(); ++nIndex)
		{
			const char chValue = strValue[nIndex];
			if ((chValue >= 'a' && chValue <= 'z') || (chValue >= '0' && chValue <= '9') ||
				chValue == '_' || chValue == '-' || chValue == '.' || chValue == '/' || chValue == ':')
				continue;
			return false;
		}

		strNamespace = strValue.substr(0, nColon);
		strName = strValue.substr(nColon + 1);
		return true;
	}

	// 判断某个模型文件在不在这个 jar 里的键
	std::string MakePathKey(const std::string& strNamespace, const std::string& strKind, const std::string& strRelative)
	{
		return strNamespace + "/" + strKind + "/" + strRelative;
	}

	// 贴图集合里的键："命名空间/相对路径"（相对路径不含 .png）
	std::string MakeRefKey(const std::string& strNamespace, const std::string& strRelative)
	{
		return strNamespace + "/" + strRelative;
	}

	// 解压 zip 里第 nIndex 个成员，失败时返回空串
	std::string ExtractZipText(mz_zip_archive* pZip, int nIndex)
	{
		std::string strText;
		if (pZip == nullptr || nIndex < 0)
			return strText;

		size_t nSize = 0;
		void* pData = mz_zip_reader_extract_to_heap(pZip, (mz_uint)nIndex, &nSize, 0);
		if (pData != nullptr)
		{
			strText.assign(static_cast<const char*>(pData), nSize);
			mz_free(pData);
		}

		return strText;
	}

	// 按需解压、解析一次；后续查询复用文档，避免逐物品解析大型语言文件。
	struct JsonCache
	{
		mz_zip_archive* pZip;

		std::map<std::string, Json> mapDocuments;

		JsonCache() : pZip(nullptr) {}

	protected:
		// 未登记、解压失败或 JSON 无效时缓存 discarded，调用者检查文档类型。
		const Json& GetJsonAt(const std::string& strKey, int nIndex)
		{
			auto it = mapDocuments.find(strKey);
			if (it != mapDocuments.end())
				return it->second;

			return mapDocuments.emplace(strKey, Json::parse(ExtractZipText(pZip, nIndex), nullptr, false)).first->second;
		}
	};

	// 语言文件缓存
	//
	// 物品 id 里的 '/' 在语言文件里写成 '.'（id 为 sophisticatedbackpacks:sawmill/sawmill_upgrade
	// 的物品，语言键是 item.sophisticatedbackpacks.sawmill.sawmill_upgrade），所以反过来可以用
	// 语言键是否存在，确认某个名字到底是不是真正的物品 id；流体名（fluid.create.tea）也在这里查
	struct LangCache : JsonCache
	{
		std::map<std::string, int>  mapIndex;	// 命名空间 -> 语言文件在 zip 中的序号
		std::map<std::string, bool> mapEnUs;	// 命名空间 -> 当前用的是不是 en_us.json

		// 记录一个语言文件（同一个命名空间优先用 en_us.json）
		void Add(const std::string& strNamespace, int nIndex, bool bEnUs)
		{
			std::map<std::string, bool>::iterator it = mapEnUs.find(strNamespace);
			if (it != mapEnUs.end() && (it->second || !bEnUs))
				return;
			Json document = Json::parse(ExtractZipText(pZip, nIndex), nullptr, false);
			if (!document.is_object())
				return;

			mapIndex[strNamespace] = nIndex;
			mapEnUs[strNamespace] = bEnUs;
			mapDocuments[strNamespace] = std::move(document);
		}

		// 这个命名空间有没有语言文件可查
		bool Has(const std::string& strNamespace)
		{
			return mapIndex.find(strNamespace) != mapIndex.end();
		}

		// 语言文件里有没有 "item.<命名空间>.<名称>" 这样的键
		bool HasKey(const std::string& strNamespace, const char* pszType, const std::string& strName)
		{
			const Json& document = GetJson(strNamespace);
			if (!document.is_object())
				return false;

			std::string strKey = std::string(pszType) + "." + strNamespace + ".";
			for (size_t nIndex = 0; nIndex < strName.size(); ++nIndex)
				strKey += (strName[nIndex] == '/' ? '.' : strName[nIndex]);
			return document.contains(strKey);
		}

		// 语言文件里有没有这个名字的物品键或方块键
		// （方块做成的物品两种键都可能出现，只看一种会漏）
		bool HasName(const std::string& strNamespace, const std::string& strName)
		{
			return HasKey(strNamespace, "item", strName) || HasKey(strNamespace, "block", strName);
		}

	private:
		const Json& GetJson(const std::string& strNamespace)
		{
			std::map<std::string, int>::const_iterator itIndex = mapIndex.find(strNamespace);
			return GetJsonAt(strNamespace, itIndex == mapIndex.end() ? -1 : itIndex->second);
		}
	};

	// 模型文件缓存
	//
	// 键统一用 "命名空间/<kind>/相对路径"，跟 MakePathKey 一致。模型的 overrides、
	// parent 检查会反复读同一个模型，所以缓存解析后的文档。
	struct ModelCache : JsonCache
	{
		std::map<std::string, int> mapIndex;	// 模型文件的键 -> zip 中的序号

		void Add(const std::string& strKey, int nIndex)
		{
			mapIndex[strKey] = nIndex;
		}

		// 这个模型文件在不在包里
		bool Has(const std::string& strKey)
		{
			return mapIndex.find(strKey) != mapIndex.end() &&
				GetJson(strKey).is_object();
		}

		const Json& GetJson(const std::string& strKey)
		{
			std::map<std::string, int>::const_iterator itIndex = mapIndex.find(strKey);
			return GetJsonAt(strKey, itIndex == mapIndex.end() ? -1 : itIndex->second);
		}
	};


	void CollectJsonStrings(const Json& entry, std::vector<std::string>& arrStrings)
	{
		if (entry.is_string())
			arrStrings.push_back(entry.get<std::string>());
		else if (entry.is_structured())
			for (const auto& childEntry : entry)
				CollectJsonStrings(childEntry, arrStrings);
	}

	bool ScanModelJson(const Json& model, std::vector<std::string>& arrStrings,
		std::string& strParent, std::vector<std::string>& arrOverrides)
	{
		if (!model.is_object())
			return false;
		CollectJsonStrings(model, arrStrings);
		const auto itParent = model.find("parent");
		if (itParent != model.end() && itParent->is_string())
			strParent = itParent->get<std::string>();
		const auto itOverrides = model.find("overrides");
		if (itOverrides != model.end() && itOverrides->is_array())
		{
			for (const auto& overrideEntry : *itOverrides)
			{
				if (!overrideEntry.is_object())
					continue;
				const auto itModel = overrideEntry.find("model");
				if (itModel != overrideEntry.end() && itModel->is_string())
					arrOverrides.push_back(itModel->get<std::string>());
			}
		}
		return true;
	}

	// 贴图引用是不是指向一张确实存在的贴图
	//
	// 引用没写命名空间时游戏按 minecraft 算，这里顺手把模型自己的命名空间也认一下，
	// 免得漏掉把贴图放在自己命名空间下的写法
	std::string FindTexture(const std::set<std::string>& setTextures, const std::string& strRef,
		const std::string& strDefaultNamespace)
	{
		std::string strNamespace = "minecraft";
		std::string strPath = strRef;

		const size_t nColon = strRef.find(':');
		if (nColon != std::string::npos)
		{
			strNamespace = strRef.substr(0, nColon);
			strPath = strRef.substr(nColon + 1);
		}

		const std::string strKey = MakeRefKey(strNamespace, strPath);
		if (setTextures.count(strKey) > 0)
			return strKey;

		const std::string strDefaultKey = MakeRefKey(strDefaultNamespace, strPath);
		if (nColon == std::string::npos && setTextures.count(strDefaultKey) > 0)
			return strDefaultKey;
		return std::string();
	}

	// 这个模型有没有引用到真实存在的贴图（自己没写就顺着 parent 往上找）
	//
	// 能拿在手上的东西都有贴图：air.json 是空模型，item/generated.json 这种模板只有 display，
	// 而 stone.json 只是转手指向 block/stone。正好用这一点把不是物品的模型筛掉
	std::string FindModelTexture(const std::string& strKey, const std::string& strNamespace,
		const std::set<std::string>& setTextures, ModelCache& modelCache, std::set<std::string>& setVisited)
	{
		if (!setVisited.insert(strKey).second)	// 模型互相引用时别绕圈
			return std::string();

		const Json& model = modelCache.GetJson(strKey);
		if (!model.is_object())
			return std::string();

		std::vector<std::string> arrStrings;
		std::vector<std::string> arrOverrides;
		std::string strParent;
		ScanModelJson(model, arrStrings, strParent, arrOverrides);

		// parent 和 overrides 里的是模型引用（item/generated 这种），不是贴图，别拿来当证据
		std::set<std::string> setModelRefs(arrOverrides.begin(), arrOverrides.end());
		setModelRefs.insert(strParent);

		for (size_t nIndex = 0; nIndex < arrStrings.size(); ++nIndex)
		{
			// 贴图路径都带 '/'，用不着看 gui_light 这类字符串
			if (arrStrings[nIndex].find('/') == std::string::npos || setModelRefs.count(arrStrings[nIndex]) > 0)
				continue;

			const std::string strTexture = FindTexture(setTextures, arrStrings[nIndex], strNamespace);
			if (!strTexture.empty())
				return strTexture;
		}

		// parent 一般不写扩展名，最多往上找几层（setVisited 里每层加一个键，顺便当层数用）
		ModelPath parent;
		if (setVisited.size() >= 8 || !SplitModelRef(strParent, parent))
			return std::string();

		return FindModelTexture(MakePathKey(parent.strNamespace, parent.strKind, parent.strRelative),
			parent.strNamespace, setTextures, modelCache, setVisited);
	}
}


bool ImportJar(const std::wstring& strJarPath, JarImportResult& result, std::wstring& strError)
{
	result = JarImportResult{};
	strError.clear();
	mz_zip_archive zip{};
	const std::string strPath = ToUtf8(strJarPath);
	if (!mz_zip_reader_init_file(&zip, strPath.c_str(), 0))
	{
		strError = L"Unable to open JAR archive: " + strJarPath;
		return false;
	}

	// ---------- 第一步：收集物品模型、贴图和语言文件 ----------
	//
	// 候选只来自 assets/<命名空间>/models/item/：能拿在手上的东西（方块物品也算）都有物品模型，
	// 而水、岩浆、各种砖墙这些拿不到的方块只有 models/block/ 里的模型
	std::vector<ModelPath> arrModels;
	ModelCache modelCache;
	modelCache.pZip = &zip;
	std::set<std::string> setTextures;		// 包里有哪些贴图
	std::map<std::string, std::string> mapTexturePaths;	// 贴图键 -> JAR 内的 PNG 路径
	std::vector<FluidPath> arrFluids;		// 候选流体（来自 textures/fluid 贴图）
	std::vector<int> arrFluidTags;			// 流体标签文件在 zip 里的序号
	std::map<std::string, unsigned> mapTagKinds;
	LangCache langCache;
	langCache.pZip = &zip;

	const mz_uint nFiles = mz_zip_reader_get_num_files(&zip);
	for (mz_uint nIndex = 0; nIndex < nFiles; ++nIndex)
	{
		mz_zip_archive_file_stat archiveEntry;
		if (!mz_zip_reader_file_stat(&zip, nIndex, &archiveEntry) || archiveEntry.m_is_directory)
			continue;

		ModelPath model;
		if (SplitModelPath(archiveEntry.m_filename, model))
		{
			// block 模型也要记下来：物品模型会顺着 parent 找到它们
			modelCache.Add(MakePathKey(model.strNamespace, model.strKind, model.strRelative), (int)nIndex);
			if (model.strKind == "item")
				arrModels.push_back(model);
			continue;
		}

		std::string strTextureNamespace;
		std::string strTextureRelative;
		if (SplitTexturePath(archiveEntry.m_filename, strTextureNamespace, strTextureRelative))
		{
			const std::string strTextureKey = MakeRefKey(strTextureNamespace, strTextureRelative);
			setTextures.insert(strTextureKey);
			mapTexturePaths[strTextureKey] = archiveEntry.m_filename;

			// 流体：textures/fluid/<名称>_still.png / _flow.png
			std::string strFluidName;
			if (SplitFluidName(strTextureRelative, strFluidName))
			{
				FluidPath fluid;
				fluid.strNamespace = strTextureNamespace;
				fluid.strName = strFluidName;
				arrFluids.push_back(fluid);
			}
			continue;
		}

		std::string strTagId;
		unsigned uTagKind = 0;
		if (RecipeTagFile(archiveEntry.m_filename, strTagId, uTagKind) &&
			Json::parse(ExtractZipText(&zip, (int)nIndex), nullptr, false).is_object())
			mapTagKinds[strTagId] |= uTagKind;

		// 流体标签的成员（minecraft:water 这种）写在文件内容里，先记下位置
		if (IsFluidTagPath(archiveEntry.m_filename))
		{
			arrFluidTags.push_back((int)nIndex);
			continue;
		}

		// 顺带记下语言文件，后面确认物品名和流体名时要用到
		std::string strLangNamespace;
		std::string strLangFile;
		if (SplitLangPath(archiveEntry.m_filename, strLangNamespace, strLangFile))
			langCache.Add(strLangNamespace, (int)nIndex, strLangFile == "en_us.json");
	}

	// ---------- 第二步：先找出变体模型 ----------
	//
	// 拉弓的每一帧（bow_pulling_0）、指南针的每一圈（compass_16）、各种护甲纹饰……
	// 都是被别的模型的 overrides 引用的，玩家拿不到单独的这一件，不算物品
	std::set<std::string> setVariants;
	for (size_t nIndex = 0; nIndex < arrModels.size(); ++nIndex)
	{
		const ModelPath& model = arrModels[nIndex];
		const std::string strKey = MakePathKey(model.strNamespace, model.strKind, model.strRelative);
		if (!modelCache.GetJson(strKey).is_object())
			continue;

		std::vector<std::string> arrStrings;	// 这里只看模型引用，字符串列表用不上
		std::vector<std::string> arrOverrides;
		std::string strParent;
		ScanModelJson(modelCache.GetJson(strKey), arrStrings, strParent, arrOverrides);

		for (size_t nOverrideIndex = 0; nOverrideIndex < arrOverrides.size(); ++nOverrideIndex)
		{
			ModelPath ref;
			if (!SplitModelRef(arrOverrides[nOverrideIndex], ref))
				continue;

			const std::string strRefKey = MakePathKey(ref.strNamespace, ref.strKind, ref.strRelative);
			if (strRefKey != strKey)	// 引用自己（如 clock.json 里的 "model": "item/clock"）不算变体
				setVariants.insert(strRefKey);
		}
	}

	// ---------- 第三步：逐个物品模型推出物品 id ----------
	std::vector<std::string> arrNames;
	std::set<std::string> setNames;
	std::map<std::string, std::string> mapItemTextures;	// 物品 ID -> 贴图键
	std::map<std::string, unsigned> mapEntryTypes;

	// 收录一个名字，按 “命名空间:名字” 去重
	auto AddName = [&arrNames, &setNames, &mapEntryTypes](const std::string& strNamespace, const std::string& strName, unsigned uKind = RecipeItem)
	{
		if (strNamespace.empty() || strName.empty())
			return;

		const std::string strFull = strNamespace + ":" + strName;
		mapEntryTypes[strFull] |= uKind;
		if (setNames.insert(strFull).second)
			arrNames.push_back(strFull);
	};

	auto AddModelName = [&AddName, &mapItemTextures, &setTextures, &modelCache](
		const std::string& strNamespace, const std::string& strName, const std::string& strModelKey)
	{
		AddName(strNamespace, strName);
		const std::string strFull = strNamespace + ":" + strName;
		if (mapItemTextures.count(strFull) != 0)
			return;

		std::set<std::string> setVisited;
		const std::string strTexture = FindModelTexture(strModelKey, strNamespace,
			setTextures, modelCache, setVisited);
		if (!strTexture.empty())
			mapItemTextures[strFull] = strTexture;
	};

	for (size_t nIndex = 0; nIndex < arrModels.size(); ++nIndex)
	{
		const ModelPath& model = arrModels[nIndex];
		const std::string strKey = MakePathKey(model.strNamespace, model.strKind, model.strRelative);
		if (!modelCache.GetJson(strKey).is_object())
			continue;

		// 直接放在 models/item 下的模型：文件名就是物品名
		const size_t nSlash = model.strRelative.find('/');
		if (nSlash == std::string::npos)
		{
			// 变体模型（拉弓的每一帧、指南针的每一圈……）不是能单独拿到的物品
			if (setVariants.count(strKey) > 0)
				continue;

			// 模型得真的引用到一张存在的贴图：air 这种空模型、item/generated 这种
			// 光有 display 的模板都没引用贴图，正好筛掉
			std::set<std::string> setVisited;
			const std::string strTexture = FindModelTexture(strKey, model.strNamespace,
				setTextures, modelCache, setVisited);
			if (strTexture.empty())
				continue;

			const std::string strName = WithoutJsonExt(model.strRelative);
			AddName(model.strNamespace, strName);
			mapItemTextures[model.strNamespace + ":" + strName] = strTexture;
			continue;
		}

		const std::string strFolder = model.strRelative.substr(0, nSlash);	// 最外层的文件夹名

		// 物品：同级还有同名的扁平模型时，这个文件夹只是该物品的零件集合
		// （如 models/item/wrench/），物品名已经由那个扁平模型收录了
		if (modelCache.Has(MakePathKey(model.strNamespace, "item", strFolder + ".json")))
			continue;

		// 文件夹里有 item.json 时（Create 等模组的自定义模型写法），文件夹名就是物品名
		if (modelCache.Has(MakePathKey(model.strNamespace, "item", strFolder + "/item.json")))
		{
			AddModelName(model.strNamespace, strFolder,
				MakePathKey(model.strNamespace, "item", strFolder + "/item.json"));
			continue;
		}

		// 剩下的情况光看文件名分不出文件夹名和相对路径哪个才是物品 id，用语言文件确认：
		//   create 的 models/item/package/ 里那一堆零件  -> 语言键 item.create.package 存在，
		//                                                   而 item.create.package.xxx 都不存在，用文件夹名
		//   models/item/sawmill/sawmill_upgrade.json    -> 语言键
		//                                                   item.sophisticatedbackpacks.sawmill 不存在，
		//                                                   而 item.sophisticatedbackpacks.sawmill.sawmill_upgrade
		//                                                   存在，用相对路径
		// 方块做成的物品，语言键是 block. 开头，两种都查
		// 按约定路径收录时，文件夹本身也是物品 id 的一部分
		const std::string strRelativeName = WithoutJsonExt(model.strRelative);
		if (langCache.Has(model.strNamespace))
		{
			if (langCache.HasName(model.strNamespace, strFolder))
				AddModelName(model.strNamespace, strFolder, strKey);
			if (langCache.HasName(model.strNamespace, strRelativeName))
				AddModelName(model.strNamespace, strRelativeName, strKey);
		}
		else
		{
			// 没有语言文件可查，就按物品模型的约定路径收录：
			// models/item/<id>.json 里的 <id> 就是物品 id
			AddModelName(model.strNamespace, strRelativeName, strKey);
		}
	}

	// ---------- 第四步：流体 ----------
	//
	// 流体没有物品模型，两处线索：
	//   1. 流体标签 data/<命名空间>/tags/fluids/**.json 的成员，命名空间写在值里
	//      （原版的 minecraft:water / minecraft:lava 就是这么来的）
	//   2. textures/fluid/<名称>_still.png / _flow.png 贴图。语言文件里有
	//      fluid.<命名空间>.<名称> 的就是模组自己注册的流体（create:tea）；没有的，多半是
	//      Forge 之类把流体挂在 minecraft 命名空间下、贴图由模组补的
	//      （Create 补的 milk 贴图对应 minecraft:milk）
	for (size_t nIndex = 0; nIndex < arrFluidTags.size(); ++nIndex)
	{
		const Json entry = Json::parse(ExtractZipText(&zip, arrFluidTags[nIndex]), nullptr, false);
		if (!entry.is_object())
			continue;
		const auto itValues = entry.find("values");
		if (itValues == entry.end() || !itValues->is_array())
			continue;
		for (const auto& memberEntry : *itValues)
		{
			std::string strValue;
			if (memberEntry.is_string())
				strValue = memberEntry.get<std::string>();
			else if (memberEntry.is_object())
			{
				const auto itId = memberEntry.find("id");
				if (itId != memberEntry.end() && itId->is_string())
					strValue = itId->get<std::string>();
			}
			const std::wstring strWideValue = FromUtf8(strValue);
			if (!RecipeValidId(strWideValue, true))
				continue;
			if (RecipeIsTag(strWideValue))
			{
				mapTagKinds[strValue] |= RecipeFluid;
				continue;
			}
			std::string strNamespace;
			std::string strName;
			if (!SplitFluidId(strValue, strNamespace, strName))
				continue;
			AddName(strNamespace, strName, RecipeFluid);
			const std::string strBlockTexture = MakeRefKey(strNamespace, "block/" + strName + "_still");
			if (setTextures.count(strBlockTexture) > 0)
				mapItemTextures[strValue] = strBlockTexture;
		}
	}

	for (size_t nIndex = 0; nIndex < arrFluids.size(); ++nIndex)
	{
		const std::string& strNamespace = arrFluids[nIndex].strNamespace;
		const std::string& strName = arrFluids[nIndex].strName;

		const std::string strIdNamespace =
			(strNamespace == "minecraft" || langCache.HasKey(strNamespace, "fluid", strName))
			? strNamespace : "minecraft";
		AddName(strIdNamespace, strName, RecipeFluid);

		const std::string strStill = MakeRefKey(strNamespace, "fluid/" + strName + "_still");
		const std::string strFlow = MakeRefKey(strNamespace, "fluid/" + strName + "_flow");
		const std::string strId = strIdNamespace + ":" + strName;
		if (setTextures.count(strStill) > 0)
			mapItemTextures[strId] = strStill;
		else if (mapItemTextures.count(strId) == 0 && setTextures.count(strFlow) > 0)
			mapItemTextures[strId] = strFlow;
	}

	mz_zip_reader_end(&zip);

	for (const auto& tagEntry : mapTagKinds)
		result.mapEntryTypes[FromUtf8(tagEntry.first)] |= tagEntry.second;
	for (const auto& strName : arrNames)
		result.arrNames.push_back(FromUtf8(strName));
	for (const auto& entry : mapEntryTypes)
		result.mapEntryTypes[FromUtf8(entry.first)] |= entry.second;
	for (const auto& textureEntry : mapItemTextures)
	{
		const auto itPath = mapTexturePaths.find(textureEntry.second);
		if (itPath != mapTexturePaths.end())
			result.mapTexturePaths[FromUtf8(textureEntry.first)] = itPath->second;
	}
	return true;
}

void MergeJarImportResult(JarImportResult& accumulatedResult, const JarImportResult& importedResult)
{
	std::set<std::wstring> setNames(accumulatedResult.arrNames.begin(), accumulatedResult.arrNames.end());
	for (const auto& strName : importedResult.arrNames)
		if (setNames.insert(strName).second)
			accumulatedResult.arrNames.push_back(strName);
	for (const auto& entry : importedResult.mapEntryTypes)
		accumulatedResult.mapEntryTypes[entry.first] |= entry.second;
	for (const auto& textureEntry : importedResult.mapTexturePaths)
		accumulatedResult.mapTexturePaths[textureEntry.first] = textureEntry.second;
}
