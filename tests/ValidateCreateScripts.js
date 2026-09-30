// Run after CreateRecipeModelTests writes its fixture file.
const fs = require('node:fs');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const recipes = [];
const ingredient = id => ({ kind: 'itemInput', id });
const item = (id, count) => ({ kind: 'item', id, count });
const fluid = (id, amount) => ({ kind: 'fluid', id, amount });
const chance = (item, chance) => {
  assert.ok(chance >= 0 && chance <= 1);
  return { ...item, chance };
};
const create = new Proxy({}, { get: (_, method) => (...args) => {
  const recipe = { method, args };
  for (const fn of ['heated', 'superheated', 'processingTime', 'keepHeldItem', 'acceptMirrored', 'transitionalItem', 'loops']) {
    recipe[fn] = value => { recipe[`${fn}Value`] = value ?? true; return recipe; };
  }
  recipes.push(recipe); return recipe;
} });
vm.runInNewContext(fs.readFileSync(process.argv[2], 'utf8'), {
  Item: { of: item }, Fluid: { of: fluid }, Ingredient: { of: ingredient }, CreateItem: { of: chance },
  ServerEvents: { recipes: callback => callback({ recipes: { create } }) }
});
const assembly = recipes.find(r => r.method === 'sequenced_assembly');
assert.equal(assembly.args[1].kind, 'itemInput');
assert.equal(assembly.args[2].length, 4);
assert.equal(assembly.args[0][1].chance, 0.25);
assert.equal(assembly.args[0][1].count, 3);
assert.equal(assembly.loopsValue, 5);
assert.deepEqual(Array.from(assembly.args[2], r => r.method), ['pressing', 'cutting', 'deploying', 'filling']);
for (const step of assembly.args[2]) {
  assert.equal(step.args[0].id, assembly.transitionalItemValue);
  assert.equal(step.args[1][0].id, assembly.transitionalItemValue);
}
const fullGrid = recipes.find(r => r.method === 'mechanical_crafting' && r.args[1].length === 9);
assert.equal(Object.keys(fullGrid.args[2]).length, 81);
for (const row of fullGrid.args[1]) {
  assert.equal(row.length, 9);
  for (const symbol of row) assert.ok(fullGrid.args[2][symbol]);
}
const basin = recipes.find(r => r.method === 'mixing' && r.args[1].length === 66);
assert.equal(basin.args[1].filter(e => e.kind === 'itemInput').length, 64);
assert.equal(basin.args[1].filter(e => e.kind === 'fluid').length, 2);
assert.equal(recipes.find(r => r.method === 'filling').args[1][1].amount, 1000);
console.log(`${recipes.length} generated recipes parsed and checked`);
