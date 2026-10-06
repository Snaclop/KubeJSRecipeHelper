ServerEvents.recipes(event => {
  // Minecraft 1.20.1 · 农夫乐事 · 砧板切割
  event.custom({
    "type": "farmersdelight:cutting",
    "ingredients": [{ "item": "minecraft:andesite" }],
    "tool": [{ "type": "farmersdelight:tool_action", "action": "axe_strip" }, { "tag": "minecraft:axes" }],
    "result": [{ "item": "minecraft:ancient_debris", "count": 1, "chance": 0.05 }],
    "sound": "minecraft:entity.sheep.shear"
  })
})

ServerEvents.recipes(event => {
  // Minecraft 1.21.1 · 农夫乐事 · 砧板切割
  event.custom({
    "type": "farmersdelight:cutting",
    "ingredients": [{ "item": "minecraft:andesite" }],
    "tool": [{ "type": "farmersdelight:item_ability", "action": "axe_strip" }, { "tag": "minecraft:axes" }],
    "result": [{ "item": { "id": "minecraft:ancient_debris", "count": 1 }, "chance": 0.05 }]
  })
})

