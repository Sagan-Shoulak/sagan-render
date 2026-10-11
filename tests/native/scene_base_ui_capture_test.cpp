#include <array>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <cmath>

namespace
{
  using rgb = std::array<std::uint8_t, 3>;
  struct bitmap
  {
    int width{};
    int height{};
    std::vector<rgb> pixels;
    explicit bitmap(const std::filesystem::path &path)
    {
      std::ifstream input{path, std::ios::binary};
      assert(input && "Expected UI capture is missing");
      const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>{input}, {}};
      assert(bytes.size() > 54 && bytes[0] == 'B' && bytes[1] == 'M');
      auto u32 = [&](std::size_t at) {
        return std::uint32_t{bytes[at]} | (std::uint32_t{bytes[at + 1]} << 8) |
               (std::uint32_t{bytes[at + 2]} << 16) | (std::uint32_t{bytes[at + 3]} << 24);
      };
      width = static_cast<int>(u32(18));
      const int signed_height = static_cast<int>(u32(22));
      height = signed_height < 0 ? -signed_height : signed_height;
      const unsigned bits = bytes[28] | (unsigned{bytes[29]} << 8);
      assert(width > 0 && height > 0 && (bits == 24 || bits == 32));
      const auto encoding = u32(30);
      assert(encoding == 0 || (bits == 32 && encoding == 3));
      if (encoding == 3)
      {
        assert(u32(54) == 0x00ff0000 && u32(58) == 0x0000ff00 && u32(62) == 0x000000ff);
      }
      const std::size_t stride = (width * bits + 31) / 32 * 4;
      const std::size_t offset = u32(10);
      assert(bytes.size() >= offset + stride * height);
      pixels.resize(static_cast<std::size_t>(width) * height);
      for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
        {
          const auto at = offset + (signed_height > 0 ? height - 1 - y : y) * stride + x * (bits / 8);
          pixels[static_cast<std::size_t>(y) * width + x] = {bytes[at + 2], bytes[at + 1], bytes[at]};
        }
    }
    auto at(int x, int y) const -> rgb { return pixels[static_cast<std::size_t>(y) * width + x]; }
    auto whites(int x, int y, int w, int h) const -> std::size_t
    {
      std::size_t result{};
      for (int row = y; row < y + h; ++row)
        for (int col = x; col < x + w; ++col)
          result += at(col, row) == rgb{245, 249, 255};
      return result;
    }
    auto ink(int x, int y, int w, int h, rgb paint) const -> std::size_t
    {
      std::size_t result{};
      for (int row=y;row<y+h;++row) for(int col=x;col<x+w;++col) result += at(col,row)==paint;
      return result;
    }
    // Independently specified 5x7 glyph cells at the cards' integer 7-point size.
    // Require ink AND gaps: omitted punctuation or substituted glyphs must fail.
    auto glyph_is(int x, int y, const std::array<std::uint8_t, 7> &rows, rgb paint) const -> bool
    {
      for (int row = 0; row < 7; ++row)
        for (int col = 0; col < 5; ++col)
          if ((at(x + col, y + row) == paint) != ((rows[row] & (1U << (4 - col))) != 0))
            return false;
      return true;
    }
    auto scaled_glyph_is(double x, double y, const std::array<std::uint8_t,7> &rows,
                         rgb paint, double height=8.0) const -> bool
    {
      const double cell=height/7;
      for(int row=0;row<7;++row) for(int col=0;col<5;++col)
      {
        // The bootstrap compositor rounds each glyph cell's origin to pixels.
        const int px=static_cast<int>(std::lround(x+col*cell));
        const int py=static_cast<int>(std::lround(y+row*cell));
        if((at(px,py)==paint) != ((rows[row] & (1U<<(4-col)))!=0)) return false;
      }
      return true;
    }
  };
  auto equal_panel(const bitmap &a, const bitmap &b) -> bool
  {
    for (int y = 90; y < 846; ++y)
      for (int x = 760; x < 1244; ++x)
        if (a.at(x, y) != b.at(x, y)) return false;
    return true;
  }
  auto equal_region(const bitmap &a, const bitmap &b, int x, int y, int w, int h) -> bool
  {
    for (int row = y; row < y + h; ++row)
      for (int col = x; col < x + w; ++col)
        if (a.at(col, row) != b.at(col, row)) return false;
    return true;
  }
}

int main(int argc, char **argv)
{
  assert(argc==2);
  const std::filesystem::path root{argv[1]};
  auto frame=[&](const char *prefix,int at){return bitmap{root/(std::string{prefix}+std::to_string(at)+".bmp")};};
  const rgb up{86,222,157},down{255,132,132},muted{142,164,188},accent{71,174,255};
  const auto initial=frame("frame-",9400),cancelled=frame("frame-",9600),built=frame("frame-",9800);
  assert(initial.width==1280 && initial.height==900);
  // All four empty actions are visible in the enlarged panel, not only their titles.
  for(int x: {780,1016}) for(int y: {715,781}) assert(initial.ink(x,y,202,8,accent)>30);
  assert(cancelled.ink(780,715,202,8,accent)>30);
  assert(built.ink(780,715,202,8,accent)==0);
  assert(!equal_panel(frame("frame-",9850),frame("frame-",9900)));
  assert(!equal_region(initial,built,780,680,180,10));
  const auto full=frame("frame-",10550);
  assert(full.ink(780,715,202,8,accent)==0); // no phantom empty action after confirmation
  assert(frame("keyboard-",10400).ink(780,715,202,8,accent)==0);
  assert(equal_panel(frame("keyboard-",10600),frame("keyboard-",10650))); // R cannot escape chooser
  const auto small=frame("small-",10000);
  assert(small.width==800 && small.height==400);
  assert(small.whites(508,250,232,12)>20); // confirmed slot remains visible after focus scroll
  assert(initial.at(758,100)==rgb({142,203,255}));
  assert(frame("frame-",16100).at(758,100)!=rgb({142,203,255}));
  const auto recipe=frame("economy-",9500),shortage=frame("economy-",19000);
  assert(recipe.whites(772,308,144,70)>70);
  assert(shortage.at(773,795)==rgb({36,55,82}));
  assert(equal_region(shortage,frame("economy-",19750),772,308,460,70)); // disabled click spends nothing
  const auto repaired=frame("economy-",12500);
  assert(repaired.at(773,723)==rgb({36,55,82})); // no over-repair at 85 health
  assert(equal_region(repaired,frame("economy-",13000),772,726,440,12));
  const auto stock_before=frame("economy-",22000),stock_after=frame("economy-",130000);
  assert(stock_after.ink(772,645,320,7,down)>30); // finite food is red, with its stock and ETA
  assert(!equal_region(stock_before,stock_after,772,645,320,7));
  assert(!equal_region(stock_before,stock_after,772,679,320,7)); // kit quantity changes
  assert(!equal_region(stock_after,frame("economy-",660000),772,734,320,9)); // automatic repair count
  const auto power=frame("economy-",70000);
  assert(power.whites(772,200,460,9)>80 && power.whites(772,240,460,9)>80);
  const auto slots=frame("context-",120000);
  // Mixed sifter: copper grows while steel drains. Operation alone is not a surplus badge.
  assert(slots.ink(780,540,204,8,up)>60);
  assert(slots.ink(780,574,204,8,down)>60);
  assert(slots.ink(780,589,204,7,down)>30); // steel reserve and EMPTY IN estimate
  assert(slots.ink(1016,682,204,8,up)>40); // produced kits are green
  assert(slots.ink(1016,716,204,8,down)>60); // fabricator's limiting input remains visible
  const auto early=frame("context-",70000);
  assert(early.ink(780,598,204,8,down)>40 && early.ink(780,613,204,7,down)>30); // life-support food/ETA
  const std::array<std::uint8_t,7> plus{0,4,4,31,4,4,0},slash{1,2,2,4,8,8,16};
  assert(slots.scaled_glyph_is(780+15*48.0/7,540,plus,up));
  assert(slots.scaled_glyph_is(780+23*48.0/7,540,slash,up)); // COPPER SURPLUS +0.01 kg/min
  assert(stock_after.glyph_is(856,754,slash,muted)); // rate-window explanation
  for(int at: {121250,124250,127250,131250,135250,139250,142250,146250,150250})
  {
    const auto inspector=frame("context-",at);
    assert(inspector.whites(772,180,460,12)>40);
    assert(inspector.ink(772,286,450,150,up)+inspector.ink(772,286,450,150,down)+
           inspector.ink(772,286,450,150,muted)>100); // actual colored balance and reserve rows
  }
  for(int at: {122250,125250,129250,133250,137250,140250,144250,148250,151250})
  {
    const auto inspector=frame("context-",at);
    assert(inspector.whites(784,771,240,10)>20); // fixed Back
    assert(inspector.whites(812,812,220,10)>20); // fixed Return
  }
  const auto small_context=frame("context-",155250);
  assert(small_context.width==800 && small_context.height==400);
  assert(small_context.at(501,263)==rgb({55,125,181}));
  assert(small_context.whites(500,178,252,30)==0); // final maintenance rows, not hidden footer controls
  std::cout<<"Scene base UI pixels passed: expanded summaries, green surplus/red deficit, depletion estimates, "
             "mixed input/output balances, + and /min glyphs, all nine inspectors, builds, disabled costs, "
             "no wasted kits, read-only stock, keyboard/modal capture, 800x400 scrolling and orbit return.\n";
}
