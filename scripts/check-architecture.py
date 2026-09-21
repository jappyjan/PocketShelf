#!/usr/bin/env python3
"""Keep module dependencies and the shared source manifest honest."""
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parent.parent
manifest = (root / "scripts/sources.sh").read_text()
listed = {"src/main.c"}
for group in re.findall(r'^\w+_SOURCES="([^"]*)"', manifest, re.MULTILINE):
    listed.update(group.split())
actual = {str(path.relative_to(root)) for path in (root / "src").rglob("*.c")}
errors = []
if listed != actual:
    errors.append(f"Source manifest mismatch: {sorted(listed ^ actual)}")
allowed = {
    "core": {"core"},
    "net": {"core", "net"},
    "storage": {"core", "storage"},
    "library": {"core", "net", "library", "storage"},
}
for directory in ("src", "tests"):
    for path in (root / directory).rglob("*"):
        if path.suffix not in (".c", ".h"):
            continue
        relative = path.relative_to(root)
        layer = relative.parts[1]
        for include in re.findall(r'^#include\s+["<]([^">]+)', path.read_text(), re.MULTILINE):
            if include.endswith(".c"):
                errors.append(f"{relative}: include interfaces, not {include}")
            if directory != "src":
                continue
            if layer in allowed and "/" in include and include.split("/")[0] in allowed.keys() | {"app", "ui", "platform"}:
                if include.split("/")[0] not in allowed[layer]:
                    errors.append(f"{relative}: forbidden dependency on {include}")
            if layer in allowed and include == "inkview.h":
                errors.append(f"{relative}: InkView belongs in UI/platform code")
            if relative.as_posix() in {"src/ui/list.c", "src/ui/list.h", "src/ui/widgets.c", "src/ui/widgets.h"} and include.startswith(("app/", "library/", "net/", "platform/")):
                errors.append(f"{relative}: reusable widgets must not depend on {include}")
if errors:
    print("\n".join(errors), file=sys.stderr)
    sys.exit(1)
print("Module dependencies and source manifest passed.")
