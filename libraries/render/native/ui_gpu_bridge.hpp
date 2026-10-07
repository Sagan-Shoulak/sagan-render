#pragma once

#include <cstdint>
#include <string>

auto sagan_5f5f72656e6465725f75695f6f70656e(
  const std::string &title, std::int64_t width, std::int64_t height) -> bool;
auto sagan_5f5f72656e6465725f75695f706f6c6c() -> bool;
auto sagan_5f5f72656e6465725f75695f636c6f7365() -> void;
auto sagan_5f5f72656e6465725f75695f7769647468() -> double;
auto sagan_5f5f72656e6465725f75695f686569676874() -> double;
auto sagan_5f5f72656e6465725f656c61707365645f7365636f6e6473() -> double;
auto sagan_5f5f72656e6465725f75695f626567696e() -> void;
auto sagan_5f5f72656e6465725f75695f66696c6c(
  double x, double y, double width, double height,
  std::int64_t red, std::int64_t green, std::int64_t blue) -> void;
auto sagan_5f5f72656e6465725f75695f6c696e65(
  double first_x, double first_y, double second_x, double second_y,
  double thickness, double clip_x, double clip_y, double clip_width,
  double clip_height, std::int64_t red, std::int64_t green,
  std::int64_t blue) -> void;
auto sagan_5f5f72656e6465725f75695f74657874(
  double x, double y, const std::string &value, double height,
  std::int64_t red, std::int64_t green, std::int64_t blue) -> void;
auto sagan_5f5f72656e6465725f75695f6d6573685f737068657265(
  double viewport_x, double viewport_y, double viewport_width, double viewport_height,
  double camera_x, double camera_y, double camera_z,
  double forward_x, double forward_y, double forward_z,
  double up_x, double up_y, double up_z, double field_of_view,
  double near_distance, double far_distance,
  double body_x, double body_y, double body_z, double radius,
  std::int64_t appearance, double minimum_radius, bool selected) -> void;
auto sagan_5f5f72656e6465725f75695f70726573656e74() -> void;
auto sagan_5f5f72656e6465725f75695f6b65795f70726573736564(
  const std::string &key) -> bool;
auto sagan_5f5f72656e6465725f75695f706f696e7465725f70726573736564() -> bool;
auto sagan_5f5f72656e6465725f75695f706f696e7465725f72656c6561736564() -> bool;
auto sagan_5f5f72656e6465725f75695f706f696e7465725f78() -> double;
auto sagan_5f5f72656e6465725f75695f706f696e7465725f79() -> double;
auto sagan_5f5f72656e6465725f75695f6f726269745f64656c74615f78() -> double;
auto sagan_5f5f72656e6465725f75695f6f726269745f64656c74615f79() -> double;
auto sagan_5f5f72656e6465725f75695f7363726f6c6c5f79() -> double;
