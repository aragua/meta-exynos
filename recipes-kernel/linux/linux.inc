DESCRIPTION = "Linux Kernel"
SECTION = "kernel"
LICENSE = "GPLv2"

LIC_FILES_CHKSUM = "file://COPYING;md5=d7810fab7487fb0aad327b76f1be7cd7"

inherit kernel siteinfo

# Specify the commandline for your device

# Set the verbosity of kernel messages during runtime
# You can define CMDLINE_DEBUG in your local.conf or distro.conf to override this behaviour  
CMDLINE_DEBUG ?= '${@base_conditional("DISTRO_TYPE", "release", "quiet", "debug", d)}'
CMDLINE_append = " ${CMDLINE_DEBUG} "

# Kernel bootlogo is distro-specific (default is OE logo).
# Logo resolution (qvga, vga, ...) is machine-specific.
LOGO_SIZE ?= "."

ALLOW_EMPTY_kernel-devicetree = "1"

python __anonymous () {

    import bb
    
    devicetree = bb.data.getVar('KERNEL_DEVICETREE', d, 1) or ''
    if devicetree:
    	depends = bb.data.getVar("DEPENDS", d, 1)
    	bb.data.setVar("DEPENDS", "%s dtc-native" % depends, d)
}

do_configure_prepend() {

        # Rename getline in ./scripts/unifdef.c
        # Kernels up to 2.6.29 are currently failing to build unifdef.c,
        # clashing with exposed getline() from <stdio.h>
        # see https://patchwork.kernel.org/patch/11166/
        # committed in 2.6.29 (commit d15bd1067b1fcb2b7250d22bc0c7c7fea0b759f7)

        sed -i -e 's/getline/parseline/g' ${S}/scripts/unifdef.c


        echo "" > ${S}/.config

        #
        # logo support, if you supply logo_linux_clut224.ppm in SRC_URI, then it's going to be used
        #
        if [ -e ${WORKDIR}/${LOGO_SIZE}/logo_linux_clut224.ppm ]; then
                install -m 0644 ${WORKDIR}/${LOGO_SIZE}/logo_linux_clut224.ppm drivers/video/logo/logo_linux_clut224.ppm
                echo "CONFIG_LOGO=y"                    >> ${S}/.config
                echo "CONFIG_LOGO_LINUX_CLUT224=y"      >> ${S}/.config
        fi

	#
	# endian support
	#
        if [ "${SITEINFO_ENDIANNESS}" = "be" ]; then
                echo "CONFIG_CPU_BIG_ENDIAN=y"          >> ${S}/.config
        fi

        echo "CONFIG_CMDLINE=\"${CMDLINE}\"" >> ${S}/.config

        sed -e '/CONFIG_CMDLINE=/d' \
            -e '/CONFIG_CPU_BIG_ENDIAN/d' \
            -e '/CONFIG_LOGO=/d' \
            -e '/CONFIG_LOGO_LINUX_CLUT224=/d' \
            -e '/CONFIG_LOCALVERSION/d' \
            -e '/CONFIG_LOCALVERSION_AUTO/d' \
	    < '${WORKDIR}/defconfig' >>'${S}/.config'

        echo 'CONFIG_LOCALVERSION=""' >>${S}/.config
        echo '# CONFIG_LOCALVERSION_AUTO is not set' >>${S}/.config

        #
        # Udev quirks
        #

        # Newer versions of udev mandate that sysfs doesn't have deprecated entries
            sed -e /CONFIG_SYSFS_DEPRECATED/d \
                -e /CONFIG_SYSFS_DEPRECATED_V2/d \
                -e /CONFIG_HOTPLUG/d \
                -e /CONFIG_UEVENT_HELPER_PATH/d \
                -e /CONFIG_UNIX/d \
                -e /CONFIG_SYSFS/d \
                -e /CONFIG_PROC_FS/d \
                -e /CONFIG_TMPFS/d \
                -e /CONFIG_INOTIFY_USER/d \
                -e /CONFIG_SIGNALFD/d \
                -e /CONFIG_TMPFS_POSIX_ACL/d \
                -e /CONFIG_BLK_DEV_BSG/d \
                -i '${S}/.config'

            echo '# CONFIG_SYSFS_DEPRECATED is not set' >> ${S}/.config
            echo '# CONFIG_SYSFS_DEPRECATED_V2 is not set' >> ${S}/.config
            echo 'CONFIG_HOTPLUG=y' >> ${S}/.config
            echo 'CONFIG_UEVENT_HELPER_PATH=""' >> ${S}/.config
            echo 'CONFIG_UNIX=y' >> ${S}/.config
            echo 'CONFIG_SYSFS=y' >> ${S}/.config
            echo 'CONFIG_PROC_FS=y' >> ${S}/.config
            echo 'CONFIG_TMPFS=y' >> ${S}/.config
            echo 'CONFIG_INOTIFY_USER=y' >> ${S}/.config
            echo 'CONFIG_SIGNALFD=y' >> ${S}/.config
            echo 'CONFIG_TMPFS_POSIX_ACL=y' >> ${S}/.config
            echo 'CONFIG_BLK_DEV_BSG=y' >> ${S}/.config
            echo 'CONFIG_DEVTMPFS=y' >> ${S}/.config
            echo 'CONFIG_DEVTMPFS_MOUNT=y' >> ${S}/.config

        # Newer inits like systemd need cgroup support
            sed -e /CONFIG_CGROUP_SCHED/d \
                -e /CONFIG_CGROUPS/d \
                -i '${S}/.config'
            
            echo 'CONFIG_CGROUP_SCHED=y' >> ${S}/.config
            echo 'CONFIG_CGROUPS=y' >> ${S}/.config
            echo 'CONFIG_CGROUP_NS=y' >> ${S}/.config
            echo 'CONFIG_CGROUP_FREEZER=y' >> ${S}/.config
            echo 'CONFIG_CGROUP_DEVICE=y' >> ${S}/.config
            echo 'CONFIG_CPUSETS=y' >> ${S}/.config
            echo 'CONFIG_PROC_PID_CPUSET=y' >> ${S}/.config
            echo 'CONFIG_CGROUP_CPUACCT=y' >> ${S}/.config
            echo 'CONFIG_RESOURCE_COUNTERS=y' >> ${S}/.config

        #
        # root-over-nfs-over-usb-eth support. Limited, but should cover some cases.
        # Enable this by setting a proper CMDLINE_NFSROOT_USB.
        #
        if [ ! -z "${CMDLINE_NFSROOT_USB}" ]; then
                oenote "Configuring the kernel for root-over-nfs-over-usb-eth with CMDLINE ${CMDLINE_NFSROOT_USB}"
                sed -e '/CONFIG_INET/d' \
                    -e '/CONFIG_IP_PNP=/d' \
                    -e '/CONFIG_USB_GADGET=/d' \
                    -e '/CONFIG_USB_GADGET_SELECTED=/d' \
                    -e '/CONFIG_USB_ETH=/d' \
                    -e '/CONFIG_NFS_FS=/d' \
                    -e '/CONFIG_ROOT_NFS=/d' \
                    -e '/CONFIG_CMDLINE=/d' \
                    -i ${S}/.config
                echo "CONFIG_INET=y"                     >> ${S}/.config
                echo "CONFIG_IP_PNP=y"                   >> ${S}/.config
                echo "CONFIG_USB_GADGET=y"               >> ${S}/.config
                echo "CONFIG_USB_GADGET_SELECTED=y"      >> ${S}/.config
                echo "CONFIG_USB_ETH=y"                  >> ${S}/.config
                echo "CONFIG_NFS_FS=y"                   >> ${S}/.config
                echo "CONFIG_ROOT_NFS=y"                 >> ${S}/.config
                echo "CONFIG_CMDLINE=\"${CMDLINE_NFSROOT_USB}\"" >> ${S}/.config
        fi

        echo 'CONFIG_FHANDLE=y' >> ${S}/.config

	yes '' | oe_runmake oldconfig
}

do_configure_append() {
        if test -e scripts/Makefile.fwinst ; then
		sed -i -e "s:-m0644:-m 0644:g" scripts/Makefile.fwinst
	fi
}

# bitbake.conf only prepends PARALLEL make in tasks called do_compile, which isn't the case for compile_modules
# So explicitly enable it for that in here
EXTRA_OEMAKE = "${PARALLEL_MAKE} "

do_install_append() {
	oe_runmake headers_install INSTALL_HDR_PATH=${D}${exec_prefix}/src/linux-${KERNEL_VERSION} ARCH=$ARCH
	oe_runmake firmware_install INSTALL_MOD_PATH=${D} ARCH=$ARCH
	install -d ${D}/boot
	make dtbs || true
	install -m 0644 ${S}/arch/arm/boot/dts/*.dtb ${D}/boot || true
}

PACKAGES =+ "kernel-headers"
FILES_kernel-headers = "${exec_prefix}/src/linux*"

require recipes-kernel/linux/linux-dtb.inc

FILES_kernel-devicetree += "/boot/*.dtb"

RDEPENDS_kernel-image_append = " kernel-devicetree"

# Automatically depend on lzop/lz4-native if CONFIG_KERNEL_LZO/LZ4 is enabled
python () {
    try:
        defconfig = bb.fetch2.localpath('file://defconfig', d)
    except bb.fetch2.FetchError:
        return

    try:
        configfile = open(defconfig)
    except IOError:
        return

    if 'CONFIG_KERNEL_LZO=y\n' in configfile.readlines():
        depends = d.getVar('DEPENDS', False)
        d.setVar('DEPENDS', depends + ' lzop-native')

    if 'CONFIG_KERNEL_LZ4=y\n' in configfile.readlines():
        depends = d.getVar('DEPENDS', False)
        d.setVar('DEPENDS', depends + ' lz4-native')
}
