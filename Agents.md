# **Agents.md**

## **Purpose**

This file defines the project context, constraints, and collaboration rules for OpenAI agents and human contributors working on **XenoARM JIT** – a high‑performance dynamic recompiler that translates Original Xbox x86 instructions to native AArch64 at runtime. It complements the public **README** and other docs.

## **Project Goals**

* **Functional accuracy**: Bit‑exact emulation of the Intel Pentium III ISA (including MMX/SSE) as exercised by retail Xbox titles.  
* **Performance**: Sustain ≥ 90 fps for the majority of titles on flagship ARM devices (Apple M‑series, Snapdragon X‑Elite, Dimensity X) using aggressive optimisation while keeping compile pauses \< 2 ms.  
* **Portability**: Build and run on Linux, macOS, Windows and Android with the same codebase.  
* **Maintainability**: Clear layering, self‑documenting code, exhaustive tests, automated formatting, and CI green at all times.

## **Non‑Goals**

* GPU, audio or I/O emulation (handled by the host emulator).  
* 32‑bit ARM support (focus is AArch64).  
* Support for Xbox homebrew toolchain quirks outside retail binaries.

## **High‑Level Architecture**

flowchart TD  
    A\[x86 Guest Code\] \--\>|Binary Translation| B\[IR Builder\]  
    B \--\> C\[Optimiser\]  
    C \--\> D\[ARM64 CodeGen\]  
    D \--\> E\[TC – Translation Cache\]  
    E \--\> F\[CPU Exec Loop\]  
    F \--\>|Self‑Modifying Code| B

* See `docs/DEVELOPMENT_PLAN.md` for phased milestones.

## **Agent Roles & Responsibilities**

| Role | Scope | Key Outputs |
| ----- | ----- | ----- |
| **Architect** | Overall design, IR spec, performance strategy | ADRs, architecture diagrams |
| **Compiler Engineer** | Translation, optimisation passes, register allocator | `src/backend/` code, unit tests |
| **Runtime Engineer** | SMC detection, memory model fencing, host interfaces | `src/runtime/`, benchmark harness |
| **QA Engineer** | Test matrices, game‑level regressions, performance tracking | GoogleTest suites, perf dashboards |
| **Doc Writer** | API & dev docs, tutorials | Doxygen comments, `docs/` updates |

**Tip for AI code agents**: default to `clang-format -style=file` before committing; break tasks into PR‑sized chunks (\<400 LOC) and always accompany code with tests & docs.

## **Development Workflow**

1. **Branching** – short‑lived feature branches off `main`; name pattern `feat/xyz`, `fix/abc`.  
2. **Commits** – Conventional Commits (`feat:`, `fix:`, `perf:`, `docs:`…). Squash‑merge unless release branches.  
3. **PR Checklist**  
   * CI ✓ (build \+ tests \+ clang‑tidy).  
   * New/changed code covered by tests (≥ 80 % line).  
   * Benchmarks show ≥ 0 % regression.  
   * Docs updated.  
4. **Issue Tags** – `bug`, `good‑first`, `help‑wanted`, `perf`, `task/high‑prio`.

## **Coding Guidelines**

| Topic | Rule |
| ----- | ----- |
| **Language** | Modern C++17, STL only; avoid exceptions in hot paths. |
| **Formatting** | Enforced via `.clang-format`. |
| **Files** | One class per header; keep headers include‑guarded & IWYU clean. |
| **Errors** | Prefer `gsl::span`, `std::expected` for safe APIs; fail‑fast with `assert` in Debug. |
| **SIMD** | Use intrinsics (`arm_neon.h`) rather than inline asm. |
| **IR** | Keep IR nodes immutable after creation. |

## **Build & Tooling**

\# Typical dev build  
cmake \-B build \-DCMAKE\_BUILD\_TYPE=RelWithDebInfo  
cmake \--build build \-j$(nproc)

* Compilers: GCC 11+, Clang 14+, MSVC 17.7+.  
* Recommended IDEs: CLion, VS Code \+ CMake Tools.

## **Testing Strategy**

* **Unit** – GoogleTest in `tests/` (compile‑time IR checks, codegen golden files).  
* **Integration** – Mini game traces replayed head‑less; compare CPU state.  
* **Perf** – Google Benchmark; track regressions in `bench/`.  
* **Fuzz** – LLVM libFuzzer pass feeding random x86 byte streams through pipeline.

## **Continuous Integration**

GitHub Actions (`.github/workflows/ci.yml`):

* matrix: {ubuntu‑latest, macos, windows‑latest, android‑ndk‑r26}.  
* Steps: build → test → benchmark summary → artefact upload.

## **Release & Versioning**

* **Version scheme**: SemVer MAJOR.MINOR.PATCH.  
* Release branch `release/v*`; tag and draft notes in `docs/RELEASE_NOTES_*`.  
* Automated changelog via *semantic‑release* GitHub Action.

## **Documentation**

* Source‑annotated with Doxygen.  
* Higher‑level docs in Markdown under `docs/`.  
* All public APIs require doc blocks.

## **Security & Compliance**

* All third‑party code under MIT/BSD/Apache‑2.0 compatible licences.  
* Changes touching memory safety or privilege boundaries require security review.  
* Do not merge proprietary Xbox BIOS or game code – only clean‑room reverse‑engineering is allowed.

## **OpenAI Agent Prompt Tips**

When adding or modifying code:  
1\. Identify the target module and linked tests.  
2\. Plan the smallest coherent change set.  
3\. Generate code following the coding guidelines.  
4\. Produce or update tests & docs.  
5\. Output a git patch or new files in the expected layout.

## **Glossary**

* **IR** – Intermediate Representation.  
* **SMC** – Self‑Modifying Code.  
* **TC** – Translation Cache.

---

