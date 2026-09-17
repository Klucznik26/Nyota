%global debug_package %{nil}

Name:           nyota
Version:        0.5.0
Release:        1%{?dist}
Summary:        Nyota programming language interpreter
License:        Proprietary
URL:            https://github.com/Klucznik26/Nyota
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  pkgconfig(sdl2)
Requires:       sdl2-compat

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

%files
%{_bindir}/nyota
%{_datadir}/nyota/
%{_docdir}/nyota/

%changelog
* Thu Sep 17 2026 Marek <marekwnakle@gmail.com> - 0.5.0-1
- First Fedora package of the Linux Nyota host.
