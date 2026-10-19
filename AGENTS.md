# AGENTS.md

> **Audience:** maintainers, contributors, reviewers, and AI coding agents.
> **Scope:** the entire repository.
> **Status:** pre-alpha, foundation phase (October 2026).
> **Read this first.** Subdirectories may add a local `AGENTS.md` with extra rules. Local rules MUST NOT contradict this file.

## 0. How to use this document

This file is the shared engineering contract for Morpho. It explains what we are building, how the code is organized, which decisions are already settled, and how changes are made.

- **MUST** means required. A change that violates a MUST is rejected unless this document is updated first.
- **SHOULD** means expected. Deviate only with a written reason in the pull request or an ADR.
- **MAY** means optional.
- When this document and a code comment disagree, this document wins. Fix the comment.
- When this document and a local `AGENTS.md` disagree, stop and ask a maintainer.
- The repository is at milestone **M0**. Some paths and scripts referenced here are planned and do not exist yet. Create them as part of the relevant milestone, not all at once.

All repository artifacts are written in **US English**: code, identifiers, comments, commit messages, documentation, issue templates, and source UI strings. Conversations with users may happen in any language; the repository stays English.

## Contents

- [0. How to use this document](#0-how-to-use-this-document)
- [1. Project](#1-project)
- [2. Foundational decisions](#2-foundational-decisions)
- [3. Architecture](#3-architecture)
- [4. Languages and code standards](#4-languages-and-code-standards)
- [5. Performance and memory](#5-performance-and-memory)
- [6. Security, privacy, and offline guarantees](#6-security-privacy-and-offline-guarantees)
- [7. Licensing and dependencies](#7-licensing-and-dependencies)
- [8. Repository layout](#8-repository-layout)
- [9. Build, test, and run](#9-build-test-and-run)
- [10. Testing](#10-testing)
- [11. Git workflow](#11-git-workflow)
- [12. Documentation](#12-documentation)
- [13. UX, accessibility, and localization](#13-ux-accessibility-and-localization)
- [14. Roadmap](#14-roadmap)
- [15. Working agreement for AI agents](#15-working-agreement-for-ai-agents)
- [16. Changing this document](#16-changing-this-document)
- [Appendix A: Glossary](#appendix-a-glossary)

## 1. Project

### 1.1 What Morpho is

Morpho is a free, offline, local-first file converter for the desktop. It converts images, audio, video, documents, and archives without uploading files, creating accounts, or sending telemetry. It aims to match the convenience of online converters while being faster, more private, and dramatically lighter.

**Tagline:** Convert anything. Locally.

### 1.2 Product principles

1. **Offline by default.** Files never leave the machine. The release build contains no network client and performs no update checks, analytics, or crash uploads.
2. **Free and open.** Original code is MIT-licensed. There are no paid tiers, ads, trials, or feature locks.
3. **Fast and lean.** C++20 and Qt 6 Widgets, streaming conversion, bounded memory, lazy initialization, and measurable performance budgets. No Electron, no web runtime, no scripting engine.
4. **Private.** No accounts, no telemetry, no cloud, no file inspection beyond what the requested conversion needs.
5. **Safe with hostile input.** Every input file is untrusted. Parsers are bounded, fuzzed, and isolated behind narrow interfaces.
6. **Native and accessible.** Native widgets, keyboard operation, screen-reader support, system theme, HiDPI, and localization-ready strings.
7. **Maintainable.** Small modules, explicit ownership, automated tests, reproducible builds, and documented decisions.
8. **Boring where possible.** Prefer the standard library and well-understood libraries over clever infrastructure.

### 1.3 Scope

- Desktop GUI with drag-and-drop, batch queue, per-file progress, cancellation, retry, and output management.
- Offline conversion between supported formats in the image, audio, video, document, and archive categories.
- Sensible per-format options (quality, bitrate, sample rate, dimensions, metadata policy, and similar).
- A headless CLI used by integration tests and automation; shipping it is expected by 1.0.
- Linux first (X11 and Wayland), with a portable core that can support Windows and macOS later.
- AppImage as the primary distribution format, Flatpak as the sandboxed option, and optionally a `.deb`.

### 1.4 Non-goals

- An online service, URL downloading, cloud storage, accounts, sync, telemetry, ads, or paid plans.
- A media editor, image editor, office suite, OCR tool, or AI upscaler.
- Running external converter binaries (`ffmpeg`, ImageMagick, LibreOffice, Pandoc, and similar) at runtime.
- Office document conversion in 1.0. It is only in scope if a permissively licensed, embeddable engine is approved through an ADR. Bundling a headless LibreOffice is explicitly prohibited.
- A plugin marketplace or dynamically loaded third-party plugins in 1.x.
- Mobile, web, or server editions.
- DRM circumvention or conversion of protected content.
- Automatic updates, background network services, or crash reporting that leaves the machine.

### 1.5 Current status

M0 is the foundation phase. The repository currently contains this guide, a README, and the project logo; the application source has not been scaffolded. The first milestone establishes the build, CI, core skeleton, Qt window, CLI skeleton, tests, and packaging skeleton. Do not assume planned files exist; inspect the tree before relying on them.

## 2. Foundational decisions

| Area | Decision | Rationale / constraints |
| --- | --- | --- |
| Application language | **C++20** by default; **C17** only for stable C ABIs, C-only dependencies, or a measured low-level module | RAII, value semantics, zero-cost abstractions, and a mature ecosystem. C stays available where it genuinely helps. |
| GUI toolkit | **Qt 6 Widgets** in C++; no QML | Native widgets, accessibility, mature file dialogs, and lower memory than QML, Electron, or Qt WebEngine. |
| Build system | **CMake 3.28+**, Ninja, CMake Presets, CTest | Ubiquitous, reproducible, IDE-friendly, and easy to script in CI. |
| Platforms | **Linux x86_64 first**, X11 and Wayland; portable core; Windows/macOS later | Focus delivery. Platform code stays behind interfaces so ports do not rewrite the core. |
| Runtime backends | **In-process bundled libraries**; no external converter executables | Offline guarantee, reproducible behavior, fewer security and packaging surprises. |
| Packaging | **AppImage primary**, Flatpak next, `.deb` optional | Self-contained delivery plus a sandboxed option. |
| License | **MIT** for original code; LGPL dependencies dynamically linked | Permissive project license with strict third-party compliance. |
| Versioning | **SemVer 2.0.0**; `0.y.z` before 1.0 | Predictable releases and changelog automation. |
| Git workflow | **GitHub Flow**: short-lived branches from `main`, pull requests, squash merge; Conventional Commits | Clean history, small reviews, and a `main` branch that is always releasable. |
| Testing | **Catch2 v3 + CTest**; Qt Test for UI; libFuzzer for parsers | Fast, local, deterministic, and network-free. |
| Documentation | **US English**; ADRs for significant decisions | Global collaboration and durable context. |
| Toolchain baseline | GCC 13+ or Clang 18+, CMake 3.28+, Qt 6.4+, FFmpeg 6.1+ | Ubuntu 24.04 LTS is the reference development environment. |

Qt Widgets plus C++20 is the pragmatic choice for a low-memory, native, cross-platform desktop application. It avoids the render process, GPU overhead, and attack surface of web-based stacks. C remains a first-class option for stable ABI boundaries and low-level adapters, but C++ is the default because ownership errors in large C codebases are expensive.

## 3. Architecture

### 3.1 Dependency rule

Dependencies point inward. Higher layers know about lower layers; lower layers never include higher-layer headers.

```text
                    +-----------------------------------+
                    | app: main(), wiring, lifecycle    |
                    +----------------+------------------+
                                     |
                    +----------------+------------------+
                    |                                   |
              +-----v------+                     +------v-----+
              | ui (Qt)    |                     | cli        |
              +-----+------+                     +------+-----+
                    |                                   |
                    +----------------+------------------+
                                     |
                            +--------v---------+
                            | backends         |  image / audio / video /
                            |                  |  document / archive
                            +--------+---------+
                                     |
                            +--------v---------+
                            | core             |  Qt-free domain, engine,
                            |                  |  scheduler, I/O, errors
                            +--------+---------+
                                     |
                            +--------v---------+
                            | platform         |  OS services behind
                            |                  |  narrow interfaces
                            +------------------+
```

- `core` MUST NOT include Qt, UI, CLI, or backend headers. It may use the C++ standard library and approved small utilities.
- `backends` MAY use `core`, their third-party codec libraries, and approved wrappers. They MUST NOT use Qt or UI code.
- `ui` MAY use `core` and Qt. It MUST NOT include backend implementation headers; it talks to the engine through service interfaces.
- `cli` MAY use `core` and the engine. It MUST NOT depend on Qt.
- `app` is the composition root. It wires concrete backends, platform services, UI, and CLI entry points.
- `platform` implements OS-specific services behind `core` interfaces. Platform headers do not leak into core or UI.

Public headers under `include/morpho/` expose only standard-library types and Morpho types. Third-party types (`AVFrame`, `QImage`, `png_structp`, and similar) MUST NOT appear there.

### 3.2 Modules

| Module | Responsibility |
| --- | --- |
| `core/domain` | Format descriptors, conversion requests, jobs, errors, limits, capability registry, options schema. |
| `core/engine` | Job scheduler, worker pool, cancellation, progress, conversion pipeline, output finalization, verification. |
| `core/io` | Path handling, magic-byte detection, atomic output files, temporary workspaces, disk-space checks. |
| `backends/image` | PNG, JPEG, WebP, AVIF, TIFF, BMP, GIF, and future image formats. |
| `backends/audio` | MP3, AAC/M4A, Opus, Vorbis, FLAC, WAV. |
| `backends/video` | MP4, WebM, MKV, MOV, animated GIF. |
| `backends/document` | Image/text to PDF; PDF input only after an approved ADR. |
| `backends/archive` | ZIP, TAR, 7Z inspection and extraction with strict safety limits. |
| `platform` | XDG paths, temporary storage, desktop integration, and Linux-specific services. |
| `ui` | Main window, queue model, format controls, settings, progress, translations, theme integration. |
| `cli` | Headless conversion, format listing, machine-readable output, stable exit codes. |
| `app` | Process entry point, dependency wiring, logging initialization, crash handling, shutdown. |

### 3.3 Conversion pipeline

Every conversion follows the same lifecycle, regardless of backend:

```text
Queued -> Probing -> Converting -> Verifying -> Published
   |         |            |            |
   +---------+------------+------------+--> Failed / Canceled
```

1. **Enqueue:** validate the request, reserve an output name, and create a private temporary workspace in the destination filesystem.
2. **Probe:** detect the real format from magic bytes, inspect metadata, and enforce input limits before large allocations.
3. **Plan:** select a backend and options, reject unsupported pairs, and estimate disk requirements.
4. **Convert:** stream the input to a temporary output file. Report progress and check the cancellation token between chunks or frames.
5. **Verify:** reopen the output, validate the container or codec header, dimensions or duration, and non-zero payload. Full decoding is reserved for tests and explicit debug modes.
6. **Publish:** flush the file, atomically rename it to the final name, and remove the workspace. Existing files are never overwritten unless the user explicitly selected overwrite.
7. **Report:** return the output path, elapsed time, warnings, and technical metadata to the caller.

On failure or cancellation, delete the temporary output, preserve any pre-existing destination file, and return a structured error. A partially written file MUST never be visible under the final name.

### 3.4 Jobs, threading, and cancellation

- The UI thread MUST NOT perform file I/O, probing, encoding, decoding, or verification.
- One scheduler owns a bounded worker pool. Do not create one thread per job. Do not detach threads.
- Default parallelism is `clamp(hardware_concurrency() / 2, 1, 4)` concurrent jobs. The user may lower it to 1; raising the cap requires a measurable benefit.
- The scheduler passes each active job a thread budget. The sum of backend threads MUST NOT exceed `hardware_concurrency()`; backends MUST honor the budget to avoid oversubscription.
- Use `std::jthread` and `std::stop_token`. Cancellation is cooperative and checked at least every chunk or frame. Target cancel latency is 250 ms.
- Progress is staged (`Probe`, `Convert`, `Verify`) with weights and an overall value in `[0, 1]`. Throttle UI updates to at most 10 Hz per job.
- The job queue is bounded. The default maximum is 1,000 pending items; adding more returns a typed error rather than consuming unbounded memory.
- Shutdown cancels active jobs, joins workers, and removes temporary workspaces. No library code calls `std::exit()` or `std::terminate()`.
- Shared state is minimized. Prefer message passing and immutable snapshots over locks. UI state is updated only on the UI thread.

### 3.5 Errors

`core` uses a lightweight `Result<T>` type built on `std::variant<T, Error>` or an equivalent C++20 design. `std::expected` is C++23 and is not available. Exceptions are not used for expected failures.

| Code | Meaning | Typical user action |
| --- | --- | --- |
| `InvalidRequest` | Missing, contradictory, or malformed options. | Fix the selection or options. |
| `UnsupportedFormat` | Input or output format is not supported, or the pair is impossible. | Choose a supported target. |
| `CorruptInput` | The input is truncated, malformed, or fails validation. | Try another file or re-export the source. |
| `PermissionDenied` | The process cannot read the input or write the output. | Choose a writable folder. |
| `NoSpace` | The destination filesystem lacks space for the estimated output. | Free space or choose another folder. |
| `ResourceLimit` | A limit was exceeded (pixels, duration, entries, memory). | Reduce the input or raise a documented limit. |
| `Canceled` | The user or scheduler canceled the job. | None. |
| `DependencyFailure` | A codec or library failed in an unexpected way. | Retry; report with logs if it persists. |
| `Internal` | A bug or invariant violation. | Report with the error code and logs. |

- Error messages in `core` are English technical messages for logs and developers. The UI maps stable error codes to localized, actionable copy.
- Errors carry a stable code, a short context string, and an optional cause. Do not expose raw third-party messages as the primary user-facing text.
- Catch exceptions only at third-party boundaries, convert them to `Error`, and log the technical detail. Never swallow an exception silently.
- `std::bad_alloc` becomes `ResourceLimit` at the boundary. Do not attempt heroic recovery from allocation failure.
- Every failure path must clean up temporary files and leave user data untouched.

### 3.6 Files, paths, and atomic output

- Internal paths use `std::filesystem::path`. On Linux, paths are byte sequences; never assume UTF-8. The UI converts with `QFile::encodeName` and `QFile::decodeName` and displays escaped names when needed.
- Never normalize, case-fold, or truncate a user path silently. Preserve Unicode code points and filename bytes.
- Temporary output lives in a private directory inside the destination filesystem (`.morpho-tmp/<uuid>/`) so the final rename is atomic. Workspace files are created with mode `0600`; final files use `0666 & ~umask`.
- Clean the workspace on success, failure, and cancellation. At startup, remove only stale Morpho workspaces older than seven days that match the exact internal pattern. Never delete arbitrary files.
- Publish with a same-filesystem atomic rename. Flush the file before renaming. If the destination name exists, apply the selected policy: `Rename` (default, append ` (1)`, ` (2)`, and so on), `Overwrite` (explicit), or `Fail`.
- The default output directory is the source file's directory for a single-file job, and the XDG downloads directory under a `Morpho` subdirectory for a batch. If the default is not writable, fall back to the downloads directory and make the fallback visible. Always show the output directory before conversion.
- Preserve metadata by default when the target format supports it, and provide an explicit “Remove metadata” option. Never invent metadata values. Document per-format behavior and warn when location metadata is present.
- Do not follow output symlinks. Writing a temporary file and renaming over a symlink replaces the symlink, which is acceptable; never write through an untrusted symlink target.
- Check free space before converting. Require at least `max(input_size, 64 MiB) + 10%`, fail early with `NoSpace`, and keep the check cheap.
- If a filename component exceeds 255 bytes, truncate the base name and append a short stable hash while preserving the extension. Return a warning.
- Do not write outside the chosen output directory, the Morpho cache/config directories, or the system temporary directory. Backends receive an explicit workspace and output path.

### 3.7 Format registry and capability matrix

- Format and capability data live in one place in code: IDs, category, extensions, MIME types, read/write support, options schema, backend binding, and limits.
- `morpho-cli formats --json` is the machine-readable source for tests and tooling. `morpho-cli formats --markdown` generates `docs/formats.md`; CI verifies that the checked-in document is current. Do not hand-edit the generated table.
- Extensions are hints, never proof. Detect with magic bytes and backend probing. On mismatch, use the detected format and emit a warning; never trust an extension for security decisions.
- A format pair is supported only when a backend declares it and integration tests cover it. The UI MUST NOT offer a pair that the engine cannot perform.
- Supported format pairs are a user-facing compatibility contract after 1.0. Removing one requires a deprecation note and a major-version change.

### 3.8 Backend contract

Each backend implements a narrow converter interface. The exact API lives in `include/morpho/`; the following sketch is illustrative:

```cpp
namespace morpho {

struct ConversionRequest;
struct ConversionReport;
class JobContext;

class Converter {
public:
    virtual ~Converter() = default;
    virtual Result<SourceInfo> probe(const std::filesystem::path& path) const = 0;
    virtual Result<ConversionReport> convert(const ConversionRequest& request,
                                             JobContext& context) = 0;
};

}  // namespace morpho
```

- A backend is stateless or owns only per-job state. It MUST be safe to run separate instances concurrently.
- A backend MUST NOT use Qt, open network sockets, spawn processes, load plugins, read environment-controlled file paths, or write outside the provided workspace/output paths.
- Use RAII wrappers for codec resources. No manual `free`/`close` in business logic. No third-party types in public headers.
- Check cancellation regularly, report honest progress, and keep the worst-case cancel latency under 250 ms.
- Enforce limits before allocating: image pixels, video duration, audio channels, archive entries, decompressed size, and metadata size.
- Every backend needs tests for success, corrupt input, unsupported options, cancellation, and limit enforcement. Fuzz the probe and metadata paths.

### 3.9 UI boundary

- The UI observes the engine through a controller/service facade. Widgets do not call backends or the filesystem directly.
- Qt types stay in `ui`. Convert to core types at the boundary; do not leak `QString` or `QImage` into core APIs.
- Use model/view classes (`QAbstractTableModel`) for the queue. Do not build the queue with `QTableWidget`.
- All user-visible strings use `tr()`. Do not concatenate translated fragments; use placeholders and plural forms.
- No blocking waits, `QEventLoop` tricks, or `QThread::wait()` on the UI thread. Use queued signals from the engine.
- File dialogs use native Qt dialogs. Opening the output folder uses `QDesktopServices` or a desktop portal, never a network URL.
- `QProcess` is forbidden. Desktop integration is limited to opening the output folder through `QDesktopServices` or a portal; that action MUST NOT be used for conversion or URL fetching.
- The main window uses native decorations. Do not implement a custom frameless window in 1.x.

### 3.10 CLI boundary

- `morpho-cli` is a headless frontend over the same core and engine. It exists for automation, integration tests, and headless systems.
- Human output goes to stdout; logs and errors go to stderr. `--json` output is stable and documented once the command surface is frozen.
- Exit codes: `0` success, `1` conversion failed, `2` usage error, `3` unsupported format, `4` canceled, `5` internal error. Document any addition before shipping it.
- The CLI MUST NOT initialize Qt or require a display server.

## 4. Languages and code standards

### 4.1 C++20

C++ is the default language for the application, core, backends, and UI.

- Use RAII for every resource. Owning raw pointers and raw `new`/`delete` are forbidden.
- Use `std::unique_ptr` for exclusive ownership. Use `std::shared_ptr` only for genuinely shared lifetimes, with a comment explaining why. Prefer references and values.
- Pass by value for sinks, `const&` for reads, and `std::span`/`std::string_view` for non-owning views.
- Mark error-returning functions `[[nodiscard]]`. Mark destructors and moves `noexcept` where true.
- Use `enum class`, `constexpr`, `override`, and `final` deliberately.
- Avoid C-style casts. Use `static_cast`; confine `reinterpret_cast` to ABI boundaries with a comment.
- Use `std::chrono` for time and `std::filesystem` for paths. Prefer `std::error_code` overloads where failure is expected, not exceptional.
- Use `std::format` for formatting. Do not use `sprintf`, `strcpy`, or manual buffer arithmetic.
- No variable-length arrays, no undefined behavior, no strict-aliasing tricks, no unaligned access.
- No global mutable state or singletons. Depend on injected interfaces; `app` owns lifetimes.
- Keep headers self-contained and include-what-you-use. Do not rely on transitive includes.
- Do not use C++20 modules or coroutines without an ADR; they add toolchain friction.
- Avoid `auto` when the type is not obvious from the right-hand side. Never use `auto` for integer types where width matters.
- Prefer named constants over magic numbers. Use `std::size_t` for memory sizes and `std::int64_t` for file sizes and durations; validate conversions.

### 4.2 C17

C is a first-class option, not a second-class citizen. Use it where a stable C ABI, a C-only dependency, or measured low-level control justifies it.

- Compile as C17 with `-std=c17 -pedantic`. Do not use GNU extensions unless a platform header requires them behind a guard.
- Public C APIs use opaque handles, explicit ownership, and status codes. Do not expose struct layouts or STL types.
- Prefix functions with `morpho_`, types with `Morpho`, enum constants and macros with `MORPHO_`.
- Every allocation has a documented matching free function. No ownership ambiguity.
- Use `<stdint.h>` fixed-width types, `<stdbool.h>`, and `_Static_assert` for layout assumptions.
- No `longjmp`/`setjmp`, no `errno` as a public API, no variable-length arrays, no hidden global state.
- Check every arithmetic operation that can overflow, especially sizes and offsets.
- C headers must be C++-safe via `extern "C"` guards. Do not let exceptions cross a C ABI.

### 4.3 Qt 6 Widgets

- Minimum Qt 6.4; Qt 6.8 LTS is recommended. Do not use APIs newer than the minimum without a version guard.
- Allowed runtime modules: Core, Gui, Widgets, Svg. Test uses Qt Test. Build-time Linguist tools are not linked into the application.
- Forbidden: QML, Qt Quick, Qt WebEngine, Qt Network, Qt Multimedia, Qt Charts, private Qt headers, and static Qt builds.
- Use `QAbstractTableModel` and views for large collections. Keep business logic out of widgets.
- Use `QFileDialog`, `QStandardPaths`, `QSettings`, `QStorageInfo`, and `QDesktopServices` at the UI boundary.
- Use categorized logging (`QLoggingCategory`) for Qt-specific diagnostics. Do not leave unconditional `qDebug()` calls in production paths.
- Support system light/dark themes, 100-200% scaling, SVG icons, and keyboard-only navigation. No hard-coded pixel dimensions or colors.
- Connect signals with a context object. Never use a direct cross-thread connection to touch UI state.
- Use queued signals for worker progress. Do not poll shared state from a timer.
- Use `QImageReader` with a scaled size for thumbnails. Never decode full-resolution images just to draw a list row.

### 4.4 Naming

| Element | Convention | Example |
| --- | --- | --- |
| C++ namespaces | lowercase | `morpho::image` |
| C++ types/classes | PascalCase | `ConversionQueueModel` |
| C++ functions/methods | camelCase | `convertFile()` |
| C++ variables/parameters | camelCase | `outputPath` |
| C++ private members | `m_` + camelCase | `m_jobCount` |
| C++ constants | `k` + PascalCase | `kDefaultQuality` |
| C++ enum values (scoped) | PascalCase | `ErrorCode::CorruptInput` |
| C functions | `morpho_` + snake_case | `morpho_job_cancel()` |
| C types | `Morpho` + PascalCase | `MorphoJob` |
| C enum constants/macros | `MORPHO_` + SCREAMING_SNAKE | `MORPHO_OK` |
| Files | snake_case | `conversion_queue_model.cpp` |
| Test files | `*_test.cpp` | `png_converter_test.cpp` |
| Qt signals | camelCase, past tense where natural | `jobFinished` |

Use `is`, `has`, `can`, and `should` for booleans. Name things for what they mean, not for their type. Avoid abbreviations except well-known ones (`id`, `url`, `dpi`, `rgb`).

### 4.5 Formatting and includes

- Formatting is enforced by `clang-format` with the repository `.clang-format`: LLVM base, 4-space indent, 100-column limit, attached braces, pointer alignment to the right.
- Include order: own header, C standard headers, C++ standard headers, Qt headers, third-party headers, project headers. Separate groups with a blank line.
- Use angle brackets for public/project and third-party includes, quotes for the current module's private headers.
- Use `#pragma once`.
- One statement per line. No tabs. UTF-8, LF, final newline.
- CMake: lowercase commands, 2-space indentation, target-based configuration, no `file(GLOB)`, no directory-wide `include_directories` or `link_libraries`.
- YAML/JSON: 2-space indentation. Markdown: keep lines readable and wrap prose near 100 columns.
- Run `./scripts/format.sh` before committing. CI runs `./scripts/format.sh --check`.
- `clang-tidy` runs in CI on changed C/C++ files. Do not suppress a check without a comment explaining why and narrowing the suppression to the smallest scope.

### 4.6 Comments and documentation

- Comments are English and explain **why**, not **what**. The code explains what.
- Public headers and non-obvious algorithms get `///` documentation comments.
- TODOs must reference an issue: `TODO(#123): handle multi-page TIFF`.
- No commented-out code, journal comments, author tags, or “temporary” hacks without an issue.
- Keep comments truthful. A stale comment is a bug.
- Do not add license headers with personal copyright lines; use the SPDX header described in Section 7.

### 4.7 Error handling and logging

- Expected failures use `Result<T>`; exceptions are reserved for third-party boundaries and truly exceptional conditions.
- Log through the core logging facade. Do not use `printf` or `std::cout` in libraries; the CLI writes to stdout/stderr intentionally.
- Levels: trace, debug, info, warn, error, critical. Default to info in release and debug in development, controlled by `MORPHO_LOG`.
- Log lines include timestamp, level, component, message, and structured key/value context where useful.
- Never log file contents, secrets, tokens, or personal data. Full paths are debug-only; info logs use basenames.
- Logging must be thread-safe and must never throw or block the UI thread.
- Rotate local logs (5 MiB, three files) under the XDG state directory. Provide a “Clear logs” action.
- Do not swallow errors. If recovery is impossible, return `Internal`, log the context, and keep the process alive.

### 4.8 Forbidden practices

- Shelling out to converters, `system()`, `popen()`, `exec*()`, or `QProcess` for conversion or file processing.
- Network clients, URL fetching, telemetry, analytics, ads, or background update checks.
- GPL/AGPL/SSPL runtime dependencies, static LGPL linking, or unlicensed code.
- Global mutable state, hidden singletons, or static initialization order dependencies.
- Blocking the UI thread with I/O, waits, sleeps, or synchronous conversions.
- `sleep()` as synchronization; use condition variables, futures, or events.
- Unbounded queues, buffers, caches, or “read the whole file into memory” shortcuts.
- Trusting file extensions, MIME guesses, or metadata without validation.
- Silent overwrites, silent data loss, or leaving partial files under final names.
- Catching all exceptions without logging and converting them to a typed error.
- `printf`/`std::cout` in core, backend, or UI library code.
- Third-party headers in `include/morpho/` or third-party types in public APIs.
- Large binaries, generated files, secrets, or build artifacts in git.
- Disabling warnings globally or adding `-Wno-*` without a scoped justification.

## 5. Performance and memory

### 5.1 Principles

1. **Measure first.** No performance claim without a benchmark, profile, or reproducible measurement.
2. **Stream.** Never read a whole media file into memory when the format allows incremental processing.
3. **Bound everything.** Queue sizes, buffers, caches, worker counts, archive expansion, and metadata reads have explicit limits.
4. **Initialize lazily.** Do not load codecs, scan plugins, build thumbnails, or parse large configs at startup.
5. **Avoid copies and per-item allocations in hot loops.** Reserve, reuse, and move. Do not sacrifice clarity for unmeasured micro-optimizations.
6. **Batch UI updates.** Coalesce progress and model changes; repaint at most 10 times per second per job.
7. **Keep the UI responsive.** Conversions are background work; the interface stays interactive and cancellable.
8. **Prefer zero-cost abstractions.** Use templates and `constexpr` where they clarify; avoid virtual dispatch in the hottest inner loops if a measured alternative exists.

### 5.2 Initial budgets

These are targets for release builds on the reference machine, not marketing guarantees. Update them with measurements and an ADR if reality demands a different tradeoff.

| Metric | Target | Measurement |
| --- | --- | --- |
| Startup to interactive (warm cache) | <= 800 ms | `bench/startup` |
| Idle RSS, empty queue | <= 90 MiB | `/usr/bin/time -v`, `bench/startup` |
| RSS with 100 queued files and thumbnails | <= 180 MiB | integration benchmark |
| Peak RSS, 24 MP JPEG to WebP | <= 350 MiB | `bench/image` |
| Peak RSS, 1 h 1080p H.264 to VP9 | <= 600 MiB | `bench/video` |
| UI input latency (p95) | <= 50 ms | manual/UI instrumentation |
| Progress update rate | <= 10 Hz per job | code review and instrumentation |
| Cancellation latency | <= 250 ms | integration test |
| AppImage size (compressed) | <= 250 MB | CI artifact |
| Leaks in sanitizer tests | 0 | ASan/LSan |

Reference machine for benchmarks: 4 vCPU, 8 GiB RAM, SSD, Ubuntu 24.04, 1920x1080. CI runners are noisy; compare medians of at least five runs and treat a 10% regression or any budget breach as a merge blocker unless an ADR explains the tradeoff.

### 5.3 Benchmarking and profiling

- Benchmarks live in `benchmarks/` and build in release mode with symbols.
- Record the git SHA, compiler, CPU, memory, kernel, and relevant library versions with every baseline.
- Use `perf stat`, `perf record`, `heaptrack`, `valgrind --tool=massif`, and `strace` for syscall behavior.
- Benchmark the whole pipeline, not only the codec call: probe, conversion, verification, and publish.
- Store baselines in `benchmarks/baseline.json`; CI may report deltas but MUST NOT fail on noisy microbenchmarks alone.
- Add a benchmark for every performance-sensitive feature and every performance-focused PR.

### 5.4 Optimization rules

- Correctness and clarity come first. Optimize only after a profile identifies the bottleneck.
- Do not enable `-ffast-math` or `-march=native` in distributed builds. A local native preset is allowed but not shipped.
- Do not write hand-tuned SIMD without a benchmark, a scalar fallback, and tests on both paths.
- Avoid full-file buffering, large intermediate copies, and holding decoded frames longer than necessary.
- Keep codec thread counts within the scheduler budget.
- Prefer hardware-accelerated codecs only when runtime detection is safe and a software path exists or the limitation is documented.
- Do not add a cache “just in case.” Every cache has a size limit, eviction policy, and measured benefit.

## 6. Security, privacy, and offline guarantees

### 6.1 Threat model

Morpho processes files from untrusted sources. The relevant threats are malformed files, decompression bombs, malicious metadata, path traversal, symlink escapes, resource exhaustion, parser memory corruption, and compromised dependencies. There is no remote attacker over the network because the release build has no network client.

### 6.2 Hard rules

- **No network at runtime.** Do not link Qt Network, libcurl, or any HTTP client. Build FFmpeg with `--disable-network`. Do not register URL handlers or fetch remote resources.
- **No external converters or processes for file processing.** No shell, no `PATH` lookup, no downloaded helper binaries, no dynamic plugin loading from user paths. Opening the output folder through a desktop portal is the only permitted desktop integration.
- **No telemetry or crash uploads.** Logs and optional crash dumps stay local and are shared only by an explicit user action.
- **All input is hostile.** Validate before allocating. Enforce pixel, duration, channel, entry, and expansion limits.
- **Workspaces are private.** Use unpredictable names, mode `0600`, and cleanup on every exit path. Never extract or write through symlinks.
- **Atomic output only.** No partial final files. Never overwrite without an explicit policy.
- **No secrets.** The project has no API keys or credentials. Never commit tokens, logs with personal paths, or user files.

### 6.3 Input handling

- Detect formats by magic bytes and backend probes, not extensions.
- Limit image dimensions and total pixels before decoding. Use scaled decoding for thumbnails.
- Limit video duration, stream count, and resolution before allocating large buffers.
- Install an FFmpeg interrupt callback tied to the job's stop token and a generous per-operation timeout. A malformed file must not hang a worker forever.
- For archives: reject absolute paths, `..` components, device files, escaping symlinks, and hardlinks outside the destination. Apply entry-count and decompressed-size quotas.
- For PDF input (future): disable scripting and external resource loading, enforce page and memory limits, and fuzz the parser. PDF support requires an ADR.
- Reject encrypted or DRM-protected content with a clear `UnsupportedFormat` error.
- Verify output headers and basic properties before publishing.

### 6.4 Build hardening

Release builds use, where supported by the toolchain:

- `-fPIE` and `-pie`
- `-Wl,-z,relro,-z,now`
- `-fstack-protector-strong`
- `-D_FORTIFY_SOURCE=3`
- `-fstack-clash-protection`
- `-fcf-protection=full`
- No executable stack, no `setuid` binary, no static LGPL libraries

CI builds and tests the hardening flags. Do not disable them to make a build pass.

### 6.5 Privacy

- Files are opened only from local paths selected by the user.
- No account, cloud, analytics, ad, or update-check code exists in the product.
- Settings and logs live under XDG config/state directories. Provide a way to clear logs and reset settings.
- Logs redact full paths at info level and never include file contents.
- Opening the output folder uses the desktop portal or file manager. Do not open URLs.
- Any future feature that needs the network requires an ADR, an explicit opt-in, off-by-default behavior, and a privacy review. Prefer not adding it.

### 6.6 Offline verification

CI runs the integration suite in a network-isolated environment (network namespace or equivalent). Conversions must succeed with no outbound sockets. As a second check, run a representative conversion under `strace -f -e trace=network` and assert that no connection is attempted. The release build is audited with `ldd` and `nm` to confirm that no network client is linked.

### 6.7 Vulnerability reporting

Use GitHub private vulnerability reporting. Do not open a public issue for a security problem. Include the version, distribution, affected format, reproduction steps, and a minimal sample if it can be shared safely. Maintainers acknowledge reports privately and coordinate a fix and disclosure.

## 7. Licensing and dependencies

### 7.1 Project license

Morpho's original code is licensed under the **MIT License**. The `LICENSE` file contains the full text with the copyright line `Copyright (c) 2026 Morpho contributors`.

Every original source file starts with:

```cpp
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors
```

C files use block comments:

```c
/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Morpho contributors
 */
```

Vendored or third-party files keep their own headers and are listed in `THIRD_PARTY_NOTICES.md`. Binary assets such as logos, icons, and test fixtures require provenance and a license entry in `assets/README.md` or `tests/fixtures/README.md`; do not commit unlicensed images or media.

Contributions are accepted under the MIT license. Commits use the Developer Certificate of Origin (DCO) sign-off (`git commit -s`). Do not contribute code you cannot license under these terms.

### 7.2 Dependency policy

- Every runtime dependency needs an ADR, a license review, a maintenance check, a size/performance measurement, and an entry in `THIRD_PARTY_NOTICES.md`.
- Prefer the C++ standard library and Qt over a new library. Prefer a small, maintained library over a large framework.
- Pin versions and hashes. No floating branches, no automatic major updates, no unverified downloads.
- No dependency that phones home, bundles telemetry, or requires network access at runtime.
- No dependency with an unclear license, no license file, or abandoned upstream without a maintenance plan.
- Test-only dependencies follow the same license rules and must not leak into runtime artifacts.

### 7.3 Approved dependencies for 1.0

This table is the runtime allowlist. Anything else requires an ADR and maintainer approval.

| Dependency | License | Linking / notes |
| --- | --- | --- |
| Qt 6 Core, Gui, Widgets, Svg | LGPL-3.0 | Dynamic linking only; ship notices and allow replacement. No static Qt. |
| FFmpeg libavcodec, libavformat, libavutil, libswscale, libswresample | LGPL-2.1-or-later | Dynamic linking; build without GPL, nonfree, or network components. |
| libpng | libpng-2.0 | Permissive. |
| libjpeg-turbo | BSD-3-Clause / IJG | Permissive; verify the exact upstream notices. |
| libwebp | BSD-3-Clause | Permissive. |
| libavif + libaom / SVT-AV1 | BSD-2/3-Clause | Permissive; verify per release. AVIF output uses libavif. |
| libvpx | BSD-3-Clause | VP9 encoding. |
| libtiff | libtiff (BSD-like) | Permissive. |
| libarchive | BSD-2-Clause | Permissive. |
| libharu | Zlib | Permissive; PDF writing. |
| Catch2 | BSL-1.0 | Test-only. |
| openh264 | BSD-2-Clause | Optional H.264 encoder; patent considerations apply. |
| libmp3lame | LGPL-2.0-or-later | Dynamic linking only; optional MP3 encoder. |
| libopus, libvorbis, libFLAC | BSD-3-Clause | Permissive. |
| Qt Test, LinguistTools | Build/test only | Not linked into the shipped application; verify terms before distributing tooling. |

HEIC/HEIF input is a future candidate, not part of the 1.0 matrix. It requires libheif plus an approved HEVC decoder and a separate ADR.

### 7.4 Forbidden dependencies

- GPL, AGPL, or SSPL runtime libraries, including Poppler, MuPDF, Ghostscript, x264, x265, and GPL-only Qt modules.
- Nonfree codecs such as libfdk-aac, and nonfree archive tools such as unrar.
- Qt WebEngine, Electron, Chromium, Node.js, or any embedded web runtime.
- libcurl, HTTP clients, analytics SDKs, crash-reporting SDKs, and auto-update frameworks.
- Proprietary SDKs, binary blobs without redistribution rights, and code copied from sources whose license is incompatible with MIT.
- Static linking of LGPL libraries.

GPL tools used only at build time and not distributed (for example, a compiler or code generator) are acceptable. Do not link or bundle them.

### 7.5 LGPL compliance

- Link LGPL libraries dynamically. Never use `-Wl,-Bstatic` for them or vendor static archives.
- Ship unmodified shared libraries with their license texts and a written offer or pointer to the corresponding source.
- Do not prevent users from replacing or relinking the LGPL library. Document how each AppImage and Flatpak layout satisfies the LGPL relinking requirements, for example through extract/repack instructions or corresponding object files.
- If an LGPL library is patched, publish the patch and build instructions.
- Include `THIRD_PARTY_NOTICES.md` and an SBOM in every release.
- Perform a legal review before the first public binary release and whenever the dependency set changes materially.

### 7.6 Codecs and patents

- Prefer royalty-free formats: AV1, VP9, Opus, Vorbis, and FLAC.
- H.264, HEVC, and AAC involve patent pools. Do not claim patent safety. Get legal review before distributing binaries in relevant jurisdictions.
- Do not ship GPL encoders (x264, x265) or nonfree codecs. H.264 encoding uses openh264 or hardware support only when available; otherwise the format is unsupported and the UI says so.
- The FFmpeg build is part of the repository and uses at least:

```text
--disable-gpl
--disable-nonfree
--disable-network
--disable-programs
--disable-doc
--disable-debug
--disable-static
--enable-shared
--disable-autodetect
```

- CI verifies the resulting build configuration and fails if GPL, nonfree, or network support is enabled.

## 8. Repository layout

Target layout. Create directories only when they contain files; git does not track empty directories.

```text
.
|-- AGENTS.md
|-- README.md
|-- LICENSE
|-- CONTRIBUTING.md
|-- CODE_OF_CONDUCT.md
|-- SECURITY.md
|-- CHANGELOG.md
|-- THIRD_PARTY_NOTICES.md
|-- CMakeLists.txt
|-- CMakePresets.json
|-- .clang-format
|-- .clang-tidy
|-- .editorconfig
|-- .gitignore
|-- .github/
|   |-- workflows/
|   |-- ISSUE_TEMPLATE/
|   `-- pull_request_template.md
|-- cmake/
|-- assets/                  # icons, logo, licensed media, and provenance
|-- include/morpho/          # public core API; no Qt, no third-party types
|-- src/
|   |-- core/
|   |   |-- domain/
|   |   |-- engine/
|   |   `-- io/
|   |-- backends/
|   |   |-- image/
|   |   |-- audio/
|   |   |-- video/
|   |   |-- document/
|   |   `-- archive/
|   |-- platform/
|   |   `-- linux/
|   |-- ui/
|   |-- cli/
|   `-- app/
|-- tests/
|   |-- unit/
|   |-- integration/
|   |-- fixtures/
|   `-- fuzz/
|-- benchmarks/
|-- docs/
|   |-- adr/
|   |-- architecture.md
|   |-- formats.md           # generated; do not hand-edit
|   `-- user/
|-- packaging/
|   |-- appimage/
|   |-- flatpak/
|   |-- deb/
|   `-- ffmpeg/
|-- scripts/
`-- translations/
```

CMake target names: `morpho_core`, `morpho_backends`, `morpho_ui`, `morpho` (GUI executable), `morpho-cli`. Use namespaced aliases such as `Morpho::core` for consumers.

## 9. Build, test, and run

### 9.1 Prerequisites

Reference environment: **Ubuntu 24.04 LTS x86_64**, GCC 13+ or Clang 18+, CMake 3.28+, Ninja, pkg-config, Qt 6.4+ development packages, FFmpeg 6.1+ development libraries, the approved codec development packages, Catch2 v3, clang-format 18, clang-tidy 18, and ccache. Wayland and X11 sessions are both supported for manual testing.

Development packages are installed by `scripts/bootstrap-ubuntu.sh` once M0 lands. Do not install packages system-wide from an AI agent without explicit user approval. Prefer the dev container or CI image when available.

### 9.2 Canonical commands

```bash
git clone https://github.com/ImLecus/morpho.git
cd morpho
./scripts/bootstrap-ubuntu.sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev --output-on-failure
./build/dev/src/app/morpho
```

Run the full local gate before opening a pull request:

```bash
./scripts/check.sh
```

If the scripts do not exist yet, the equivalent CMake commands are canonical:

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev --output-on-failure
```

Run a subset of tests:

```bash
ctest --preset dev -R image --output-on-failure
```

Build and test with sanitizers:

```bash
cmake --preset asan
cmake --build --preset asan
ctest --preset asan --output-on-failure
```

Release build:

```bash
cmake --preset release
cmake --build --preset release
```

### 9.3 CMake presets and options

| Preset | Purpose |
| --- | --- |
| `dev` | Debug, tests on, benchmarks off, ccache, no LTO. |
| `release` | RelWithDebInfo, tests on, LTO, hardening, reproducible flags. |
| `asan` | Debug with AddressSanitizer, UndefinedBehaviorSanitizer, and LeakSanitizer. |
| `tsan` | Debug with ThreadSanitizer for scheduler tests. |
| `fuzz` | Clang libFuzzer targets, fuzzing enabled. |
| `ci` | Release, warnings as errors, tests on, benchmarks off. |

| Option | Default | Meaning |
| --- | --- | --- |
| `MORPHO_BUILD_TESTS` | `ON` when top-level | Build unit, integration, and UI tests. |
| `MORPHO_BUILD_BENCHMARKS` | `OFF` | Build performance benchmarks. |
| `MORPHO_WARNINGS_AS_ERRORS` | `OFF` locally, `ON` in CI | Treat compiler warnings as errors. |
| `MORPHO_ENABLE_ASAN` | `OFF` | Enable ASan, UBSan, and LSan. |
| `MORPHO_ENABLE_TSAN` | `OFF` | Enable ThreadSanitizer. |
| `MORPHO_ENABLE_LTO` | `ON` for release | Link-time optimization. |
| `MORPHO_NATIVE_OPTIMIZATIONS` | `OFF` | Local-only `-march=native`; never ship artifacts built with it. |

### 9.4 Build rules

- Target-based CMake only. Set compile features and flags on targets, not globally.
- No `file(GLOB)`. List sources explicitly.
- `CMAKE_CXX_EXTENSIONS OFF`, `CMAKE_C_EXTENSIONS OFF`, C++20, C17.
- `morpho_core` has no Qt in its link interface. CI verifies this.
- Warnings at minimum: `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wold-style-cast -Wnon-virtual-dtor -Woverloaded-virtual -Wformat=2 -Wundef -Wcast-qual`.
- Warnings as errors in CI. Do not silence a warning globally; fix it or add a narrow, commented suppression.
- Release builds use `-O3`, LTO, and the hardening flags from Section 6.4. Debug builds use `-O0 -g`.
- Reproducible builds: use `-ffile-prefix-map`, honor `SOURCE_DATE_EPOCH`, and avoid embedding timestamps or absolute build paths.
- Run each public header through a standalone compile check to catch missing includes.

### 9.5 Continuous integration

GitHub Actions runs on Ubuntu 24.04:

- Format check with clang-format.
- clang-tidy on changed C/C++ files.
- GCC 13 release build and tests.
- Clang 18 ASan/UBSan build and tests.
- TSan for scheduler tests.
- Offline integration suite.
- License and dependency audit.
- Packaging smoke build on tags.
- Nightly: fuzzing, valgrind subset, benchmarks, AppImage and Flatpak builds.

Required checks for `main`: format, build, tests, sanitizer, offline, and license. Pin third-party actions by commit SHA, use least privilege, and never expose secrets to pull requests from forks. Cache ccache and package downloads, not build directories.

### 9.6 Dependency acquisition

- Development uses system packages from the pinned Ubuntu 24.04 image. `scripts/bootstrap-ubuntu.sh` records the package set.
- Release builds run in a pinned container and bundle all non-system shared libraries. Do not rely on host libraries.
- Windows and macOS builds will use a pinned manifest (for example, vcpkg) plus an ADR. Do not add a second package manager before that decision.
- Avoid `FetchContent` for runtime dependencies. If a header-only test utility must be fetched, pin the commit and hash and record the license.

## 10. Testing

### 10.1 Test levels

| Level | Scope | Tooling |
| --- | --- | --- |
| Unit | Pure core logic, path handling, limits, options, wrappers | Catch2 |
| Integration | End-to-end conversions per format pair, cancellation, collisions, permissions, Unicode paths, corrupt input | Catch2 + CTest + CLI/core |
| UI | Queue model, dialogs, drag-and-drop, keyboard flow, accessible names | Qt Test with `QT_QPA_PLATFORM=offscreen` |
| Fuzz | Probe, metadata, archive extraction, option parsing | libFuzzer |
| Performance | Startup, memory, throughput, cancel latency | Benchmarks + `/usr/bin/time -v` |
| Packaging | Clean-install smoke test, library closure, offline behavior | Containers + `ldd`/`readelf` |
| Manual | X11/Wayland, HiDPI, light/dark, screen reader, keyboard-only | Release checklist |

### 10.2 Requirements

- Every bug fix gets a regression test. Write the failing test first where practical.
- Every feature has tests for success, invalid input, cancellation, and relevant limits.
- Core line coverage target: 80% overall; changed core lines: 90%. UI coverage is not gated, but critical flows need smoke tests.
- Tests are deterministic. Use fixed seeds, unique temporary directories, and no reliance on wall-clock timing.
- No network access in tests. No downloading fixtures. No external converter binaries.
- UI tests use `QSignalSpy` and `QTRY_*`; do not use `sleep()` or arbitrary timeouts.
- Integration tests may use fakes for platform services, but codec tests use the real libraries.
- Flaky tests are defects. Quarantine only with a linked issue and an owner.
- Run sanitizer tests for new core and backend code. Run TSan when touching the scheduler or shared state.
- Fuzz targets run nightly and before releases. A crash or timeout is a release blocker.

### 10.3 Fixtures

- Keep fixtures small: at most 100 KiB per file and 5 MiB total per test suite.
- Record the provenance, license, and creation method in `tests/fixtures/README.md`.
- Prefer generating fixtures at test time from documented parameters, but keep at least one independently produced sample per format to catch encoder/decoder agreement bugs.
- Do not commit personal photos, copyrighted media, or files with sensitive metadata. Strip metadata or create synthetic samples.
- Store checksums for binary fixtures and verify them in tests.

### 10.4 Manual release QA

- Launch on X11 and Wayland.
- Test 100%, 150%, and 200% scaling.
- Test light and dark system themes.
- Operate the full flow with the keyboard only.
- Verify screen-reader names and focus order with Orca or the platform equivalent.
- Convert a 100-file batch, cancel midway, retry, and close the window during conversion.
- Test read-only output directories, low disk space, Unicode and very long filenames, and duplicate names.
- Run the AppImage and Flatpak on a clean VM with no Morpho development packages installed.
- Verify no network activity during launch and conversion.

## 11. Git workflow

### 11.1 GitHub Flow branches

Morpho uses **GitHub Flow** as its branch management strategy:

- `main` is protected, always releasable, and never receives direct pushes.
- Create a short-lived branch from `main` for every change. Use `feat/<slug>`, `fix/<slug>`, `perf/<slug>`, `refactor/<slug>`, `docs/<slug>`, `test/<slug>`, `build/<slug>`, `ci/<slug>`, `chore/<slug>`, or `revert/<slug>`.
- Include an issue number when useful: `fix/123-cancel-empty-file`.
- Open a pull request as soon as the branch is reviewable. Keep the branch focused and short-lived.
- Rebase on `main` before requesting review. Resolve conflicts locally and rerun tests.
- Merge with squash merge after approval and green CI, then delete the branch.
- Do not maintain long-lived `develop`, `release`, or environment branches. Releases are tags on `main`.
- Hotfixes use the same branch-and-pull-request flow. A patch for an older release line is the only exception and requires maintainer approval.

### 11.2 Commits

Morpho uses **Conventional Commits 1.0.0**.

```text
<type>(<scope>)!: <description>

<optional body: what changed and why>

<optional footers>
```

- Types: `feat`, `fix`, `perf`, `refactor`, `docs`, `test`, `build`, `ci`, `chore`, `style`, `revert`.
- Scopes: `core`, `engine`, `ui`, `cli`, `image`, `audio`, `video`, `document`, `archive`, `platform`, `i18n`, `a11y`, `build`, `ci`, `deps`, `docs`, `tests`, `packaging`, `repo`.
- Description: imperative mood, lowercase after the type, no trailing period, at most 72 characters.
- Body: explain why and any tradeoff, wrapped at 72 columns. Do not restate the diff.
- Breaking change: add `!` after the scope and a `BREAKING CHANGE:` footer explaining migration.
- Reference issues in footers: `Refs: #123` or `Closes: #123`.
- One logical change per commit. Every commit should build and pass its focused tests.
- Do not mix formatting-only changes with behavior changes.
- Sign off contributions with `git commit -s` (DCO).
- Do not hand-edit `CHANGELOG.md`; it is generated from commits.

Examples:

```text
feat(image): add AVIF output support

fix(core): preserve existing output when conversion fails

perf(video): bound the frame queue to reduce peak memory

refactor(ui): extract the conversion queue model from the main window

docs(adr): accept the bundled FFmpeg policy

build(deps): update libwebp to 1.4.0

test(image): cover CMYK JPEG input

ci: run the offline integration suite in a network namespace

feat(cli)!: replace --to with --format

BREAKING CHANGE: scripts using --to must switch to --format.
```

### 11.3 Pull requests

- Open an issue first for non-trivial features, architecture changes, new dependencies, or behavior changes.
- Use a Conventional Commit title that can become the squash commit.
- Describe the problem, the approach, the tests run, and the risks. Link the issue.
- Include before/after screenshots or a short recording for UI changes, in light and dark themes.
- Include benchmark numbers and the measurement environment for performance changes.
- Call out license, dependency, privacy, or security implications explicitly.
- Keep PRs focused. Aim for at most 400 changed lines excluding tests and generated files; split larger work.
- Open a draft PR early for feedback on risky work.
- Update tests, docs, and the generated format matrix in the same PR.
- Do not include unrelated refactors, formatting churn, or drive-by dependency updates.
- Disclose substantial AI assistance in the PR description; the author remains responsible for reviewing every line.

### 11.4 Reviews and merges

- At least one maintainer approval is required. Two approvals are required for licensing, security, or architecture changes.
- CI must be green and all conversations resolved before merge.
- Review for correctness, ownership, error paths, resource limits, tests, performance, accessibility, and license compliance. Style is automated; do not spend review time on it.
- Assume good faith. Discuss code, not people. Block only on correctness, security, licensing, or documented conventions.
- Squash merge only. The PR title becomes the commit subject; the PR description informs the body.
- Do not force-push shared branches. Force-pushing your own PR branch is allowed before review begins.
- Do not merge your own PR without approval unless you are the sole maintainer and CI is green.

### 11.5 Releases

- Semantic Versioning: `MAJOR.MINOR.PATCH`, tags `vX.Y.Z`.
- Before 1.0, minor releases may include breaking changes; document them clearly in the changelog.
- Generate `CHANGELOG.md` from Conventional Commits. Group features, fixes, performance, and breaking changes.
- Release artifacts include the binary/package, SHA-256 checksums, SBOM, `THIRD_PARTY_NOTICES.md`, and license texts.
- Sign tags and artifacts when maintainer keys are available. At minimum, publish checksums and a reproducible build recipe.
- Release checklist: CI green, manual QA complete, licenses reviewed, offline test passed, version updated, tag created, artifacts smoke-tested, release notes published.
- There is no auto-update. Users update through their package manager or by downloading a new artifact.
- Hotfixes normally branch from `main` and receive a patch release. If an older release line must be patched, branch from the affected tag with maintainer approval and merge the fix back to `main`.

### 11.6 What not to commit

- Build directories, compiled artifacts, object files, libraries, and packaged binaries.
- IDE or editor state (`.vscode/`, `.idea/`, swap files) unless the team explicitly standardizes a shared config.
- Logs, core dumps, coverage output, benchmark output, and temporary files.
- Secrets, tokens, credentials, personal paths, or user files.
- Large media, generated fixtures without provenance, or files above 100 KiB without maintainer approval.
- Generated translation binaries (`.qm`); commit `.ts` sources only.
- Vendored dependencies without an ADR and license review.

Keep `.gitignore` current. If a file is ambiguous, ask before committing it.

## 12. Documentation

### 12.1 Required documents

- `README.md`: what Morpho is, current status, build and run instructions, supported platforms, links to user docs, and license.
- `CONTRIBUTING.md`: contributor workflow, mirrored from this file for humans.
- `CODE_OF_CONDUCT.md`: Contributor Covenant 2.1 or later.
- `SECURITY.md`: private reporting process and supported versions.
- `CHANGELOG.md`: generated from commits.
- `THIRD_PARTY_NOTICES.md`: dependency licenses, versions, linking mode, and modifications.
- `docs/architecture.md`: detailed architecture and data flow.
- `docs/formats.md`: generated capability matrix.
- `docs/user/`: installation, usage, supported formats, privacy, troubleshooting.
- `docs/adr/`: Architecture Decision Records.
- `LICENSES/`: full license texts when required by dependencies; the project MIT text remains in `LICENSE`.

### 12.2 Architecture Decision Records

Use `docs/adr/NNNN-short-title.md` with:

```markdown
# NNNN: Title

- Status: Proposed | Accepted | Superseded by NNNN | Rejected
- Date: YYYY-MM-DD
- Deciders: names or roles

## Context

## Decision

## Consequences

## Alternatives considered
```

ADRs are required for: language or GUI framework changes, new runtime dependencies, new format families, threading or scheduler redesigns, storage changes, network features, plugin ABIs, license policy exceptions, and breaking CLI/config changes. Accepted ADRs are immutable; supersede them with a new ADR that links back.

### 12.3 Documentation style

- US English, present tense, active voice, second person for instructions.
- Sentence case for headings. One idea per paragraph. Short sentences.
- Code fences include a language. Commands are copy-pasteable.
- Link to files with relative links. Avoid duplicating content that will drift; link instead.
- Screenshots need alt text. Diagrams need a text explanation.
- Do not include secrets, personal data, or unlicensed assets.
- Update docs in the same PR as the behavior change. A feature without documentation is incomplete.

## 13. UX, accessibility, and localization

### 13.1 Main flow

1. Add files with the native file dialog or drag-and-drop.
2. Review the queue: name, type, size, target format, status, progress.
3. Choose the target format and options for the batch, with per-file overrides where supported.
4. Choose the output directory and collision policy.
5. Convert. Jobs run in the background with progress and cancel controls.
6. On completion, open the output folder, reveal a file, or retry failures.

The UI should make the local-only guarantee visible without turning privacy into a marketing banner. A short “Files stay on this device” line near the drop zone is enough.

### 13.2 Interaction principles

- No surprises: never overwrite, move, or delete a file without explicit confirmation.
- Fast feedback: show a job as soon as it is queued; show probing as indeterminate; show conversion progress.
- Reversible where possible: cancel before publish leaves no output; retry starts a new attempt.
- Batch-first: operations apply to the selection, with clear scope.
- Errors are actionable: say what failed, which file, and what to try next. Put technical details behind a disclosure.
- No modal dialogs for success. Reserve modals for destructive or irreversible actions.
- The window can close during conversion only after confirming cancellation; cleanup still runs.

### 13.3 Copy and errors

- Sentence case, concise, plain language. Avoid jargon in primary messages.
- Use the file's base name, not the full path, in user-facing errors.
- Error pattern: what happened + likely cause + next step. Example: “Couldn't convert vacation.png. The file may be corrupted. Try another copy or re-export it.”
- Never blame the user. Never show a raw codec error as the primary message.
- Include a stable error code in the details section for bug reports.

### 13.4 Accessibility

- Full keyboard operation, visible focus, logical tab order, and documented shortcuts.
- Accessible names and descriptions for every control, including icon-only buttons.
- Screen-reader support through Qt accessibility. Test with Orca on Linux.
- Do not rely on color alone for status. Pair color with text or icons.
- Contrast meets WCAG 2.2 AA. Support high-contrast themes.
- Respect font scaling and 200% display scaling. Use layouts, not fixed positions.
- Target size of at least 24x24 device-independent pixels for interactive controls.
- Respect reduced-motion settings; avoid gratuitous animation.
- Progress is announced at reasonable intervals, not on every frame.

### 13.5 Localization

- All user-visible strings use `tr()`. Source language is English (`en`).
- Use `%n` plural forms and named placeholders; never concatenate translated fragments.
- Format numbers, dates, and sizes with `QLocale`.
- Do not embed text in images. Use translatable labels.
- Translation sources live in `translations/*.ts`; compiled `.qm` files are build artifacts.
- Test with pseudo-localization (long strings, right-to-left where applicable) before release.
- Changing a source string invalidates its translation; keep strings stable and meaningful.

### 13.6 Visual design

- Use the system palette and native controls. Follow light/dark mode automatically.
- No hard-coded colors. Use semantic roles (error, warning, success, muted).
- Icons are SVG and inherit the palette.
- Prefer a simple, dense, utilitarian layout over decorative chrome.
- Motion is subtle and optional. No web-style landing pages inside the app.

## 14. Roadmap

The order may change through an approved issue and ADR. Each milestone ends with tests, docs, and a working build.

### M0: Foundation (current)

- Repository scaffolding: `LICENSE`, `README.md`, `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `SECURITY.md`, `.editorconfig`, `.gitignore`.
- CMake project, presets, target skeleton, and CI.
- Core types: `Result`, `Error`, format descriptor, conversion request, job state, limits.
- Qt main window shell with the queue layout and empty state.
- CLI skeleton: `morpho-cli --version`, `formats`.
- Catch2 and Qt Test wired into CTest.
- First ADRs: language/GUI, bundled backends, licensing, offline guarantee, build baseline.
- Packaging skeleton for AppImage.
- **Exit:** a clean Ubuntu 24.04 container builds, tests, opens the window, and runs `morpho-cli formats`; CI is green.

### M1: Images

- PNG, JPEG, WebP, AVIF, TIFF, and BMP input/output; static GIF input/output first, animation if the tests and memory budget allow.
- Batch conversion, quality/lossless options, alpha-to-JPEG background, metadata policy, output naming and collision handling.
- Real progress, cancellation, retry, and per-file errors.
- Golden tests, corrupt-input tests, fuzzing for probes, and performance baselines.
- **Exit:** every declared pair has an integration test, cancellation is reliable, and image budgets are met.

### M2: Audio

- MP3, AAC/M4A, Opus, Ogg Vorbis, FLAC, and WAV.
- Bitrate, sample rate, channel layout, and metadata options.
- Streaming conversion with bounded memory.
- **Exit:** format matrix complete, tags preserved or stripped according to the documented policy, and audio tests pass under sanitizers.

### M3: Video

- MP4 (H.264/AAC), WebM (VP9/Opus), MKV, MOV, and animated GIF.
- H.264 decoding, encoding through openh264 or hardware where available; no x264/x265.
- Progress, cancellation, frame-queue limits, and duration/resolution guards.
- **Exit:** 1080p conversions meet the memory and cancel budgets; unsupported codec combinations fail clearly.

### M4: Documents and archives

- Images and plain text to PDF through libharu.
- PDF input only after an ADR selects a permissively licensed renderer (for example, PDFium) and documents sandboxing and limits.
- ZIP, TAR, and 7Z inspection/extraction with strict path and size limits; ZIP/TAR creation if the UX justifies it.
- Office document conversion remains out of scope for 1.0.
- **Exit:** archive safety tests cover traversal, symlinks, quotas, and corrupt central directories.

### M5: 1.0

- AppImage and Flatpak releases, optional `.deb`.
- Accessibility audit, pseudo-localization, and at least one community translation if available.
- Security review, dependency audit, SBOM, license review, and offline verification.
- Performance tuning and documentation completion.
- **Exit:** no open P0/P1 issues, budgets met, clean install tested on Ubuntu 24.04 with X11 and Wayland, and all release artifacts pass the smoke test.

### Planned format scope for 1.0

| Category | Input | Output | Primary backend |
| --- | --- | --- | --- |
| Images | PNG, JPEG, WebP, AVIF, TIFF, BMP, GIF | Same set | libpng, libjpeg-turbo, libwebp, libavif, libtiff, FFmpeg with approved filters |
| Audio | MP3, AAC/M4A, Opus, Ogg Vorbis, FLAC, WAV | Same set | FFmpeg and approved codec libraries |
| Video | MP4, WebM, MKV, MOV, GIF | MP4, WebM, MKV, MOV, GIF | FFmpeg, openh264/hardware, VP9/Opus |
| Documents | PNG/JPEG/WebP/TIFF/BMP, plain text | PDF | libharu |
| Archives | ZIP, TAR, 7Z | ZIP, TAR (if implemented) | libarchive |

Format support is added only with a backend, license clearance, limits, tests, and a generated matrix entry. The table is directional; `docs/formats.md` is authoritative once generated.

## 15. Working agreement for AI agents

You are a contributor, not an autonomous maintainer. Optimize for a small, correct, reviewable change that respects the user's existing work.

### Before changing code

1. Read this file, `README.md`, `CONTRIBUTING.md`, the relevant ADRs, `docs/formats.md`, and the code and tests near the change.
2. Run `git status` and inspect uncommitted work. Never overwrite, revert, or delete user changes.
3. Identify the smallest coherent change. If the request spans several concerns, propose a sequence instead of one large diff.
4. Search for existing patterns and utilities before inventing new ones.
5. Ask before proceeding if the request is ambiguous, architectural, or conflicts with this document.

### While changing code

- Follow the existing structure, naming, error model, and test style.
- Do not perform drive-by refactors, formatting churn, or unrelated dependency updates.
- Add or update tests and documentation in the same change.
- Add the SPDX header to new source files.
- Never fabricate APIs, flags, or library behavior. Verify signatures and semantics against the installed headers or upstream documentation.
- Do not add a runtime dependency, network capability, external process, GPL/AGPL library, static LGPL link, telemetry, or plugin loader without explicit maintainer approval and an ADR.
- Do not edit generated files (`docs/formats.md`, `.qm` files, generated changelogs) by hand.
- Do not run privileged commands, install system packages, or modify files outside the repository without user approval.
- Do not use destructive git commands (`reset --hard`, `clean -fd`, force push) unless the user explicitly asks and the target is your own unshared work.
- Never commit secrets, tokens, user files, large binaries, or build artifacts.
- Keep repository artifacts in English. Reply to the user in their language.

### After changing code

- Run formatting, build, and the relevant tests. For core or backend changes, run sanitizer tests when feasible.
- Report the exact commands you ran and their results. If you could not run something, say so explicitly.
- Do not claim performance improvements without measurements. Include the environment and before/after numbers.
- Summarize the change, the risks, and any follow-up work.
- Commit only when the user asks. Use Conventional Commits and DCO sign-off. Do not push or open a pull request unless asked.
- If a test fails for an unrelated reason, report it instead of hiding it or weakening the test.

### Ask first

- Adding a runtime dependency or changing the dependency set.
- Adding network access, telemetry, an external process, or an auto-update mechanism.
- Introducing a new format family or changing the public format matrix.
- Changing public CLI flags, JSON output, config keys, or file locations.
- Rewriting the architecture, scheduler, or error model.
- Deleting user data or rewriting shared git history.
- Changing this document or the license policy.

### Never

- Add network code, analytics, ads, or URL fetching to the product.
- Shell out to converters or invoke downloaded binaries.
- Static-link LGPL libraries or ship GPL/AGPL runtime code.
- Disable tests, sanitizers, warnings, or hardening to make a change pass.
- Add a global `catch (...)` that swallows errors.
- Push to `main`, force-push a shared branch, or merge without approval.
- Present unverified work as complete.

### Definition of done

A change is done when:

- The behavior is implemented and covered by tests, including failure and cancellation paths.
- Formatting, linting, build, and tests pass locally and in CI.
- Documentation and the format matrix are updated where relevant.
- No new warnings, leaks, network calls, external processes, or forbidden dependencies exist.
- Performance budgets are respected, or the tradeoff is measured and documented.
- Accessibility and localization requirements are met for UI changes.
- License headers and third-party notices are correct.
- The commit follows Conventional Commits and is signed off if it will be contributed.

## 16. Changing this document

- Propose changes in a pull request with a `docs(agents):` commit and a short explanation.
- Architectural changes require an ADR; update this document to link to it.
- Keep this file concise and normative. Move long explanations to `docs/` and link them.
- Update `CONTRIBUTING.md` in the same pull request so human and agent guidance do not drift.
- Maintainers review this document when the stack, license policy, or roadmap changes materially, and at least once per release cycle.

## Appendix A: Glossary

- **Backend:** An in-process implementation that probes and converts one format family.
- **Capability matrix:** The generated list of supported input/output pairs, options, and limits.
- **Converter:** The interface a backend implements.
- **DCO:** Developer Certificate of Origin; a sign-off certifying the right to contribute code.
- **Fixture:** A small, licensed test file with documented provenance.
- **Job:** One conversion request and its lifecycle, from queue to published output.
- **Probe:** Detecting the real format and inspecting metadata before conversion.
- **Publish:** Atomically moving a verified temporary output to its final path.
- **Target format:** The requested output format for a job.
- **Workspace:** A private temporary directory used during one job and removed afterward.
