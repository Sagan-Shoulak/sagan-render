# Planetary albedo textures

These PPM files are renderer fixtures derived from public NASA imagery. PPM is
used so the native demo can upload exact RGB pixels without adding an image
decoder dependency. The renderer samples them with the shared material shader.

- `earth_blue_marble_1024x512.ppm` preserves NASA/Goddard Space Flight Center
  Scientific Visualization Studio's 1024x512 Blue Marble mosaic:
  https://svs.gsfc.nasa.gov/2915/
- `moon_lro_2048x1024.ppm` is converted from the 2048x1024 JPEG edition of
  NASA Scientific Visualization Studio's 2025 LRO color map in the CGI Moon
  Kit:
  https://svs.gsfc.nasa.gov/4720/

Source download SHA-256 values:

- Blue Marble PNG: `d1d3cb36ae6ae452c43cdf7ae0ffc13a959e8a5a2358f58b05e5f623c15119f9`
- LRO 2K JPEG: `f7130a1822681fa7512d7dcfd40db8c10b9ba4f06777910348698260ed7a2170`

Committed PPM SHA-256 values:

- `earth_blue_marble_1024x512.ppm`:
  `c9470550bf332a323c0f1484065d5f2760dea5ffa4f028cc086d44b32559348c`
- `moon_lro_2048x1024.ppm`:
  `1af4f320caff4f3b7c1a412b2355077dd9cde07caa8af33ce901db4cb86220b2`

The source images are equirectangular, north-up maps. The sphere UVs shift their
zero-longitude center by half a turn to match the procedural sphere's first +X
longitude. The RGB values remain display encoded because the bootstrap
offscreen target is UNORM and does not yet apply an sRGB output transfer curve.
The earlier 32x16 files remain only as historical bootstrap fixtures.

Credits: NASA/GSFC Scientific Visualization Studio; Blue Marble data courtesy
of Reto Stockli (NASA/GSFC) and NASA Earth Observatory; lunar mosaic by the
Lunar Reconnaissance Orbiter Camera team, visualized by Ernie Wright (USRA).
