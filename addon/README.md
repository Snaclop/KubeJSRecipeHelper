# Addon definitions / 模组配置

Place UTF-8 JSON files in `addon` beside `KubeJSRecipeHelper.exe`. Reopen **Other recipes** to reload them. The build copies the three shipped definitions to the output directory; include the `addon` folder when distributing the program. Invalid files are reported by filename and skipped. A duplicate mod ID is rejected; the first file in filename order is kept.

Each file declares a mod's official name, recipe controls, validation limits and JSON output templates. New definitions can use existing control and template types without recompiling the program. The recipe format must match the actual mod and game version. Definitions do not install mods or run JavaScript inside the helper.

将 UTF-8 编码的 JSON 放到 `KubeJSRecipeHelper.exe` 旁的 `addon` 文件夹，重新打开“其它配方”即可读取。构建时会自动复制现有三个模组的配置；分发程序时需带上 `addon` 文件夹。文件出错会提示文件名并跳过，不影响其他有效文件。模组 ID 重复时，保留文件名排序靠前的文件。

配置包含官方模组名称、配方控件、校验限制和输出模板。使用已有控件及模板规则添加模组时，无需重新编译程序。配方格式须符合实际模组和游戏版本；配置不会安装模组，也不会在助手内运行 JavaScript。

## Minimal example / 最小示例

This illustrates the definition format; `example:processing` is a placeholder recipe type.

以下只说明配置格式，`example:processing` 是占位类型，需替换成实际配方类型。

```json
{
  "schemaVersion": 1,
  "id": "example",
  "name": "Example Mod",
  "recipes": [
    {
      "id": "processing",
      "label": { "en": "Processing", "zh-CN": "加工", "zh-TW": "加工" },
      "versions": ["1.20.1"],
      "groups": [
        { "id": "inputs", "label": "Input", "min": 1, "max": 1, "maxCount": 1 },
        { "id": "outputs", "label": "Output", "output": true, "min": 1, "max": 1 }
      ],
      "fields": [
        { "id": "energy", "label": "Energy", "type": "integer", "default": 1000, "min": 1, "max": 100000 }
      ],
      "output": {
        "type": "example:processing",
        "ingredient": { "$first": "inputs", "$value": { "item": { "$ref": "entry.id" } } },
        "result": { "$first": "outputs", "$value": { "item": { "$ref": "entry.id" }, "count": { "$ref": "entry.count" } } },
        "energy": { "$ref": "params.energy" }
      }
    }
  ]
}
```

## Fields / 字段说明

`schemaVersion` is the helper's definition format version (currently `1`), separate from Minecraft versions. Mod IDs and recipe IDs must be unique within their scope. `name` is the official mod name and stays the same in every UI language. A `label` can be a string or an object with `en`, `zh-CN` and `zh-TW`; missing translations fall back to English, then an available Chinese translation.

`schemaVersion` 是配置格式版本，目前为 `1`，与 Minecraft 版本无关。模组 ID 全局唯一，配方 ID 在所属模组内唯一。`name` 使用官方模组名称，不随界面语言变化。`label` 可为字符串，或含 `en`、`zh-CN`、`zh-TW` 的对象；缺少翻译时依次使用英文或现有中文。`versions` 决定版本下拉框内容，默认选择列表末项。

| Group property / 格子组属性 | Meaning / 含义 |
| --- | --- |
| `id`, `label` | Stable ID and display label / 标识与显示名称 |
| `kind` | `item` (default) or `fluid` / 物品或流体 |
| `output` | Output group, default `false` / 是否为产物组 |
| `side` | `input` or `output`; defaults to the group direction / 显示位置，可让容器等辅助格显示在右侧 |
| `min`, `max` | Minimum filled slots and maximum slots; defaults `0`, `1` / 最少已选格数、最多格数 |
| `initial` | Initially visible slots, default `1` / 初始格数 |
| `maxCount` | Per-slot count limit; default `64` items or `99999999` mB / 每格数量上限 |
| `maxTotal` | Group limit, default `max` / 本组总上限 |
| `totalByCount` | Count item quantities toward `maxTotal`; otherwise count filled slots / 按数量累计总上限，默认按已选格数累计 |
| `chance` | Allow output chance, default `false` / 是否允许设置概率，默认固定为 100% |
| `positiveChanceVersions` | Versions requiring chance above 0 / 概率必须大于 0 的版本列表 |
| `when` | Conditional visibility / 条件显示 |

Each recipe supports up to 8 groups (4 per side), 128 slots per group and 32 parameter fields. Slots and parameter fields are paginated automatically. `requireAny`, for example `[["inputs", "fluid_inputs"], ["outputs", "fluid_outputs"]]`, requires at least one filled slot in each listed set of groups.

每种配方最多 8 组格子，每侧最多 4 组；每组最多 128 格，参数最多 32 个，超出当前显示范围时自动分页。`requireAny` 如 `[["inputs", "fluid_inputs"], ["outputs", "fluid_outputs"]]`，表示输入两组至少选择一项、输出两组至少选择一项。

Parameter `type` supports `integer`, `number`, `text`, `resource`, `tag`, `boolean` and `choice`. All parameters need `id`, `label`, `type` and `default`. Numeric parameters accept `min` and `max`; `required: false` permits empty text/resource/tag values. Tag values may start with `#`, which is removed when referenced in output.

参数 `type` 支持整数、小数、文本、资源 ID、标签、勾选框、下拉框，依次对应 `integer`、`number`、`text`、`resource`、`tag`、`boolean`、`choice`。每个参数均需 `id`、`label`、`type`、`default`。数字用 `min`、`max` 限制范围；文本、资源 ID 和标签可通过 `required: false` 允许留空。标签可带开头的 `#`，输出引用时会去除。

Choices use `choices: [{"label": "Heated", "value": "heated"}]`. `value` is a string, number or boolean, not the translated label. `allowCustom: true` allows typing a custom value into the combo box, validated as `customType` (`text`, `resource` or `tag`). Mold, dye and sound choices in the shipped files demonstrate this.

下拉框使用 `choices: [{"label": "加热", "value": "heated"}]`。`value` 是实际值，可为字符串、数字或布尔值，不是翻译文字。`allowCustom: true` 允许直接输入自定义值，用 `customType` 指定 `text`、`resource` 或 `tag` 校验。现有模具、颜色和声音选项可作为参考。

`when: {"field": "custom_time", "equals": true}` shows a group or parameter when a boolean or choice field has that value. Hidden controls retain their draft values but are not validated. Use a matching `$when` on optional output fields when they should be omitted.

`when: {"field": "custom_time", "equals": true}` 表示指定勾选框或下拉框满足条件时才显示格子组或参数。隐藏控件保留草稿值，但不进行校验。需要省略的输出字段应配上相应的 `$when`。

## Output templates / 输出模板

Ordinary JSON values are copied to the recipe body. Operator objects below substitute typed values. The resulting object is serialized inside `ServerEvents.recipes(... event.custom(...))`; no raw JavaScript template substitution is used.

普通 JSON 内容直接成为配方数据，以下操作对象负责填入有类型的数据。最终统一生成 `ServerEvents.recipes(... event.custom(...))`，不会直接拼入未经转义的 JavaScript 文本。

| Operator / 操作 | Meaning / 含义 |
| --- | --- |
| `{"$ref":"params.energy"}` | Parameter value / 参数值 |
| `{"$ref":"version"}` | Selected version / 当前版本 |
| `{"$ref":"entry.id"}` | Current entry ID; also `entry.count`, `entry.chance` / 当前格子的 ID、数量或百分比概率 |
| `{"$each":"inputs","$value":{...}}` | Map filled slots to an array / 将本组已选格子转换为数组 |
| `"$repeatCount": true` on `$each` | Repeat item ingredients by count / 按物品数量重复原料；不用于流体 |
| `{"$first":"outputs","$value":{...}}` | Convert the first filled slot; omit if empty / 转换第一个已选格子，未选择时省略 |
| `{"$concat":[...array templates...]}` | Join arrays / 合并多个数组 |
| `{"$version":{"1.20.1":{...},"1.21.1":{...}}}` | Choose a template for the selected version / 按版本选择模板 |
| `{"$switch":"tool","$cases":{"knives":{...},"axes":{...}}}` | Choose by a fixed string-valued choice field / 按固定字符串下拉选项选择分支，须覆盖全部选项 |
| `{"$when":{"field":"custom_time","equals":true},"$value":{"$ref":"params.time"}}` | Conditional output / 条件输出 |
| `"$scale": 0.01` | Scale a number, e.g. percentage to 0–1 chance / 数值换算，例如百分比转为 0–1 概率 |
| `"$omitIf": ""` | Omit a matching value (after scaling) / 值等于指定内容时省略，在数值换算后判断 |
| `"$omitEmpty": true` | Omit an empty generated object or array / 省略生成的空对象或数组 |

A null result is omitted from its containing object or array. `$each`/`$first` are the only places where `entry.*` references are available. Literal recipe keys beginning with `$` are reserved for operators. Check the three shipped files for complete version-specific examples, including tool alternatives, fluid formats and nested chance results.

结果为 `null` 的字段或数组元素会被省略。`entry.*` 只能在 `$each`、`$first` 内使用；以 `$` 开头的键保留为模板操作。三个现有文件包含完整版本示例，可参考工具分支、流体字段和嵌套概率产物的写法。
