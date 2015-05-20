Name:           xbs
Version:        0
Release:        8
Url:            https://github.com/dmt4/xbs
Summary:        X ball-and-sticks
License:        GPL-2.0
Group:          Productivity/Scientific/Chemistry
Source:         %{name}-%{version}.tar.xz
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
BuildRequires:  libX11-devel
%if 0%{?suse_version}
BuildRequires:  fdupes
%endif

%description
X ball-and-sticks shows pseudo-3D cartoon models of atomistic 
configurations. It has sophisticated data format and offers 
various labelling, shading, lighting and colouring options.


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

%if 0%{?suse_version}
%fdupes -s $RPM_BUILD_ROOT
%endif

#%%clean
#rm -rf %%{buildroot}

%files
%defattr(-,root,root)
%doc README.md in.bs
%{_bindir}/*
%{_mandir}/man1/*

%files doc
%defattr(-,root,root)
%doc Changes.xbs debian/README.debian debian/changelog debian/runex misc/ examples/


%changelog
