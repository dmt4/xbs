Name:           xbs
Version:        0
Release:        8
Url:            https://github.com/dmt4/xbs
Summary:        Ball-and-sticks plotting program
License:        GPL-2.0
Group:          Productivity/Scientific/Chemistry
Source:         %{name}-%{version}.tar.xz
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
BuildRequires:  libX11-devel
BuildRequires:  fdupes


%description
The ball-and-sticks plotting program can create still
and moving three dimensional models of molecules. X11 and
PostScript output are available.  Models can be rotated,
scaled, etc. Various labeling, shading, lighting, 
coloring options are available.


%package doc
Summary:        Documents and examples for %{name}
Requires:       %{name} = %{version}-%{release}
Requires:       tcsh
Requires:       perl-base

%description doc
Extra docs and examples for %{name}


%prep
%setup -q


%build
%{__make} %{?_smp_mflags}


%install
install -D -s -m 755 %{name} %{buildroot}%{_bindir}/%{name}
install -D    -m 644 debian/%{name}.1 %{buildroot}%{_mandir}/man1/%{name}.1
gzip -9 %{buildroot}%{_mandir}/man1/*

%fdupes -s $RPM_BUILD_ROOT


%clean
rm -rf %{buildroot}


%files
%defattr(-,root,root)
%doc README.md in.bs
%{_bindir}/*
%{_mandir}/man1/*

%files doc
%defattr(-,root,root)
%doc Changes.xbs debian/README.debian debian/changelog debian/runex misc/ examples/


%changelog
