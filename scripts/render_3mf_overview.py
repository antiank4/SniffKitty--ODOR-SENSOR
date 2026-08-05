"""Render lightweight isometric SVG previews directly from 3MF mesh files."""

from __future__ import annotations

import math
import sys
import xml.etree.ElementTree as ET
import zipfile
from pathlib import Path


NS = {"m": "http://schemas.microsoft.com/3dmanufacturing/core/2015/02"}


def read_mesh(path: Path):
    with zipfile.ZipFile(path) as archive:
        model_name = next(name for name in archive.namelist() if name.lower().endswith(".model"))
        root = ET.fromstring(archive.read(model_name))

    vertices = []
    triangles = []
    for mesh in root.findall(".//m:mesh", NS):
        offset = len(vertices)
        for vertex in mesh.findall("./m:vertices/m:vertex", NS):
            vertices.append(tuple(float(vertex.get(axis, "0")) for axis in ("x", "y", "z")))
        for triangle in mesh.findall("./m:triangles/m:triangle", NS):
            triangles.append(tuple(offset + int(triangle.get(key, "0")) for key in ("v1", "v2", "v3")))
    if not vertices or not triangles:
        raise ValueError(f"No mesh geometry found in {path}")
    return vertices, triangles


def project(vertex):
    x, y, z = vertex
    angle = math.radians(32)
    return ((x - y) * math.cos(angle), z + (x + y) * math.sin(angle) * 0.55)


def panel(path: Path, x0: float, y0: float, width: float, height: float, title: str):
    vertices, triangles = read_mesh(path)
    projected = [project(vertex) for vertex in vertices]
    min_x = min(point[0] for point in projected)
    max_x = max(point[0] for point in projected)
    min_y = min(point[1] for point in projected)
    max_y = max(point[1] for point in projected)
    scale = min((width - 36) / max(max_x - min_x, 1), (height - 82) / max(max_y - min_y, 1))

    def point(index):
        px, py = projected[index]
        return (
            x0 + 18 + (px - min_x) * scale,
            y0 + height - 44 - (py - min_y) * scale,
        )

    xs = [v[0] for v in vertices]
    ys = [v[1] for v in vertices]
    zs = [v[2] for v in vertices]
    dimensions = f"{max(xs)-min(xs):.1f} × {max(ys)-min(ys):.1f} × {max(zs)-min(zs):.1f} mm"

    ordered = sorted(triangles, key=lambda tri: sum(vertices[i][0] + vertices[i][1] + vertices[i][2] for i in tri))
    shapes = []
    for tri in ordered:
        coords = " ".join(f"{px:.1f},{py:.1f}" for px, py in (point(index) for index in tri))
        shapes.append(f'<polygon points="{coords}" fill="#ef4444" fill-opacity="0.11" stroke="#991b1b" stroke-width="0.35"/>')

    return f"""
    <g>
      <rect x="{x0}" y="{y0}" width="{width}" height="{height}" rx="16" fill="#fff" stroke="#cbd5e1"/>
      <text x="{x0 + 18}" y="{y0 + 28}" class="panel-title">{title}</text>
      <text x="{x0 + 18}" y="{y0 + 49}" class="dimension">Bounding box: {dimensions}</text>
      {''.join(shapes)}
    </g>"""


def main():
    if len(sys.argv) != 5:
        raise SystemExit("usage: render_3mf_overview.py OUTPUT.svg BODY.3mf LID_V1.3mf LID_V2.3mf")
    output, body, lid1, lid2 = map(Path, sys.argv[1:])
    panels = [
        panel(body, 24, 105, 572, 470, "Enclosure body"),
        panel(lid1, 616, 105, 280, 470, "Lid — version 1"),
        panel(lid2, 916, 105, 280, 470, "Lid — version 2"),
    ]
    svg = f"""<svg xmlns="http://www.w3.org/2000/svg" width="1220" height="610" viewBox="0 0 1220 610" role="img" aria-labelledby="title desc">
  <title id="title">SniffKitty enclosure CAD overview</title>
  <desc id="desc">Isometric mesh previews and bounding-box dimensions for the enclosure body and two lid designs.</desc>
  <style>
    .heading {{ font: 700 28px system-ui, sans-serif; fill: #0f172a; }}
    .subheading {{ font: 15px system-ui, sans-serif; fill: #475569; }}
    .panel-title {{ font: 700 18px system-ui, sans-serif; fill: #7f1d1d; }}
    .dimension {{ font: 13px ui-monospace, monospace; fill: #475569; }}
  </style>
  <rect width="1220" height="610" fill="#f8fafc"/>
  <text x="24" y="42" class="heading">SniffKitty 3D-Printed Enclosure</text>
  <text x="24" y="70" class="subheading">Portfolio preview generated directly from the source 3MF meshes · dimensions in millimeters</text>
  {''.join(panels)}
</svg>"""
    output.write_text(svg, encoding="utf-8")


if __name__ == "__main__":
    main()
