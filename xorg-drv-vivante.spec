Name: xorg-drv-vivante
Version: 12.09.01
Release: alt1
Summary: Xorg driver for GPU integrated in i.MX6 processor
License: MIT/X11
Group: System/X11
Url: http://www.freescale.com/lgfiles/NMG/MAD/YOCTO/
Packager: Pavel Nakonechny <pavel.nakonechny@skitlab.ru>

Requires: XORG_ABI_VIDEODRV = %get_xorg_abi_videodrv

Source: %name-%version.tar
Patch: %name-%version-%release.patch

BuildRequires(Pre): xorg-sdk
BuildRequires: libX11-devel libXext-devel libXvMC-devel xorg-inputproto-devel xorg-fontsproto-devel xorg-randrproto-devel
BuildRequires: xorg-renderproto-devel xorg-xextproto-devel xorg-xf86driproto-devel xorg-dri2proto-devel xorg-xineramaproto-devel
BuildRequires: libXrender-devel libxcbutil-devel xorg-util-macros libXfixes-devel libudev-devel
BuildRequires: xorg-resourceproto-devel xorg-scrnsaverproto-devel

BuildRequires: fsl-gpu-viv-mx6q-devel

Requires: fsl-gpu-viv-mx6q
Requires: xorg-dri-vivante

%description
%summary

%package -n xorg-dri-vivante
Summary: Hardware acceleration driver for GPU integrated in i.MX6 processor
Group: System/X11

Requires: fsl-gpu-viv-mx6q
Requires: xorg-drv-vivante

%description -n xorg-dri-vivante
%summary

%prep
%setup -q
%patch -p1

%build

pushd exa
%autoreconf
LDFLAGS+=" -lm -ldl -lX11 -lGAL-x11 " \
./configure \
	--with-xorg-module-dir=%_x11modulesdir \
	--disable-static
%make_build
popd

pushd dri
%autoreconf
CFLAGS+=" -I../../exa/src/vivante_gal -I../../exa/src/vivante_util " \
./configure \
	--with-xorg-module-dir=%_x11modulesdir \
	--disable-static
%make_build
popd

%install
pushd exa
%make DESTDIR=%buildroot install
popd

pushd dri
%make DESTDIR=%buildroot install
mkdir -p %buildroot/%_x11modulesdir/dri/
mv %buildroot/%_x11modulesdir/extensions/libdri.so %buildroot/%_x11modulesdir/dri/vivante_dri.so 
popd

%files
%_x11modulesdir/drivers/*.so

%files -n xorg-dri-vivante
%_x11modulesdir/dri/*.so

%set_verify_elf_method no

%changelog
* Sun Mar 24 2013 Pavel Nakonechny <pavel.nakonechny@skitlab.ru> 12.09.01-alt1
- initial build
