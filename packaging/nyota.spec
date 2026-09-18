%global debug_package %{nil}

Name:           nyota
Version:        0.5.0
Release:        3%{?dist}
Summary:        Nyota programming language interpreter
License:        Proprietary
URL:            https://github.com/Klucznik26/Nyota
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  pkgconfig(sdl2)

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
