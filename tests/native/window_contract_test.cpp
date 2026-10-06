#include "../../libraries/render/native/window_contract.hpp"

#include <cassert>
#include <stdexcept>

int main()
{
  using sagan_render::native::window_contract;

  window_contract contract;
  assert(!contract.metrics().open);
  assert(contract.metrics().display_scale == 1.0);

  contract.open(960, 540, 1920, 1080, 2.0, true);
  assert(contract.metrics().open);
  assert(contract.metrics().focused);
  assert(contract.metrics().logical_width == 960);
  assert(contract.metrics().drawable_width == 1920);
  assert(contract.consume_resized());
  assert(!contract.consume_resized());

  contract.update_dimensions(960, 540, 1920, 1080, 2.0);
  assert(!contract.consume_resized());
  contract.update_dimensions(1280, 720, 1920, 1080, 1.5);
  assert(contract.metrics().logical_width == 1280);
  assert(contract.metrics().display_scale == 1.5);
  assert(contract.consume_resized());

  contract.set_focused(false);
  assert(!contract.metrics().focused);
  assert(!contract.consume_resized());
  contract.request_close();
  assert(contract.metrics().close_requested);

  contract.close();
  assert(!contract.metrics().open);
  assert(contract.metrics().drawable_width == 0);
  assert(contract.metrics().display_scale == 1.0);
  assert(contract.metrics().close_requested);

  bool rejected = false;
  try { contract.open(0, 540, 0, 540, 1.0, false); }
  catch (const std::invalid_argument &) { rejected = true; }
  assert(rejected);
}
