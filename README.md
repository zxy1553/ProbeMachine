# ProbeMachine

ProbeMachine is a C++ prototype tool for Kripke-structure construction,
probe-library generation, and model-checking style demonstrations. The original
core remains command-line C++ code. The `demo/` directory adds a lightweight
browser launcher for thesis recording and reproducible tool presentation.

## What is included

- `case.cpp` - voting-case probe/model-checking example. It accepts optional
  command-line parameters: `<candidates> <voters> <agents>`.
- `kripke to json.cpp` - exports a sample Kripke structure to `kripke.json`.
- `json to kripke.cpp` - reads `kripke.json` and prints the recovered structure.
- `json to dot.cpp` - converts `kripke.json` to `graph.dot`.
- `computing platform.cpp` - computes probe aggregations over a Kripke input.
- `demo_server.py` and `demo/` - browser-based demo wrapper.

## Browser demo

The demo uses only Python's standard library plus a local C++ compiler. It does
not require Django or a JavaScript build system.

```powershell
cd F:\UGit\ProbeMachine
python demo_server.py
```

Open the printed local URL, usually:

```text
http://127.0.0.1:8765
```

The page lets you set state/candidate/voter/agent parameters, then:

1. generates a demo Kripke JSON artifact,
2. generates DOT and SVG graph artifacts,
3. compiles and runs `case.cpp`,
4. displays the C++ log, result, path, and runtime.

Generated files are written under `demo/outputs/` and are intentionally ignored
by git.

The demo theme can use the thesis/PPT background images. For a portable source
submission, place these files under `demo/static/backgrounds/`:

- `bg-cover.png`
- `bg-content.png`
- `bg-section.png.png`

During local recording, the server also falls back to the same filenames in
`C:\Users\ziyadyao\Downloads\`.

## Command-line example

```powershell
g++ -std=c++17 -static -static-libgcc -static-libstdc++ "case.cpp" -o "case_demo.exe"
.\case_demo.exe 2 1 1
```

If `nlohmann/json.hpp` is not installed, `case.cpp` still compiles because its
JSON helper is optional for this standalone case demonstration.

## Tests

```powershell
python -m unittest demo.tests.test_runner
```
