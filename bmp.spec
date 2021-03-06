# RPM spec file for BMP

# FIXME: The bmp RPM requires libasound.so.* without --with alsa. Need to
# disable autoreq.

# XMMS legacy
%{?_with_xmmseq:    %define xmmseq 1}
%{!?_with_xmmseq:   %define xmmseq 0}

# plugins
%{?_with_alsa:      %define alsa 1}
%{!?_with_alsa:     %define alsa 0}
%{?_with_mp3:       %define mp3  1}
%{!?_with_mp3:      %define mp3  0}

# GNOME support
%{?_with_gconf:     %define gconf 1}
%{!?_with_gconf:    %define gconf 0}
%{?_with_gnomevfs:  %define gnomevfs 1}
%{!?_with_gnomevfs: %define gnomevfs 0}

Summary:        Beep Media Player
Name:           bmp
Version:        0.9.7.1
Release:        1
Epoch:          0
License:        GPL
Group:          Applications/Multimedia
Url:            http://beepmp.sourceforge.net
Source0:        %{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

Requires:       unzip
BuildRequires:  gtk2-devel >= 2.4.0, libglade2-devel >= 2.3.1

%if %{gconf}
BuildRequires:  GConf2-devel >= 2.4.0
%endif

%if %{gnomevfs}
BuildRequires:  gnome-vfs2-devel >= 2.4.0
%endif

%description 
Beep Media Player(BMP) is a GTK2 port of the popular X Multimedia
System(XMMS) and more.

Build options:
--with: alsa mp3 gconf gnomevfs xmmseq

%package        devel
Summary:        BMP - Static libraries and header files.
Group:          Applications/Multimedia
Requires:       %{name} = %{epoch}:%{version}-%{release}

%description    devel
Static libraries and header files required for compiling BMP plugins.

%if %{mp3}
%package        mp3
Summary:        BMP - MP3 output plugin
Group:          Applications/Multimedia
Requires:       %{name} = %{epoch}:%{version}-%{release}

%description    mp3
MP3 input plugin for BMP.
%endif

%if %{alsa}
%package        alsa
Summary:        BMP - ALSA output plugin
Group:          Applications/Multimedia
Requires:       %{name} = %{epoch}:%{version}-%{release}
BuildRequires:  alsa-lib-devel >= 1.0.0

%description    alsa
Output plugin for BMP to use with the Advanced Linux Sound
Architecture (ALSA).
%endif

%prep
%setup -q

%build
%configure \
        --disable-opengl \
        %{!?_with_alsa:--disable-alsa} \
        %{!?_with_mp3:--disable-mp3} \
        %{?_with_gconf:--enable-gconf} \
        %{?_with_gnomevfs:--enable-gnome-vfs} \
        %{?_with_xmmseq:--with-xmms-eq}
make %{_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
%find_lang %{name}

rm -f $RPM_BUILD_ROOT%{_libdir}/bmp/*/*.la

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%files -f %{name}.lang
%defattr(-,root,root,-)
%doc AUTHORS COPYING ChangeLog NEWS README
%{_bindir}/beep-media-player
%{_datadir}/bmp/
%{_datadir}/pixmaps/*
%{_datadir}/applications/bmp.desktop
%{_libdir}/libbeep.so.*
%{_libdir}/bmp/Input/libcdaudio.so
%{_libdir}/bmp/Input/libvorbis.so
%{_libdir}/bmp/Input/libwav.so
%{_libdir}/bmp/Output/libOSS.so
%{_libdir}/bmp/Output/libesdout.so
%{_libdir}/bmp/Visualization/libbscope.so
%{_mandir}/man1/*

%files devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/bmp.pc
%{_libdir}/lib*.so
#%{_libdir}/lib*.a
%{_libdir}/lib*.la
%{_includedir}/bmp

%if %{alsa}
%files alsa
%defattr(-,root,root,-)
%{_libdir}/bmp/Output/libALSA.so
%endif

%if %{mp3}
%files mp3
%defattr(-,root,root,-)
%{_libdir}/bmp/Input/libmpg123.so
%endif


%changelog
* Sat Oct 22 2005 Chong Kai Xiong <descender@phreaker.net> - 0:0.9.7.1-1
- Remove .la files instead of using %exclude
- Rename Copyright to License

* Sat Dec  4 2004 Chong Kai Xiong <descender@phreaker.net> - 0:0.9.7-2
- remove duplicate listings in %files
- fix libglade2-devel version requirement
- add option to build with XMMS equalization

* Tue Jul  6 2004 Chong Kai Xiong <descender@phreaker.net> 0:0.9.7-1
- fixed file list to own package-specific directories
- remove vendor, add epoch tag, explicit requires, add unzip to requires
- force version match between plugins and main package
- use %find_lang
- don't install INSTALL

* Thu Jun 24 2004 Chong Kai Xiong <descender@phreaker.net> 0.9.7-3
- added support for GConf and GNOME VFS
- fixed file list

* Fri May 28 2004 Chong Kai Xiong <descender@phreaker.net> 0.9.7-2
- require libglade 2.0

* Sun Apr 05 2004 Chong Kai Xiong <descender@phreaker.net> 0.9.7-1
- require GTK 2.4 and ALSA 1.0

* Tue Jan 13 2004 David Lau <coder_sku@sourceforge.net> 0.9.6-3
- removes plugin .la's

* Wed Dec 24 2003 Chong Kai Xiong <descender@phreaker.net> 0.9.6-2
- first fully usable version

* Tue Nov 29 2003 Chong Kai Xiong <descender@phreaker.net> 0.9.6-1
- added support for --with switches

* Tue Nov 11 2003 Chong Kai Xiong <descender@phreaker.net> 1.0.0pre6
- initial build
