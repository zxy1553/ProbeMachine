from __future__ import annotations

import json
import math
import re
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
OUTPUT_DIR = ROOT / "demo" / "outputs"
BIN_DIR = ROOT / "demo" / "bin"


@dataclass(frozen=True)
class DemoParams:
    state_count: int = 5
    candidate_count: int = 2
    voter_count: int = 1
    agent_count: int = 1


def clamp(value: int, minimum: int, maximum: int) -> int:
    return max(minimum, min(maximum, value))


def build_demo_kripke(state_count: int, candidate_count: int, voter_count: int) -> dict[str, Any]:
    state_count = clamp(state_count, 3, 12)
    candidate_count = clamp(candidate_count, 2, 6)
    voter_count = clamp(voter_count, 1, 6)

    states = [f"s{i}" for i in range(state_count)]
    transitions = [
        {"from": states[i], "to": states[(i + 1) % state_count]}
        for i in range(state_count)
    ]
    if state_count >= 5:
        transitions.extend(
            [
                {"from": "s0", "to": "s2"},
                {"from": "s1", "to": "s3"},
                {"from": "s2", "to": "s4"},
            ]
        )

    prop_labels: dict[str, dict[str, bool]] = {}
    for index, state in enumerate(states):
        candidate = (index % candidate_count) + 1
        voter = (index % voter_count) + 1
        prop_labels[state] = {
            "reachable": True,
            f"candidate_{candidate}": True,
            f"voter_{voter}": True,
        }
        if index % 2 == 0:
            prop_labels[state]["probe"] = True

    return {
        "states": states,
        "transitions": transitions,
        "propLabels": prop_labels,
        "initialStates": ["s0"],
    }


def kripke_to_dot(kripke: dict[str, Any]) -> str:
    lines = [
        "digraph ProbeMachine {",
        "  rankdir=LR;",
        '  graph [bgcolor="transparent", pad="0.4"];',
        '  node [shape=circle, style="filled", fillcolor="#f5ead7", color="#c9a962", fontcolor="#1f1b14", fontname="Consolas"];',
        '  edge [color="#8f7541", arrowsize=0.8];',
    ]
    initial_states = set(kripke["initialStates"])
    for state in kripke["states"]:
        labels = sorted(kripke["propLabels"].get(state, {}).keys())
        shape = "doublecircle" if state in initial_states else "circle"
        label = f"{state}\\n" + "\\n".join(labels)
        lines.append(f'  "{state}" [shape={shape}, label="{label}"];')
    for transition in kripke["transitions"]:
        lines.append(f'  "{transition["from"]}" -> "{transition["to"]}";')
    lines.append("}")
    return "\n".join(lines) + "\n"


def kripke_to_svg(kripke: dict[str, Any]) -> str:
    width, height = 920, 560
    center_x, center_y = width / 2, height / 2
    radius = 190
    states = kripke["states"]
    positions: dict[str, tuple[float, float]] = {}

    for index, state in enumerate(states):
        angle = (2 * math.pi * index / len(states)) - math.pi / 2
        positions[state] = (
            center_x + radius * math.cos(angle),
            center_y + radius * math.sin(angle),
        )

    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" role="img" aria-label="Kripke structure graph">',
        "<defs>",
        '<marker id="arrow" markerWidth="10" markerHeight="10" refX="8" refY="3" orient="auto" markerUnits="strokeWidth"><path d="M0,0 L0,6 L9,3 z" fill="#8f7541"/></marker>',
        '<radialGradient id="nodeFill" cx="35%" cy="30%"><stop offset="0%" stop-color="#fff8ea"/><stop offset="100%" stop-color="#e8d7b8"/></radialGradient>',
        "</defs>",
        '<rect width="920" height="560" rx="28" fill="#f4ecd9"/>',
        '<path d="M0 416 C160 356 290 452 470 390 C632 334 708 392 920 300 L920 560 L0 560 Z" fill="#dfd0b4" opacity="0.52"/>',
        '<g stroke="#8f7541" stroke-width="2.4" marker-end="url(#arrow)" opacity="0.78">',
    ]

    for transition in kripke["transitions"]:
        x1, y1 = positions[transition["from"]]
        x2, y2 = positions[transition["to"]]
        dx, dy = x2 - x1, y2 - y1
        length = math.hypot(dx, dy) or 1
        start_x, start_y = x1 + dx / length * 44, y1 + dy / length * 44
        end_x, end_y = x2 - dx / length * 50, y2 - dy / length * 50
        parts.append(f'<line x1="{start_x:.1f}" y1="{start_y:.1f}" x2="{end_x:.1f}" y2="{end_y:.1f}"/>')

    parts.append("</g>")
    initial_states = set(kripke["initialStates"])
    for state in states:
        x, y = positions[state]
        labels = sorted(kripke["propLabels"].get(state, {}).keys())
        ring_width = 5 if state in initial_states else 2
        parts.append(f'<g class="node"><circle cx="{x:.1f}" cy="{y:.1f}" r="48" fill="url(#nodeFill)" stroke="#c9a962" stroke-width="{ring_width}"/>')
        if state in initial_states:
            parts.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="56" fill="none" stroke="#c94c4c" stroke-width="2" opacity="0.72"/>')
        parts.append(f'<text x="{x:.1f}" y="{y - 6:.1f}" text-anchor="middle" fill="#1f1b14" font-family="Consolas, monospace" font-size="20" font-weight="700">{state}</text>')
        parts.append(f'<text x="{x:.1f}" y="{y + 18:.1f}" text-anchor="middle" fill="#6c5b38" font-family="Consolas, monospace" font-size="11">{", ".join(labels[:3])}</text></g>')

    parts.append("</svg>")
    return "\n".join(parts)


def parse_case_output(output: str) -> dict[str, Any]:
    normalized_output = output.replace("ω ", "omega")
    result_match = re.search(r"\*+\s*Result\s*\*+\s*\r?\n\s*(True|False)", normalized_output)
    path_match = re.search(r"Path is:\s*([^\r\n]+)", normalized_output)
    time_match = re.search(r"Time taken:\s*([0-9.]+)\s*seconds", normalized_output)
    data_library = _parse_triplets(
        _extract_section(normalized_output, "Data Library X", "Probe Library Y")
    )
    probe_library = _parse_probes(
        _extract_section(normalized_output, "Probe Library Y", "Step1")
    )

    return {
        "result": result_match.group(1) if result_match else "Unknown",
        "path": path_match.group(1).strip() if path_match else "",
        "seconds": float(time_match.group(1)) if time_match else None,
        "parameters": _parse_parameters(normalized_output),
        "dataEntryCount": len(data_library),
        "dataLibrary": data_library,
        "probeLibrary": probe_library,
        "steps": _parse_steps(normalized_output),
    }


def _extract_section(output: str, start_title: str, end_title: str) -> str:
    pattern = rf"\*+\s*{re.escape(start_title)}\s*\*+(.*?)(?=\*+\s*{re.escape(end_title)}\s*\*+)"
    match = re.search(pattern, output, re.DOTALL)
    return match.group(1) if match else ""


def _parse_triplets(text: str) -> list[dict[str, str]]:
    items = []
    for source, predicate, target in re.findall(r"([^\s<>-]+)-([^\t\r\n<>-]+)-([^\s<>-]+)", text):
        items.append(
            {
                "source": source.strip(),
                "predicate": predicate.strip(),
                "target": target.strip(),
            }
        )
    return items


def _parse_probes(text: str) -> list[dict[str, str]]:
    return [
        {"from": source.strip(), "to": target.strip()}
        for source, target in re.findall(r"<([^,\s]+),([^>\s]+)>", text)
    ]


def _parse_steps(output: str) -> list[dict[str, Any]]:
    steps = []
    step_matches = list(re.finditer(r"\*+\s*Step(\d+)\s*\*+", output))
    for index, match in enumerate(step_matches):
        start = match.end()
        end = step_matches[index + 1].start() if index + 1 < len(step_matches) else output.find("********************************", start)
        if end == -1:
            end = len(output)
        steps.append(
            {
                "name": f"Step {match.group(1)}",
                "items": _parse_triplets(output[start:end]),
            }
        )
    return steps


def _parse_parameters(output: str) -> dict[str, int | None]:
    parameters: dict[str, int | None] = {"voters": None, "candidates": None, "agents": None}
    for key in parameters:
        match = re.search(rf"{key}:\s*([0-9]+)", output)
        if match:
            parameters[key] = int(match.group(1))
    return parameters


def write_demo_artifacts(params: DemoParams) -> dict[str, str]:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    kripke = build_demo_kripke(params.state_count, params.candidate_count, params.voter_count)
    artifacts = {
        "kripke.json": json.dumps(kripke, indent=2),
        "graph.dot": kripke_to_dot(kripke),
        "graph.svg": kripke_to_svg(kripke),
    }
    for name, content in artifacts.items():
        (OUTPUT_DIR / name).write_text(content, encoding="utf-8")
    return artifacts


def compile_case_binary() -> tuple[Path | None, str | None]:
    compiler = shutil.which("g++")
    if not compiler:
        return None, "g++ was not found on PATH; generated artifacts are still available."

    BIN_DIR.mkdir(parents=True, exist_ok=True)
    binary = BIN_DIR / ("case_demo.exe" if _is_windows() else "case_demo")
    command = [
        compiler,
        "-std=c++17",
        "-static",
        "-static-libgcc",
        "-static-libstdc++",
        str(ROOT / "case.cpp"),
        "-o",
        str(binary),
    ]
    completed = subprocess.run(command, cwd=ROOT, text=True, capture_output=True, timeout=30)
    if completed.returncode != 0:
        return None, completed.stderr or completed.stdout or "Failed to compile case.cpp."
    return binary, None


def run_case(params: DemoParams) -> dict[str, Any]:
    binary, error = compile_case_binary()
    if error:
        return {"ok": False, "error": error, "stdout": "", "summary": {}}

    assert binary is not None
    command = [
        str(binary),
        str(params.candidate_count),
        str(params.voter_count),
        str(params.agent_count),
    ]
    completed = subprocess.run(command, cwd=ROOT, text=True, capture_output=True, timeout=30)
    stdout = completed.stdout
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    (OUTPUT_DIR / "case.log").write_text(stdout + completed.stderr, encoding="utf-8")
    return {
        "ok": completed.returncode == 0,
        "error": completed.stderr if completed.returncode else "",
        "stdout": stdout,
        "summary": parse_case_output(stdout),
    }


def run_demo(params: DemoParams) -> dict[str, Any]:
    artifacts = write_demo_artifacts(params)
    case_result = run_case(params)
    return {
        "params": params.__dict__,
        "artifacts": artifacts,
        "case": case_result,
        "outputDir": str(OUTPUT_DIR),
    }


def _is_windows() -> bool:
    return "\\" in str(ROOT)
