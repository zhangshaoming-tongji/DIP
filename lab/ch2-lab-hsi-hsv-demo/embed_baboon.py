# -*- coding: utf-8 -*-
from pathlib import Path
import base64

root = Path(__file__).resolve().parent
png = root / "baboon.png"
js = root / "baboon-data.js"
b64 = base64.b64encode(png.read_bytes()).decode("ascii")
js.write_text(f"const BABOON_DATA_URL = \"data:image/png;base64,{b64}\";\n", encoding="utf-8")
print(png.stat().st_size, js.stat().st_size)
