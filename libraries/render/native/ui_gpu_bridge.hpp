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
auto sagan_5f5f72656e6465725f75695f74657874(
  double x, double y, const std::string &value, double height,
  std::int64_t red, std::int64_t green, std::int64_t blue) -> void;
auto sagan_5f5f72656e6465725f75695f70726573656e74() -> void;
auto sagan_5f5f72656e6465725f75695f6b65795f70726573736564(
  const std::string &key) -> bool;
auto sagan_5f5f72656e6465725f75695f706f696e7465725f70726573736564() -> bool;
auto sagan_5f5f72656e6465725f75695f706f696e7465725f72656c6561736564() -> bool;
auto sagan_5f5f72656e6465725f75695f706f696e7465725f78() -> double;
auto sagan_5f5f72656e6465725f75695f706f696e7465725f79() -> double;
