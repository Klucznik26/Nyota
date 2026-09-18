%global debug_package %{nil}

Name:           nyota
Version:        0.5.0
Release:        21%{?dist}
Summary:        Nyota programming language interpreter
License:        Proprietary
URL:            https://github.com/Klucznik26/Nyota
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  pkgconfig(sdl2)
BuildRequires:  pkgconfig(SDL2_image)
BuildRequires:  pkgconfig(SDL2_ttf)
BuildRequires:  pkgconfig(fontconfig)

%description
Nyota is a programming language. PRINT, GRAPH, INPUT and DELAY are part
of the language and work on every host. This package provides the Linux
(POSIX + SDL2) host: a nyota CLI that runs .nyo programs.

Tunga (editor) and AyoOS (first system host) are separate projects.

%prep
%setup -q

%build
%make_build

%install
make install DESTDIR='%{buildroot}' PREFIX=%{_prefix}

%check
%make_build test
%make_build test-graph

%files
%{_bindir}/nyota
%{_datadir}/nyota/
%{_docdir}/nyota/

%changelog
* Fri Sep 18 2026 Marek <marekwnakle@gmail.com> - 0.5.0-21
- Redesign segmented EQBOX rendering: no rectangular bar channel behind shaped segments.
- Add SEGMENTS, SEGMENTSIZE and INACTIVEALPHA visual controls.
- Add PEAK, PEAKCOLOR and millisecond PEAKHOLD markers.
- Apply glow per active segment and reserve a dedicated label strip outside vertical bars.

* Fri Sep 18 2026 Marek <marekwnakle@gmail.com> - 0.5.0-20
- Add TBOX single-line text input and SPINBOX signed numeric control.
- Add LISTVIEW and hierarchical TREEVIEW controls.
- Add interactive SPLITTER.
- Add signed LINE/ARC/CIRCLE SCALE with wrapping rotary mode, ticks, custom track/thumb shapes and glow.
- Add multi-needle CLOCK telemetry gauge with zones, custom dial, ticks, needles and glow.
- Add runtime APIs and VS Code support for the new NyotaUI controls.

* Fri Sep 18 2026 Marek <marekwnakle@gmail.com> - 0.5.0-19
- Add EQBOX high-frequency multi-bar visualization with layered backgrounds, gradients/images, segmented shapes, labels and glow.
- Add interactive SLIDER with horizontal/vertical orientation and selectable thumb geometry.
- Add STATBAR and TOOLBAR container controls with optional borders and row/column layouts.
- Add fast EQBOX_SET/EQBOX_BAR update APIs and POSIX rendering caches.
- Add VS Code EQBOX auto-uppercase support while typing.

* Fri Sep 18 2026 Marek <marekwnakle@gmail.com> - 0.5.0-18
- Add interactive SBAR with horizontal/vertical ranges, mouse drag, keyboard control and selectable thumb geometry.
- Add PBAR with continuous and segmented progress rendering.
- PBAR segmented shapes: CIRCLE, TRIANGLE, SQUARE and PARALLELOGRAM.
- SBAR thumb shapes: RECT, ROUND, CIRCLE, DIAMOND, TRIANGLE and PARALLELOGRAM.

* Fri Sep 18 2026 Marek <marekwnakle@gmail.com> - 0.5.0-17
- Render expanded COMBO dropdowns in a top-level window overlay pass.
- Give open COMBO popups input priority over controls underneath.
- Close other COMBO popups when a new one is opened and suppress hover behind the popup.

* Fri Sep 18 2026 Marek <marekwnakle@gmail.com> - 0.5.0-16
- Keep NyotaUI hidden for the full construction phase; do not finish the initial frame from host tick/event pumping.
- Cache POSIX window gradient/image backgrounds instead of rebuilding them on every hover/redraw.
- Skip rounded ancestor masks when controls do not touch rounded corners.
- Limit rounded background anti-aliasing to actual corner regions and skip guaranteed border interiors.

* Fri Sep 18 2026 Marek <marekwnakle@gmail.com> - 0.5.0-15
- Fix first NyotaUI window mapping on Fedora/Wayland by presenting again after SDL_ShowWindow().
- Skip transparent control backgrounds and use SDL_FillRect for solid backgrounds to reduce startup cost.

* Fri Sep 18 2026 Marek <marekwnakle@gmail.com> - 0.5.0-14
- Batch NyotaUI initial construction off-screen and show windows only after the first complete frame.
- Coalesce POSIX UI redraws to one render/present per SDL event batch.

* Fri Sep 18 2026 Marek <marekwnakle@gmail.com> - 0.5.0-13
- Add ROW/COL AUTO layout for PANEL and TAB containers.
- Add rounded child clipping across backgrounds, text, borders, shadows and UI primitives.
- Enable HiDPI logical rendering on the POSIX NyotaUI host.
- Add editable multiline TAREA/TextBox with UTF-8 input and host-neutral text API.

* Fri Sep 18 2026 Marek <marekwnakle@gmail.com> - 0.5.0-12
- Add portable PADX/PADY for textual controls and TABPADX/TABPADY for tab headers.
- Use padding in both SDL2_ttf and bitmap fallback text rendering.

* Fri Sep 18 2026 Marek <marekwnakle@gmail.com> - 0.5.0-11
- Anti-alias rounded/circular borders and focus rings on POSIX.
- Add a focused-tab accent that keeps the tab/content visual connection intact.

* Fri Sep 18 2026 Marek <marekwnakle@gmail.com> - 0.5.0-10
- Anti-alias rounded and shaped NyotaUI surfaces on POSIX.
- Make directional shadows follow control geometry while keeping the no-blur contract.

* Fri Sep 18 2026 Marek <marekwnakle@gmail.com> - 0.5.0-9
- Add Enter/Space keyboard activation for focused NyotaUI controls.
- Use latched NyotaUI button click events so mouse and keyboard activation are consistent.

* Fri Sep 18 2026 Marek <marekwnakle@gmail.com> - 0.5.0-8
- Add ENABLED state and disabled rendering for interactive NyotaUI controls.
- Add keyboard focus traversal and focus indication on POSIX.
- Fix DAREA default radius so CIRCLE/ELLIPSE remain valid without explicit RADIUS.

* Fri Sep 18 2026 Marek <marekwnakle@gmail.com> - 0.5.0-7
- Add neutral NyotaUI desktop defaults and interactive hover/pressed states.
- Render TAB content with square top corners and rounded bottom corners.
- Improve active/inactive tab headers and algorithmic checkbox X rendering.

* Fri Sep 18 2026 Marek <marekwnakle@gmail.com> - 0.5.0-6
- Replace NyotaUI bitmap text with anti-aliased SDL2_ttf rendering on POSIX.
- Resolve SYSTEM and named font families through fontconfig.
- Keep the 8x8 renderer only as a fallback.

* Fri Sep 18 2026 Marek <marekwnakle@gmail.com> - 0.5.0-5
- Add NyotaUI WIN with ROOT/parent hierarchy and LIST-based geometry.
- Add optional WIN CONFIG with TITLE, BG and ICO.
- Add color, image, linear/shape/spiral background support on POSIX/SDL2.

* Fri Sep 18 2026 Marek <marekwnakle@gmail.com> - 0.5.0-4
- Complete the v0.5 parser precedence and numeric edge checks.
- Implement RECORD/WITH, IMPORT, EVERY, ON ERROR/ERR_CODE and SCREEN.
- Extend regression coverage and VS Code language support 0.4.0.

* Fri Sep 18 2026 Marek <marekwnakle@gmail.com> - 0.5.0-3
- Add FILE and DIR/LS through the NyotaHost contract and POSIX backend.
- Add explicit SORT algorithms and safe virtual NYASM.
- Add regression coverage for the new language features.

* Fri Sep 18 2026 Marek <marekwnakle@gmail.com> - 0.5.0-2
- Update Fedora package for current Nyota interpreter.
- Include sprite test assets and full project documentation.
- Run regression and headless graphics tests during RPM build.
- Let RPM automatic dependency generation resolve the SDL2 runtime.

* Thu Sep 17 2026 Marek <marekwnakle@gmail.com> - 0.5.0-1
- First Fedora package of the Linux Nyota host.
