// Exercise the emitted Sagan state itself, not a parallel C++ model.
// The generated program is supplied by the focused scene build's include path.
#define main sagan_generated_scene_entrypoint
#include "program.cpp"
#undef main

#include <cassert>
#include <array>
#include <iostream>
#include <algorithm>
#include <cmath>

void balance_summaries()
{
  using state_type = sagan_626173655f75695f5f4261736555495374617465;
  state_type state;
  const auto e = state.sagan_65636f6e6f6d79();
  assert(state.sagan_62616c616e63655f7374617465(9) == "SAMPLING");
  assert(state.sagan_6465706c6574696f6e5f7365636f6e6473(9) == -2);
  state.sagan_75706461746521(1);
  assert(state.sagan_62616c616e63655f7374617465(9) == "BALANCED");
  assert(state.sagan_6465706c6574696f6e5f7365636f6e6473(9) == -1);
  assert(state.sagan_726573657276655f6c696e65(9) == "600 kg / NO DRAIN");
  state.sagan_75706461746521(60);
  assert(state.sagan_62616c616e63655f7374617465(9) == "DEFICIT");
  assert(state.sagan_6465706c6574696f6e5f7365636f6e6473(9) == 119940);
  assert(state.sagan_726573657276655f6c696e65(9) == "599.7 kg / EMPTY IN 1 d 9 h");
  assert(state.sagan_73756d6d6172795f7265736f75726365(0, 1) == 9); // chassis surfaces the finite food reserve
  assert(e->sagan_636f6e73756d6521(9, e->sagan_73746f636b(9)));
  assert(state.sagan_62616c616e63655f7374617465(9) == "EMPTY");
  assert(state.sagan_726573657276655f6c696e65(9) == "0 kg / EMPTY NOW");

  // Kits use item stock against milli-items/minute, not kilograms or whole rates.
  state_type item;
  const auto kit_e = item.sagan_65636f6e6f6d79();
  item.sagan_75706461746521(1);
  kit_e->sagan_7265636f72645f666c6f7721(0, 10, 1, 0);
  assert(item.sagan_6465706c6574696f6e5f7365636f6e6473(10) == 8);
  assert(item.sagan_726573657276655f6c696e65(10) == "8 items / EMPTY IN 8 s");
  assert(item.sagan_73756d6d6172795f7265736f75726365(0, 1) == 10); // imminent kit shortage takes priority
  assert(kit_e->sagan_7365745f73756e6c6967687421(0));
  item.sagan_75706461746521(2);
  const auto energy = kit_e->sagan_626174746572795f656e65726779();
  const auto drain = -kit_e->sagan_626174746572795f666c6f775f7761747473();
  assert(item.sagan_6465706c6574696f6e5f7365636f6e6473(12) == (energy + drain - 1) / drain);
  assert(item.sagan_6465706c6574696f6e5f7365636f6e6473(13) == item.sagan_6465706c6574696f6e5f7365636f6e6473(12));

  state_type mixed;
  const auto loop = mixed.sagan_65636f6e6f6d79();
  for (int slot=0;slot<4;++slot) assert(mixed.sagan_696e7374616c6c21(slot,slot+1));
  mixed.sagan_75706461746521(120);
  assert(mixed.sagan_62616c616e63655f6261646765(5) == "1 DEFICIT / 7 SURPLUS");
  assert(mixed.sagan_73797374656d5f6861735f64656669636974(5));
  assert(mixed.sagan_62616c616e63655f7374617465(2) == "SURPLUS");
  assert(mixed.sagan_62616c616e63655f7374617465(0) == "DEFICIT");
  assert(mixed.sagan_73756d6d6172795f7265736f75726365(5, 1) == 0);
  assert(mixed.sagan_6465706c6574696f6e5f7365636f6e6473(0) == (loop->sagan_73746f636b(0)*60+8999)/9000);
  assert(mixed.sagan_6465706c6574696f6e5f7365636f6e6473(11) == -1);
  assert(mixed.sagan_73797374656d5f7265736f7572636573(4) == 10);
  assert(mixed.sagan_73756d6d6172795f7265736f75726365(4, 0) == 0); // not liquid-water depletion while shared H2O grows
  const auto mass = loop->sagan_73746f726167655f75736564();
  for(int system=0;system<9;++system) for(int row=0;row<mixed.sagan_73797374656d5f7265736f7572636573(system);++row)
  {
    const auto r=mixed.sagan_73797374656d5f7265736f75726365(system,row);
    assert(mixed.sagan_62616c616e63655f6c696e65(r).size()<=31);
    assert(mixed.sagan_726573657276655f6c696e65(r).size()<=33);
  }
  assert(loop->sagan_73746f726167655f75736564()==mass);
  for(const auto &size: {std::array{1280.0,800.0},std::array{960.0,540.0},std::array{800.0,400.0}})
  {
    mixed.sagan_7570646174655f6c61796f757421(size[0],size[1]);
    assert(mixed.sagan_636f6c756d6e73() == (size[0]>=1100 ? 2 : 1));
    for(int id: {4,10,11,12,13,20,21,22,23})
    {
      while(mixed.sagan_666f6375736564()!=id) mixed.sagan_666f6375735f6e65787421(false);
      const auto y=std::max(174.0,mixed.sagan_636172645f746f70(id)-mixed.sagan_7363726f6c6c())+10;
      assert(mixed.sagan_636f6e74726f6c5f6174(mixed.sagan_636172645f6c656674(id)+12,y)==id);
    }
  }
  std::cout<<"Balance summaries passed: mixed resources, net-based mass/item/battery depletion, "
             "empty/zero/startup states, read-only queries and responsive card hit regions.\n";
}

void context_telemetry()
{
  using state_type = sagan_626173655f75695f5f4261736555495374617465;
  state_type state;
  const auto e = state.sagan_65636f6e6f6d79();
  assert(state.sagan_73797374656d5f737461747573(1) == "GENERATING");
  assert(state.sagan_73797374656d5f737461747573(2) == "HOLDING");
  assert(state.sagan_73797374656d5f737461747573(3) == "RUNNING");
  assert(state.sagan_73797374656d5f737461747573(4) == "NOMINAL");
  assert(state.sagan_73797374656d5f62616c616e6365(3) == "FOOD DEFICIT");
  const auto initial_energy = e->sagan_626174746572795f656e65726779();
  state.sagan_75706461746521(1.0);
  assert(state.sagan_73797374656d5f737461747573(2) == "CHARGING");
  assert(e->sagan_626174746572795f666c6f775f7761747473() == e->sagan_626174746572795f656e65726779() - initial_energy);
  assert(e->sagan_626174746572795f666c6f775f7761747473() == 86400); // actual stored-side 90% charge
  assert(state.sagan_73797374656d5f62616c616e6365(1) == "SURPLUS");
  e->sagan_7365745f73756e6c6967687421(0);
  const auto charged_energy = e->sagan_626174746572795f656e65726779();
  state.sagan_75706461746521(2.0);
  assert(state.sagan_73797374656d5f737461747573(1) == "NO SUNLIGHT");
  assert(state.sagan_73797374656d5f737461747573(2) == "DISCHARGING");
  assert(state.sagan_73797374656d5f62616c616e6365(1) == "DEFICIT");
  assert(e->sagan_626174746572795f666c6f775f7761747473() == e->sagan_626174746572795f656e65726779() - charged_energy);
  assert(e->sagan_636f6e73756d6521(9, e->sagan_73746f636b(9)));
  assert(state.sagan_73797374656d5f737461747573(3) == "NEEDS FOOD");
  assert(state.sagan_73797374656d5f7761726e696e67(3));

  state_type loop;
  const auto production = loop.sagan_65636f6e6f6d79();
  for (int slot = 0; slot < 4; ++slot) assert(loop.sagan_696e7374616c6c21(slot, slot + 1));
  assert(loop.sagan_73797374656d5f737461747573(5) == "BUILDING 10 s");
  assert(loop.sagan_636f6e746578745f726f7773(5) == 2);
  std::array<std::int64_t, 11> previous{};
  for (int r = 0; r < 11; ++r) previous[r] = production->sagan_73746f636b(r);
  loop.sagan_75706461746521(60.0);
  assert(production->sagan_666c6f775f77696e646f775f7365636f6e6473() == 60);
  for (int r = 0; r < 11; ++r)
  {
    const auto unit = r == 10 ? 1000 : 1;
    assert(production->sagan_6e65745f72617465(r) == (production->sagan_73746f636b(r) - previous[r]) * unit);
    previous[r] = production->sagan_73746f636b(r);
  }
  assert(production->sagan_696e7075745f72617465(3, 9) == 300);
  assert(production->sagan_696e7075745f72617465(3, 7) == 91200);
  assert(production->sagan_6f75747075745f72617465(7, 7) == 64000);
  assert(production->sagan_6f75747075745f72617465(8, 10) == 2000); // milli-kits, not kg
  assert(loop.sagan_73797374656d5f62616c616e6365(7) == "DEFICIT"); // startup is not steady-state
  assert(loop.sagan_636172645f636f6e74657874(5) == "Cu -0.02 kg/min"); // no sifter batch before 70 s
  loop.sagan_75706461746521(120.0);
  for (int r = 0; r < 11; ++r)
  {
    const auto unit = r == 10 ? 1000 : 1;
    assert(production->sagan_6e65745f72617465(r) == (production->sagan_73746f636b(r) - previous[r]) * unit);
  }
  assert(production->sagan_6f75747075745f72617465(5, 2) == 40); // gram trace is retained
  assert(production->sagan_696e7075745f72617465(8, 2) == 30);
  assert(production->sagan_6e65745f72617465(2) == 10);
  assert(loop.sagan_73797374656d5f62616c616e6365(5) == "SURPLUS");
  assert(loop.sagan_636172645f636f6e74657874(5) == "Cu +0.01 kg/min");
  assert(loop.sagan_636172645f636f6e74657874(6) == "H2O +3.4 kg/min"); // water seed still above sifter target
  assert(loop.sagan_636172645f636f6e74657874(7) == "O2 +29.8 kg/min");
  assert(loop.sagan_636172645f636f6e74657874(8) == "+3 kits/min");
  for (int system = 5; system < 9; ++system)
    assert(loop.sagan_636172645f636f6e74657874(system).size() <= 18); // 7-point text fits 108 px
  assert(production->sagan_6f75747075745f72617465(6, 6) == 114000);
  assert(loop.sagan_73797374656d5f737461747573(7) == "RUNNING");
  assert(loop.sagan_73797374656d5f62616c616e6365(7) == "SURPLUS");
  assert(production->sagan_6f75747075745f72617465(3, 10) == 0);
  assert(loop.sagan_666c6f775f6c696e65(3, 10) == "IN 0 / OUT 0 kits/min");
  assert(production->sagan_696e7075745f72617465(-1, 0) == 0 && production->sagan_6f75747075745f72617465(9, 0) == 0);
  // Every installed system has specific context and scrolling reaches its final
  // maintenance rows. Merely reading every line must not mutate inventory.
  const auto stock_before = production->sagan_73746f726167655f75736564();
  for (int system = 0; system < 9; ++system)
  {
    for (int row = 0; row < loop.sagan_636f6e746578745f726f7773(system); ++row)
    {
      const auto line = loop.sagan_636f6e746578745f6c696e65(system, row);
      assert(!line.empty());
      assert(line.size() <= 36); // 8-point fixed glyph advance fits 252 logical units
    }
    loop.sagan_616374697661746521(system == 0 ? 4 : (system < 5 ? 9 + system : 15 + system));
    loop.sagan_7570646174655f6c61796f757421(800, 400);
    loop.sagan_7363726f6c6c5f627921(-100);
    assert(loop.sagan_7363726f6c6c() == loop.sagan_6d61785f7363726f6c6c());
    assert(loop.sagan_6d61785f7363726f6c6c() > 0);
    assert(loop.sagan_636f6e74726f6c5f6174(520, 280) == 120); // fixed Back never scrolls away
    loop.sagan_616374697661746521(120);
  }
  assert(production->sagan_73746f726167655f75736564() == stock_before);
  assert(loop.sagan_636f6e746578745f6c696e65(3, 13) == "FOOD DEFICIT / NO LOCAL PRODUCER");
  assert(loop.sagan_636f6e746578745f6c696e65(7, 11).find("VENTED H2") != std::string::npos);

  assert(production->sagan_636f6e73756d6521(2, production->sagan_73746f636b(2)));
  assert(loop.sagan_73797374656d5f737461747573(8) == "NEEDS MINERALS");
  assert(production->sagan_6164645f73746f636b21(10, 20 - production->sagan_73746f636b(10)));
  assert(loop.sagan_73797374656d5f737461747573(8) == "TARGET MET / IDLE");
  assert(production->sagan_636f6e73756d6521(5, production->sagan_73746f636b(5)));
  assert(production->sagan_636f6e73756d6521(6, production->sagan_73746f636b(6)));
  assert(loop.sagan_73797374656d5f737461747573(7) == "H2O RESERVE / IDLE");
  assert(loop.sagan_73797374656d5f737461747573(3) == "NEEDS WATER OR ICE");
  assert(production->sagan_636f6e73756d6521(10, production->sagan_73746f636b(10)));
  assert(production->sagan_64616d6167655f73797374656d21(1, 100));
  assert(loop.sagan_73797374656d5f737461747573(1) == "BROKEN / REPAIR");
  assert(loop.sagan_73797374656d5f737461747573(5) == "SERVICE FAULT");

  state_type vent;
  const auto gases = vent.sagan_65636f6e6f6d79();
  assert(vent.sagan_696e7374616c6c21(0, 3));
  assert(gases->sagan_6164645f73746f636b21(8, 50000));
  const auto h2 = gases->sagan_73746f636b(8);
  vent.sagan_75706461746521(60.0);
  assert(gases->sagan_687964726f67656e5f76656e745f72617465() == 8000);
  assert(gases->sagan_6f75747075745f72617465(5, 8) == 8000); // gross output includes venting
  assert(gases->sagan_6e65745f72617465(8) == gases->sagan_73746f636b(8) - h2); // never count gas twice
  state_type full_storage;
  const auto storage = full_storage.sagan_65636f6e6f6d79();
  assert(storage->sagan_6164645f73746f636b21(0, storage->sagan_6361706163697479() - storage->sagan_73746f726167655f75736564()));
  assert(full_storage.sagan_73797374656d5f737461747573(4) == "FULL / OUTPUT BLOCKED");
  assert(full_storage.sagan_6d616e6461746f72795f636172645f6c696e65(4).size() <= 39);
  for (int system = 1; system <= 4; ++system)
    assert(loop.sagan_6d616e6461746f72795f636172645f6c696e65(system).size() <= 39);
  std::cout << "System context passed: all nine systems, measured flows, startup deficit, "
               "steady surplus, battery directions, input/target/repair states and scrolling.\n";
}

int main()
{
  balance_summaries();
  context_telemetry();
  using state_type = sagan_626173655f75695f5f4261736555495374617465;
  state_type state;
  auto activate = [&](const std::int64_t id) { state.sagan_616374697661746521(id); };
  auto installed = [&] { return state.sagan_696e7374616c6c6564(); };
  auto modal = [&] { return state.sagan_6d6f64616c(); };
  auto machine = [&](const std::int64_t slot) { return state.sagan_6d616368696e65(slot); };
  auto pointer = [&](double x, double y, bool down, bool up) {
    state.sagan_706f696e74657221(x, y, down, up);
  };
  auto reveal = [&](int id) {
    while (state.sagan_666f6375736564() != id) state.sagan_666f6375735f6e65787421(false);
    return std::array{state.sagan_636172645f6c656674(id) + 12,
      std::max(174.0, state.sagan_636172645f746f70(id) - state.sagan_7363726f6c6c()) + 10};
  };
  const auto chassis = state.sagan_63686173736973();
  assert(chassis->sagan_6d616e6461746f72795f736c6f7473() == 4);
  assert(chassis->sagan_6d616368696e655f736c6f7473() == 4);
  for (int slot = 0; slot < 4; ++slot)
  {
    assert(chassis->sagan_6d616e6461746f72795f74797065(slot) == slot + 1);
    assert(machine(slot) == 0);
  }
  assert(chassis->sagan_6d616e6461746f72795f74797065(-1) == 0);
  assert(chassis->sagan_6d616e6461746f72795f74797065(4) == 0);
  assert(installed() == 0);

  // Mandatory inspection never opens a machine chooser or mutates capacity.
  for (int id = 10; id <= 13; ++id)
  {
    activate(id);
    assert(!modal() && installed() == 0);
    assert(state.sagan_696e73706563746564() == id);
    activate(120);
  }
  activate(4);
  assert(state.sagan_696e73706563746564() == 4);
  activate(120);

  // Down without up, and release outside the captured control, do nothing.
  const auto slot_point = reveal(20);
  pointer(slot_point[0], slot_point[1], true, false);
  assert(!modal());
  state.sagan_7363726f6c6c5f627921(0); // idle frame must not cancel capture
  assert(state.sagan_70726573736564() == 20);
  pointer(slot_point[0] - 24, slot_point[1], false, true);
  assert(!modal());
  pointer(slot_point[0], slot_point[1], true, false);
  state.sagan_7363726f6c6c5f627921(0);
  pointer(slot_point[0], slot_point[1], false, true);
  assert(modal() && state.sagan_6275696c645f736c6f74() == 0);
  activate(101); // choose mining drill
  assert(state.sagan_63686f696365() == 2 && installed() == 0);
  activate(2);   // modal prevents tab and return activation
  activate(30);
  assert(state.sagan_746162() == 0);
  assert(!state.sagan_74616b655f72657475726e21());
  activate(111); // cancel
  assert(!modal() && installed() == 0);

  // Keyboard and pointer share exactly the same confirm path.
  activate(20);
  activate(102);
  assert(state.sagan_666f6375736564() == 110);
  state.sagan_6b6579626f6172645f616374697661746521();
  assert(!modal() && installed() == 1 && machine(0) == 3);
  const auto economy = state.sagan_65636f6e6f6d79();
  assert(economy->sagan_6275696c64696e67(0));
  assert(economy->sagan_73746f636b(0) == 1220000);
  activate(20);
  assert(!modal() && state.sagan_696e73706563746564() == 20);
  activate(120);
  assert(!state.sagan_696e7374616c6c21(0, 1)); // occupied slot cannot be overwritten
  assert(!state.sagan_696e7374616c6c21(-1, 1));
  assert(!state.sagan_696e7374616c6c21(4, 1));
  assert(!state.sagan_696e7374616c6c21(1, 5));
  for (int slot = 1; slot < 4; ++slot)
  {
    activate(20 + slot);
    if (slot == 2) activate(101);
    if (slot == 3) activate(103);
    activate(110);
  }
  assert(installed() == 4);
  state.sagan_75706461746521(20.0);
  for (int slot = 0; slot < 4; ++slot) assert(!economy->sagan_6275696c64696e67(slot));

  // Affordability disables both pointer and keyboard start; focus skips it.
  state_type scarce;
  const auto scarce_economy = scarce.sagan_65636f6e6f6d79();
  assert(scarce_economy->sagan_636f6e73756d6521(4, 95001)); // silicon one gram short
  scarce.sagan_616374697661746521(20);
  assert(!scarce.sagan_656e61626c6564(110));
  assert(scarce.sagan_636f6e74726f6c5f6174(680, 447) == 0);
  scarce.sagan_616374697661746521(110);
  assert(scarce.sagan_6d6f64616c() && scarce.sagan_696e7374616c6c6564() == 0);
  scarce.sagan_616374697661746521(103);
  assert(!scarce.sagan_656e61626c6564(110));
  scarce.sagan_666f6375735f6e65787421(false);
  assert(scarce.sagan_666f6375736564() == 111);
  assert(scarce_economy->sagan_6164645f73746f636b21(4, 1));
  assert(scarce.sagan_656e61626c6564(110));
  scarce.sagan_63616e63656c21();
  scarce.sagan_616374697661746521(4);
  const auto kits_before_repair = scarce_economy->sagan_73746f636b(10);
  assert(!scarce.sagan_656e61626c6564(121)); // 85 + 25 would over-repair
  scarce.sagan_616374697661746521(121);
  assert(scarce_economy->sagan_73746f636b(10) == kits_before_repair);
  assert(scarce_economy->sagan_64616d6167655f73797374656d21(0, 10));
  assert(scarce.sagan_656e61626c6564(121));
  scarce.sagan_616374697661746521(121);
  assert(scarce_economy->sagan_6475726162696c697479(0) == 100);
  assert(scarce_economy->sagan_73746f636b(10) == kits_before_repair - 1);
  assert(!scarce.sagan_656e61626c6564(121));
  assert(scarce_economy->sagan_64616d6167655f73797374656d21(0, 25));
  scarce.sagan_75706461746521(1);
  assert(scarce_economy->sagan_6475726162696c697479(0) == 100);
  assert(scarce_economy->sagan_6175746f6d617469635f7265706169725f636f756e74() == 1);

  // Resources and Details expose navigation only: stale slot actions rejected.
  for (int tab = 2; tab <= 3; ++tab)
  {
    activate(tab);
    activate(20);
    activate(110);
    assert(!modal() && installed() == 4);
    assert(state.sagan_746162() == tab - 1);
  }
  activate(1);

  // Relayout cancels pointer capture, retaining keyboard focus and slot state.
  pointer(state.sagan_70616e656c5f6c656674() + 12, 155, true, false);
  const auto focus = state.sagan_666f6375736564();
  state.sagan_7570646174655f6c61796f757421(800, 400);
  pointer(520, 155, false, true);
  assert(state.sagan_746162() == 0);
  assert(state.sagan_666f6375736564() == focus && installed() == 4);
  state.sagan_7363726f6c6c5f627921(-100);
  assert(state.sagan_7363726f6c6c() == state.sagan_6d61785f7363726f6c6c());
  assert(state.sagan_636f6e74726f6c5f6174(state.sagan_636172645f6c656674(23) + 12,
    std::max(174.0, state.sagan_636172645f746f70(23) - state.sagan_7363726f6c6c()) + 10) == 23);
  state.sagan_7363726f6c6c5f627921(100);
  assert(state.sagan_7363726f6c6c() == 0);
  activate(30);
  assert(state.sagan_74616b655f72657475726e21());
  assert(!state.sagan_74616b655f72657475726e21());
  std::cout << "Scene base UI state passed: chassis, restrictions, four empty slots, "
               "timed builds, cost/disabled controls, repairs, cancel/confirm, "
               "occupied protection, read-only panels, input and relayout.\n";
}
