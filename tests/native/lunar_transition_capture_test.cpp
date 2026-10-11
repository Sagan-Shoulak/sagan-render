#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace
{
  auto fields(const std::string &line) -> std::map<std::string, std::string>
  {
    std::map<std::string, std::string> result;
    std::istringstream input{line};
    for (std::string word; input >> word;)
      if (const auto equals = word.find('='); equals != std::string::npos)
        result.emplace(word.substr(0, equals), word.substr(equals + 1));
    return result;
  }

  auto values(std::string text) -> std::vector<double>
  {
    std::replace(text.begin(), text.end(), ',', ' ');
    std::istringstream input{text};
    std::vector<double> result;
    for (double value; input >> value;) result.push_back(value);
    return result;
  }

  auto check_pixels(const std::string &path, const bool orbit) -> void
  {
    std::ifstream input{path, std::ios::binary};
    assert(input);
    const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>{input}, {}};
    const auto little = [&](const std::size_t at, const unsigned count)
    {
      std::uint32_t value{};
      for (unsigned i = 0; i < count; ++i)
        value |= static_cast<std::uint32_t>(bytes.at(at + i)) << (i * 8);
      return value;
    };
    assert(bytes[0] == 'B' && bytes[1] == 'M');
    const auto offset = little(10, 4), width = little(18, 4);
    const auto signed_height = static_cast<std::int32_t>(little(22, 4));
    const auto height = static_cast<std::uint32_t>(std::abs(signed_height));
    const auto pixel_bytes = little(28, 2) / 8;
    assert(width == 960 && height == 540 && (pixel_bytes == 3 || pixel_bytes == 4));
    const auto stride = (width * pixel_bytes + 3) & ~3U;
    std::map<unsigned, unsigned> colors;
    unsigned nonblack{};
    const unsigned x0 = orbit ? 440 : 50, x1 = orbit ? 520 : 180;
    const unsigned y0 = orbit ? 248 : 100, y1 = orbit ? 328 : 200;
    for (unsigned y = y0; y < y1; ++y)
      for (unsigned x = x0; x < x1; ++x)
      {
        const auto p = offset + (signed_height > 0 ? height - y - 1 : y) * stride + x * pixel_bytes;
        const unsigned color = bytes[p] | (bytes[p + 1] << 8) | (bytes[p + 2] << 16);
        ++colors[color];
        if (color) ++nonblack;
      }
    if (orbit) assert(nonblack > 800); // The Moon must still render in orbit.
    else
    {
      assert(colors.size() > 50); // Close detail cannot be replaced by a flat fill.
      assert(nonblack == (x1 - x0) * (y1 - y0)); // No missing/clipped ground.
    }
  }
}

int main(int argc, char **argv)
{
  assert(argc == 2);
  std::ifstream input{argv[1]};
  assert(input);
  std::string capture;
  std::vector<std::map<std::string, std::string>> draws;
  std::vector<double> settled_forward, settled_up;
  unsigned frame_count{};
  const auto check_frame = [&]
  {
    if (capture.empty()) return;
    const auto start = capture.find("frame-") + 6;
    const auto time = std::stoul(capture.substr(start));
    unsigned moons{}, rocks{};
    for (const auto &draw : draws)
    {
      if (draw.at("id") == "3")
      {
        ++moons;
        assert(draw.at("radius") == "1737400");
        assert(draw.at("terrain_indices") == "589824");
        assert(draw.at("map") == "2");
        if (time >= 6000) assert(std::stod(draw.at("near")) < 10000);
        if (time >= 7000) assert(std::stod(draw.at("detail")) > 0.9);
        if (time >= 8500 && time <= 9500)
        {
          const auto forward = values(draw.at("forward")), up = values(draw.at("up"));
          if (settled_forward.empty()) { settled_forward = forward; settled_up = up; }
          for (unsigned i = 0; i < 3; ++i)
          {
            assert(std::abs(forward[i] - settled_forward[i]) < 1e-10);
            assert(std::abs(up[i] - settled_up[i]) < 1e-10);
          }
        }
      }
      if (draw.at("id") == "4")
      { ++rocks; assert(draw.at("radius") == "6"); assert(draw.at("map") == "0"); }
      for (const auto &other : draws)
        for (const auto *field : {"near", "far", "fov", "forward", "up", "viewport"})
          assert(draw.at(field) == other.at(field));
    }
    assert(moons == 1);
    assert(rocks == (time >= 4000 ? 1U : 0U));
    if (time == 3900 || time == 9600) check_pixels(capture, time == 3900);
    ++frame_count;
  };
  for (std::string line; std::getline(input, line);)
  {
    if (line.starts_with("SAGAN_SCENE_FRAME"))
    {
      check_frame();
      capture = fields(line).at("capture");
      draws.clear();
    }
    else if (line.starts_with("SAGAN_SCENE_DRAW")) draws.push_back(fields(line));
  }
  check_frame();
  assert(frame_count >= 120);
  std::cout << "Lunar transition passed: " << frame_count
            << " frames, one Moon, one primitive rock, shared projection, stable settlement, orbital pixels and close detail.\n";
}
