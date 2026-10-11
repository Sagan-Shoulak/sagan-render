// Execute the actual emitted Sagan model, not a parallel C++ economy simulation.
#define main sagan_generated_scene_entrypoint
#include "program.cpp"
#undef main

#include <array>
#include <cassert>
#include <iostream>

using economy_type = sagan_626173655f65636f6e6f6d795f5f4261736545636f6e6f6d79;
enum resource { steel, aluminum, copper, polymers, silicon, water, ice, oxygen, hydrogen, food, kits, count };

auto stocks(economy_type &state) -> std::array<std::int64_t, count>
{
  std::array<std::int64_t, count> result{};
  for (int i = 0; i < count; ++i) result[i] = state.sagan_73746f636b(i);
  return result;
}
void build_all(economy_type &state)
{
  for (int slot = 0; slot < 4; ++slot) assert(state.sagan_73746172745f6275696c6421(slot, slot + 1));
}
void empty_stock(economy_type &state, bool retain_food = false)
{
  for (int i = 0; i < count; ++i)
    if (!retain_food || i != food) assert(state.sagan_636f6e73756d6521(i, state.sagan_73746f636b(i)));
}

int main()
{
  economy_type state;
  assert(state.sagan_7265736f757263655f636f756e74() == count);
  assert(state.sagan_73746f726167655f75736564() == 3529000 && state.sagan_6361706163697479() == 10000000);
  const std::array<std::array<std::int64_t, 5>, 4> recipes{{
    {{150, 30, 10, 10, 5}}, {{400, 50, 25, 15, 5}},
    {{180, 80, 60, 30, 10}}, {{250, 40, 20, 15, 5}}
  }};
  for (int kind = 1; kind <= 4; ++kind)
  {
    assert(state.sagan_63616e5f6166666f7264(kind));
    for (int i = 0; i < count; ++i)
    {
      const auto cost = i < 5 ? recipes[kind - 1][i] * 1000 : 0;
      assert(state.sagan_636f7374(kind, i) == cost);
      if (!cost) continue;
      economy_type shortage;
      assert(shortage.sagan_636f6e73756d6521(i, shortage.sagan_73746f636b(i) - cost + 1));
      const auto before = stocks(shortage);
      assert(!shortage.sagan_73746172745f6275696c6421(0, kind));
      assert(stocks(shortage) == before && shortage.sagan_6d616368696e65(0) == 0);
    }
  }
  assert(!state.sagan_63616e5f6166666f7264(0) && !state.sagan_63616e5f6166666f7264(5));
  assert(!state.sagan_73746172745f6275696c6421(-1, 1) && !state.sagan_73746172745f6275696c6421(4, 1));
  assert(!state.sagan_6164645f73746f636b21(-1, 1) && !state.sagan_636f6e73756d6521(count, 1));
  assert(!state.sagan_636f6e73756d6521(0, -1) && !state.sagan_6164645f73746f636b21(0, -1));
  assert(!state.sagan_636f6e73756d6521(0, 1400001));

  // Kits use ONLY sifter minerals: no water/ice, unique items or imported parts.
  economy_type full;
  const std::array<std::int64_t, count> kit_recipe{5000, 2000, 10, 0, 990, 0, 0, 0, 0, 0, 0};
  for (int i = 0; i < count; ++i) assert(full.sagan_6b69745f636f7374(i) == kit_recipe[i]);
  assert(!full.sagan_6164645f73746f636b21(kits, 1000));
  assert(full.sagan_6164645f73746f636b21(silicon, full.sagan_6361706163697479() - full.sagan_73746f726167655f75736564()));
  const auto full_stock = stocks(full);
  assert(!full.sagan_6164645f73746f636b21(0, 1) && stocks(full) == full_stock);
  assert(full.sagan_6d616b655f6b697421() && full.sagan_73746f726167655f75736564() == full.sagan_6361706163697479());
  std::int64_t input_mass{};
  for (int i = 0; i < count; ++i)
  {
    const auto cost = full.sagan_6b69745f636f7374(i);
    assert(full.sagan_73746f636b(i) == full_stock[i] - cost + (i == kits));
    input_mass += cost * full.sagan_6974656d5f6d617373(i);
    if (cost) assert(state.sagan_7369667465725f7969656c64(i) > 0);
  }
  assert(input_mass == 8000);
  for (int input : {steel, aluminum, copper, silicon})
  {
    economy_type shortage;
    assert(shortage.sagan_636f6e73756d6521(input,
      shortage.sagan_73746f636b(input) - shortage.sagan_6b69745f636f7374(input) + 1));
    const auto before = stocks(shortage);
    assert(!shortage.sagan_6d616b655f6b697421() && stocks(shortage) == before);
  }
  economy_type dry_kit;
  for (int input : {water, ice, oxygen, hydrogen, food})
    assert(dry_kit.sagan_636f6e73756d6521(input, dry_kit.sagan_73746f636b(input)));
  assert(dry_kit.sagan_6d616b655f6b697421());

  economy_type drill;
  const auto before_drill = stocks(drill);
  drill.sagan_70726f6475636521(2);
  for (int i = 0; i < count; ++i)
    assert(drill.sagan_73746f636b(i) == before_drill[i] + (i == ice ? 9500 : 0));
  economy_type trace;
  empty_stock(trace);
  trace.sagan_7369667421();
  std::int64_t recovered_mass{};
  for (int i = 0; i < count; ++i)
  {
    assert(trace.sagan_73746f636b(i) == trace.sagan_7369667465725f7969656c64(i));
    recovered_mass += trace.sagan_73746f636b(i) * trace.sagan_6974656d5f6d617373(i);
  }
  assert(recovered_mass == 64350 && recovered_mass < 100000);
  assert(trace.sagan_73746f636b(copper) == 40 && trace.sagan_73746f636b(food) == 0);

  // Refill, not just the lower long-term wear average, must sustain copper.
  // Observe complete operating minutes after both machines finish construction.
  economy_type refill;
  build_all(refill);
  refill.sagan_75706461746521(60);
  for (int time : {120, 180, 240})
  {
    const auto copper_before = refill.sagan_73746f636b(copper);
    const auto kits_before = refill.sagan_73746f636b(kits);
    refill.sagan_75706461746521(time);
    assert(refill.sagan_6f75747075745f72617465(8, kits) == 3000);
    assert(refill.sagan_6f75747075745f72617465(5, copper) == 40);
    assert(refill.sagan_696e7075745f72617465(8, copper) == 30);
    assert(refill.sagan_6e65745f72617465(copper) == 10);
    assert(refill.sagan_73746f636b(copper) == copper_before + 10);
    assert(refill.sagan_73746f636b(kits) == kits_before + 3);
  }
  refill.sagan_75706461746521(300);
  assert(refill.sagan_73746f636b(kits) == 20);
  refill.sagan_75706461746521(360);
  assert(refill.sagan_696e7075745f72617465(8, copper) == 0);
  assert(refill.sagan_6e65745f72617465(copper) == 40); // buffer-full idle is not fictitious demand

  // Shared H2O feed: atomic, liquid-first, equal-mass thawing with no extra slot.
  economy_type feed;
  empty_stock(feed);
  assert(feed.sagan_6164645f73746f636b21(water, 200) && feed.sagan_6164645f73746f636b21(ice, 1000));
  assert(feed.sagan_636f6e73756d655f776174657221(300));
  assert(feed.sagan_73746f636b(water) == 0 && feed.sagan_73746f636b(ice) == 900);
  const auto before_dry = stocks(feed);
  assert(!feed.sagan_636f6e73756d655f776174657221(901) && stocks(feed) == before_dry);
  assert(!feed.sagan_636f6e73756d655f776174657221(-1));
  assert(feed.sagan_6164645f73746f636b21(oxygen, 7600) && feed.sagan_6164645f73746f636b21(food, 25));
  assert(feed.sagan_666565645f6c6966655f737570706f727421());
  assert(feed.sagan_73746f636b(ice) == 600 && feed.sagan_73746f636b(food) == 0 && feed.sagan_73746f636b(oxygen) == 0);
  for (int missing : {water, oxygen, food})
  {
    economy_type shortage;
    assert(shortage.sagan_636f6e73756d6521(missing, shortage.sagan_73746f636b(missing)));
    if (missing == water) assert(shortage.sagan_636f6e73756d6521(ice, shortage.sagan_73746f636b(ice)));
    const auto before = stocks(shortage);
    assert(!shortage.sagan_666565645f6c6966655f737570706f727421() && stocks(shortage) == before);
  }
  economy_type electrolysis;
  empty_stock(electrolysis);
  assert(electrolysis.sagan_6164645f73746f636b21(ice, 39000));
  assert(electrolysis.sagan_656c656374726f6c797a6521());
  assert(electrolysis.sagan_73746f636b(ice) == 30000 && electrolysis.sagan_73746f636b(water) == 0);
  assert(electrolysis.sagan_73746f636b(oxygen) == 8000 && electrolysis.sagan_73746f636b(hydrogen) == 1000);
  assert(electrolysis.sagan_73746f726167655f75736564() == 39000);
  const auto reserved_water = stocks(electrolysis);
  assert(!electrolysis.sagan_656c656374726f6c797a6521() && stocks(electrolysis) == reserved_water);
  assert(electrolysis.sagan_6164645f73746f636b21(ice, 9000));
  assert(electrolysis.sagan_6164645f73746f636b21(hydrogen, 49000));
  assert(electrolysis.sagan_656c656374726f6c797a6521());
  assert(electrolysis.sagan_73746f636b(oxygen) == 16000 && electrolysis.sagan_73746f636b(hydrogen) == 50000);
  assert(electrolysis.sagan_76656e7465645f687964726f67656e() == 1000);

  assert(state.sagan_73746172745f6275696c6421(0, 1));
  const auto reserved = stocks(state);
  assert(!state.sagan_73746172745f6275696c6421(0, 2) && stocks(state) == reserved);
  assert(state.sagan_6275696c64696e67(0) && !state.sagan_72756e6e696e67(0));
  state.sagan_75706461746521(9.99);
  assert(state.sagan_6275696c64696e67(0) && state.sagan_73746f636b(silicon) == 95000);
  state.sagan_75706461746521(10);
  assert(!state.sagan_6275696c64696e67(0) && state.sagan_72756e6e696e67(0));
  state.sagan_75706461746521(69.99);
  assert(state.sagan_73746f636b(silicon) == 95000);
  state.sagan_75706461746521(70);
  assert(state.sagan_73746f636b(silicon) == 115000);
  economy_type fractional;
  fractional.sagan_75706461746521(0.75);
  assert(fractional.sagan_73746172745f6275696c6421(0, 1));
  fractional.sagan_75706461746521(10.74);
  assert(fractional.sagan_6275696c64696e67(0));
  fractional.sagan_75706461746521(11);
  assert(!fractional.sagan_6275696c64696e67(0));
  const auto forward_stock = stocks(fractional);
  const auto forward_energy = fractional.sagan_626174746572795f656e65726779();
  fractional.sagan_75706461746521(0);
  assert(stocks(fractional) == forward_stock && fractional.sagan_626174746572795f656e65726779() == forward_energy);

  // 76 cannot use a kit; 75 receives exactly 25. All nine targets auto-repair.
  economy_type repairs;
  assert(repairs.sagan_64616d6167655f73797374656d21(0, 9));
  assert(repairs.sagan_6475726162696c697479(0) == 76 && !repairs.sagan_72657061697221(0));
  assert(repairs.sagan_64616d6167655f73797374656d21(0, 1));
  assert(repairs.sagan_72657061697221(0) && repairs.sagan_6475726162696c697479(0) == 100);
  assert(!repairs.sagan_72657061697221(-1) && !repairs.sagan_72657061697221(9));
  assert(!repairs.sagan_64616d6167655f73797374656d21(0, -1));
  build_all(repairs);
  assert(!repairs.sagan_72657061697221(5));
  repairs.sagan_75706461746521(20);
  assert(repairs.sagan_6164645f73746f636b21(kits, 2));
  for (int system = 0; system < 9; ++system)
    assert(repairs.sagan_64616d6167655f73797374656d21(system, repairs.sagan_6475726162696c697479(system) - 75));
  const auto before_repair = repairs.sagan_73746f636b(kits);
  repairs.sagan_75706461746521(21);
  assert(repairs.sagan_6175746f6d617469635f7265706169725f636f756e74() == 9);
  assert(repairs.sagan_73746f636b(kits) == before_repair - 9);
  for (int system = 0; system < 9; ++system) assert(repairs.sagan_6475726162696c697479(system) == 100);
  repairs.sagan_75706461746521(22);
  assert(repairs.sagan_73746f636b(kits) == before_repair - 9);

  economy_type power;
  build_all(power);
  power.sagan_75706461746521(30);
  const auto charged = power.sagan_626174746572795f656e65726779();
  assert(charged > 576000000 && !power.sagan_706f7765725f73686f7274616765());
  assert(!power.sagan_7365745f73756e6c6967687421(-1) && !power.sagan_7365745f73756e6c6967687421(1001));
  assert(power.sagan_7365745f73756e6c6967687421(0));
  power.sagan_75706461746521(60);
  assert(power.sagan_67656e6572617465645f7761747473() == 0);
  assert(power.sagan_626174746572795f656e65726779() < charged && !power.sagan_706f7765725f73686f7274616765());
  power.sagan_75706461746521(10000);
  assert(power.sagan_626174746572795f656e65726779() == 0 && power.sagan_706f7765725f73686f7274616765());
  const auto no_power = stocks(power);
  power.sagan_75706461746521(10020);
  assert(stocks(power) == no_power && !power.sagan_6c6966655f737570706f72745f7265616479());
  assert(power.sagan_7365745f73756e6c6967687421(1000));
  power.sagan_75706461746521(10021);
  assert(power.sagan_626174746572795f656e65726779() > 0 && !power.sagan_706f7765725f73686f7274616765());
  economy_type paused_build;
  assert(paused_build.sagan_7365745f73756e6c6967687421(0));
  paused_build.sagan_75706461746521(30000);
  assert(paused_build.sagan_73746172745f6275696c6421(0, 1));
  paused_build.sagan_75706461746521(30020);
  assert(paused_build.sagan_6275696c64696e67(0) && paused_build.sagan_7365636f6e64735f6c656674(0) == 10);

  // Loss of battery capacity does not steal the remaining solar power from crew.
  economy_type essential;
  build_all(essential);
  essential.sagan_75706461746521(30);
  assert(essential.sagan_64616d6167655f73797374656d21(2, 100));
  assert(essential.sagan_636f6e73756d6521(kits, essential.sagan_73746f636b(kits)));
  assert(essential.sagan_7365745f73756e6c6967687421(250));
  essential.sagan_75706461746521(31);
  assert(essential.sagan_6c6966655f737570706f72745f7265616479());
  assert(essential.sagan_7365727665645f7761747473() >= 24000);
  for (int slot = 0; slot < 4; ++slot) assert(!essential.sagan_72756e6e696e67(slot));

  economy_type coarse, fine;
  build_all(coarse);
  build_all(fine);
  coarse.sagan_75706461746521(600);
  for (int frame = 1; frame <= 2400; ++frame) fine.sagan_75706461746521(frame * 0.25);
  assert(stocks(coarse) == stocks(fine));
  assert(coarse.sagan_626174746572795f656e65726779() == fine.sagan_626174746572795f656e65726779());
  assert(coarse.sagan_6175746f6d617469635f7265706169725f636f756e74() == fine.sagan_6175746f6d617469635f7265706169725f636f756e74());
  for (int i = 0; i < 9; ++i) assert(coarse.sagan_6475726162696c697479(i) == fine.sagan_6475726162696c697479(i));

  // Two hours unattended, only FOOD seed retained: no imports/manual repair calls.
  economy_type closed_loop;
  build_all(closed_loop);
  closed_loop.sagan_75706461746521(20);
  empty_stock(closed_loop, true);
  const auto energy_before = closed_loop.sagan_626174746572795f656e65726779();
  std::int64_t peak_demand{};
  for (int time = 21; time <= 7200; ++time)
  {
    closed_loop.sagan_75706461746521(time);
    const auto demand = closed_loop.sagan_64656d616e6465645f7761747473();
    if (demand > peak_demand) peak_demand = demand;
    assert(closed_loop.sagan_6e65745f7761747473() >= 2000 && !closed_loop.sagan_706f7765725f73686f7274616765());
    if (time >= 90) assert(closed_loop.sagan_6c6966655f737570706f72745f7265616479());
    assert(closed_loop.sagan_73657276696365735f7265616479());
    assert(closed_loop.sagan_73746f726167655f75736564() <= closed_loop.sagan_6361706163697479());
    for (int i = 0; i < count; ++i) assert(closed_loop.sagan_73746f636b(i) >= 0);
    for (int i = 0; i < 9; ++i) assert(closed_loop.sagan_6475726162696c697479(i) > 75);
  }
  assert(peak_demand == 118000 && closed_loop.sagan_626174746572795f656e65726779() > energy_before);
  assert(closed_loop.sagan_6175746f6d617469635f7265706169725f636f756e74() > 30 && closed_loop.sagan_73746f636b(kits) > 0);
  assert(closed_loop.sagan_73746f636b(food) > 0 && closed_loop.sagan_73746f636b(food) < 600000);
  assert(closed_loop.sagan_77617465725f737570706c79() > 30000 && closed_loop.sagan_73746f636b(oxygen) > 7600);
  assert(closed_loop.sagan_76656e7465645f687964726f67656e() > 0);

  // Sifter + fabricator alone replenish mineral kits, with no drill/electrolyzer.
  economy_type sifted_only;
  assert(sifted_only.sagan_73746172745f6275696c6421(0, 1));
  assert(sifted_only.sagan_73746172745f6275696c6421(1, 4));
  sifted_only.sagan_75706461746521(20);
  empty_stock(sifted_only, true);
  sifted_only.sagan_75706461746521(7200);
  assert(sifted_only.sagan_73746f636b(kits) > 0 && sifted_only.sagan_6175746f6d617469635f7265706169725f636f756e74() > 20);
  assert(sifted_only.sagan_73657276696365735f7265616479());

  economy_type hungry_base;
  build_all(hungry_base);
  hungry_base.sagan_75706461746521(20);
  assert(hungry_base.sagan_636f6e73756d6521(food, hungry_base.sagan_73746f636b(food) - 25));
  hungry_base.sagan_75706461746521(30);
  assert(hungry_base.sagan_73746f636b(food) == 0 && !hungry_base.sagan_6c6966655f737570706f72745f7265616479());
  hungry_base.sagan_75706461746521(600);
  assert(hungry_base.sagan_73746f636b(food) == 0);
  std::cout << "Scene base economy passed: bulk-only recipes, ice-only drill, sifted-only kits, "
               "shared H2O, food/oxygen use, automatic exact repairs, power/battery/brownout, "
               "frame independence and two-hour unattended surplus.\n";
}
