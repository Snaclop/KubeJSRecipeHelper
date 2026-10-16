English | [简体中文](README-zh.md)

# KubeJSRecipeHelper

A visual KubeJS recipe script generator. The program reads available item, block and fluid IDs from Minecraft or mod JAR files, lets you select ingredients and outputs in recipe dialogs, and previews the generated JavaScript.

## Features

- Generate shaped crafting, shapeless crafting, smithing, smelting, blasting, smoking and stonecutting recipes.
- Generate processing recipes for Create, including pressing, milling, crushing, cutting, mixing, compacting, filling, emptying, mechanical crafting, sequenced assembly and more. **Requires the KubeJS Create extension!**
- Generate custom recipes for other mods. Currently supports Farmer's Delight, Create: Diesel Generators and Create: Dragons Plus, with more mods planned. No KubeJS extension is required.
- Generate scripts to remove specific recipes and modify recipes.
- Collect IDs from resources such as item and block models and fluid tags in JAR files; show previews in recipe slots when corresponding textures are available.
- Preview scripts in the main window and append them to `output.js`.

This program only generates script text. It does not modify game instances or automatically install recipes. Check the generated scripts against the KubeJS and mod versions you use.

## Usage

1. Click "Import .jar files" and select JAR files containing the required item or fluid resources. You can select multiple files at once. The program parses them one by one in list order and displays the current progress. If a file fails to parse, it continues processing the remaining files and reports all failed files at the end. You can import files multiple times; recognized IDs are added to the list of available entries for the current session.
2. Select a recipe type: vanilla recipes, Create or other mods, then follow the prompts.
3. Click "OK" in the recipe dialog to view the script in the main window's output box.
4. Copy the contents of the output box or click "One-click export". The latter appends the script to `output.js` in the program's **current working directory**; exporting again does not overwrite existing content.
5. Check and organize the generated scripts, then place them in your game instance's `kubejs/server_scripts/` directory. See the [official KubeJS documentation](https://kubejs.com/wiki/folder-structure/server-scripts) for recipe script placement.

JAR parsing depends on the resource structure inside each file. Unrecognized entries will not appear in the selection list. Before exporting, confirm that item IDs, quantities, probabilities and the generated scripts are suitable for the target game environment.

## Building and Running

1. Open `KubeJSRecipeHelper.slnx` from the repository root in Visual Studio.
2. Select the `Debug` or `Release` configuration and the `x64` or `Win32` platform.
3. Build and run the `KubeJSRecipeHelper` project.

The project includes the `miniz` source code for reading JAR files. You do not need to install `miniz` separately to run the program.

## Contributing

You are welcome to report problems or suggest improvements through GitHub Issues. When submitting code changes, describe the reproduction steps or use case and include relevant test results where possible. Changes to recipe generation logic should also include checks of the generated script content.
