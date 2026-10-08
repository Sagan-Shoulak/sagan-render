# Planetary albedo grids

These small PPM files are renderer fixtures derived from public NASA imagery.
They are intentionally matched to the demo sphere's 32 longitude by 16
latitude cells; they are not replacements for production sampled textures.

- `earth_blue_marble_32x16.ppm` is averaged from NASA/Goddard Space Flight
  Center Scientific Visualization Studio's 1024x512 Blue Marble mosaic:
  https://svs.gsfc.nasa.gov/2915/
- `moon_lro_32x16.ppm` is averaged from NASA Scientific Visualization Studio's
  LRO WAC natural-color global mosaic thumbnail in the CGI Moon Kit:
  https://svs.gsfc.nasa.gov/4720/

The source images are equirectangular, north-up maps. The runtime shifts their
zero-longitude center by half the grid width to match the procedural sphere's
first +X longitude cell. The RGB values remain display encoded because the
bootstrap offscreen target is UNORM and does not yet apply an sRGB output
transfer curve. A later native texture-sampling shader should own the full
sRGB-to-linear-to-sRGB path.

Credits: NASA/GSFC Scientific Visualization Studio; Blue Marble data courtesy
of Reto Stockli (NASA/GSFC) and NASA Earth Observatory; lunar mosaic by the
Lunar Reconnaissance Orbiter Camera team, visualized by Ernie Wright (USRA).
