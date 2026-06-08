#!/usr/bin/env python3
import argparse
import csv
import hashlib
import io
import os
import platform
import tempfile
from xml.etree import ElementTree

import py7zr
import requests
import yaml
try:
    from tqdm_loggable.auto import tqdm
except ModuleNotFoundError:
    from tqdm import tqdm


url_repo_qtc_fmt = "https://download.qt.io/{release_type}_releases/qtcreator/{qtcv_maj}/{qtcv_full}/installer_source/{os}_{arch}/"

url_repo_qt_fmt = "https://download.qt.io/online/qtsdkrepository/{os}_{arch}/desktop/qt{ver_maj}_{ver_concat}/qt{ver_maj}_{qt_arch}/"

os_map = {
    "Linux": "linux",
    "Windows": "windows",
    "Darwin": "mac",
}

arch_map = {
    "i386": "x86",
    "i686": "x86",
    "x86": "x86",
    "x86_64": "x64",
    "x64": "x64",
    "AMD64": "x64",
    "ARM64": "arm64",
    "aarch64": "arm64",
}

os_arch_toolchain = {
    "linux": {
        "x64": "linux_gcc_64",
        "arm64": "linux_gcc_arm64",
    },
    "windows": {
        "x64": "win64_msvc2022_64",
        "arm64": "win64_msvc2022_arm64",
    },
    "mac": {
        "x64": "clang_64",
    },
}

qt_toolchain_suffix = {
    "windows": {
        "x64": "msvc2022_64",
    },
}

def download_check_fail(url, expected_type):
    print("download URL:", url, flush=True)
    response = requests.get(url, allow_redirects=True, timeout=1800)
    if not response.ok:
        raise RuntimeError("error retrieving "+response.url)
    if response.headers.get('content-type') != expected_type:
        print(f"Warning: invalid content type, expected '{expected_type}', got '{response.headers.get('content-type')}'", flush=True)
    return response

def read_downloadable_archives(package):
    archive_names = io.StringIO(package.find("DownloadableArchives").text)
    return list(csv.reader(archive_names, delimiter=',', skipinitialspace=True))[0]

def extract_progress(archive_bytes, archive_name, destination_path):
    class ExtractProgressBar(py7zr.callbacks.ExtractCallback, tqdm):
        def __init__(self, *args, total_bytes, **kwargs):
            super().__init__(self, *args, total=total_bytes, **kwargs)
        def report_start_preparation(self):
            pass
        def report_start(self, processing_file_path, processing_bytes):
            pass
        def report_update(self, decompressed_bytes):
            pass
        def report_end(self, processing_file_path, wrote_bytes):
            self.update(int(wrote_bytes))
        def report_postprocess(self):
            self.update(int(self.total))
        def report_warning(self, message):
            pass

    with py7zr.SevenZipFile(io.BytesIO(archive_bytes)) as zf:
        with ExtractProgressBar(unit='B', unit_scale=True, miniters=1,
                                total_bytes=sum([f.uncompressed for f in zf.files]),
                                desc=archive_name) as cb_progress:
            zf.extractall(path=destination_path, callback=cb_progress)

def qtc_download_check_extract(cfg, dir_install):
    qtc_ver = cfg['versions']['qtc_version']
    qtc_dev_tag = cfg['versions'].get('qtc_dev_tag', str())
    release = not qtc_dev_tag

    # If no 'qtc_dev_tag' is provided, or it is empty, we use a released version from the "official" repo.
    # Otherwise, if the 'qtc_dev_tag' contains 'beta' or 'rc' we have to use the "development" repo.

    if not release and (qtc_dev_tag.find("beta") == -1 and qtc_dev_tag.find("rc") == -1):
        raise RuntimeWarning(f"Invalid development tag '{qtc_dev_tag}'. Valid tags contain 'beta' or 'rc'.")

    ver_split = qtc_ver.split('.')
    qtc_ver_major = ver_split[0]
    qtc_ver_minor = ver_split[1] if len(ver_split)>1 else 0
    qtc_ver_patch = ver_split[2] if len(ver_split)>2 else 0
    qtc_ver_maj = f"{qtc_ver_major}.{qtc_ver_minor}"
    qtc_ver_full = f"{qtc_ver_maj}.{qtc_ver_patch}"
    if not release:
        qtc_ver_full = f"{qtc_ver_full}-{qtc_dev_tag}"

    base_url = url_repo_qtc_fmt.format(release_type = "official" if release else "development",
                                       qtcv_maj = qtc_ver_maj,
                                       qtcv_full = qtc_ver_full,
                                       os = os_map[cfg['os']],
                                       arch = arch_map[cfg['arch']])

    archive_names = [qtc_module+".7z" for qtc_module in cfg['versions']['qtc_modules']]

    dir_install_qt = os.path.join(dir_install, "Tools", "QtCreator")

    md5sums_raw = download_check_fail(base_url+"/md5sums.txt", "text/plain").text

    md5sums = {}
    for h,f in csv.reader(io.StringIO(md5sums_raw), delimiter=' ', skipinitialspace=True):
        md5sums[f] = h

    for archive_name in archive_names:
        url_archive = base_url+"/"+archive_name

        content = download_check_fail(url_archive, "application/x-7z-compressed").content

        if md5sums[archive_name] != hashlib.md5(content).hexdigest():
            raise RuntimeError(archive_name+" MD5 hash sum does not match")

        extract_progress(content, archive_name, dir_install_qt)

    if cfg['os'] == "Darwin":
        dir_install_qt = os.path.join(dir_install_qt, "Qt Creator.sdk")

    return dir_install_qt

def qt_download_check_extract(cfg, dir_install):
    sys_os = os_map[cfg['os']]
    sys_arch = arch_map[cfg['arch']]

    # the windows repo stores 32bit and 64bit binaries under the same 32bit directory
    if cfg['os'] == "Windows" and cfg['arch'] == "AMD64":
        url_arch = "x86"
    else:
        url_arch = sys_arch

    qt_ver = cfg['versions']['qt_version']
    qt_semver = qt_ver.split('.')
    ver_maj = qt_semver[0]
    ver_min = qt_semver[1]
    if len(qt_semver) == 3:
        ver_patch = qt_semver[2]
    else:
        ver_patch = 0
    ver_concat = f"{ver_maj}{ver_min}{ver_patch}"

    # OSes will store Qt version with a toolchain suffix
    if sys_os in qt_toolchain_suffix and sys_arch in qt_toolchain_suffix[sys_os]:
        sfx = qt_toolchain_suffix[sys_os][sys_arch]
        qt_arch = f"{ver_concat}_{sfx}"
    else:
        qt_arch = ver_concat

    base_url = url_repo_qt_fmt.format(
                        os = sys_os, arch = url_arch,
                        ver_maj = ver_maj,
                        ver_concat = ver_concat,
                        qt_arch = qt_arch,
    )

    # fetch meta data
    r = download_check_fail(base_url+"/Updates.xml", "application/xml")

    metadata = ElementTree.fromstring(r.text)

    toolchain = os_arch_toolchain[sys_os][sys_arch]

    base_package_name = f"qt.qt{ver_maj}.{ver_concat}.{toolchain}"

    extra_package_names = list()
    for module in cfg['versions']['qt_modules']:
        extra_package_names.append(f"qt.qt{ver_maj}.{ver_concat}.addons.{module}.{toolchain}")

    package_archives = dict()
    archive_target_dir = dict()
    for package in metadata.iter("PackageUpdate"):
        if package.find("Name").text in [base_package_name] + extra_package_names:
            package_archives[package.find("Name").text] = {
                "version": package.find("Version").text,
                "archives": read_downloadable_archives(package)
                }
        for op in package.iter("Operation"):
            if op.get("name") == "Extract":
                op_args = [op_arg.text for op_arg in op.iter("Argument")]
                archive_target_dir[op_args[1]] = op_args[0]

    archives_match = dict()
    for module_name in cfg['versions']['qt_modules']:
        found = False
        for package_name, data in package_archives.items():
            for archive_name in data["archives"]:
                if archive_name.startswith(module_name):
                    archives_match[module_name] = [package_name, data["version"], archive_name]
                    found = True
        if not found:
            print(f"no archive for Qt module '{module_name}' found")

    if not archives_match:
        raise RuntimeError(f"no matches for Qt modules ({cfg['versions']['qt_modules']}) found")

    dir_install = os.path.join(dir_install, "Qt")

    for package_name, package_version, archive_name in archives_match.values():
        url_archive = base_url+'/'+package_name+'/'+package_version+archive_name

        content = download_check_fail(url_archive, "application/x-7z-compressed").content

        sha1sum = download_check_fail(url_archive+".sha1", "application/x-7z-compressed").text

        if sha1sum != hashlib.sha1(content).hexdigest():
            raise RuntimeError(archive_name+" SHA1 hash sum does not match")

        if archive_name in archive_target_dir:
            archive_dir_install = archive_target_dir[archive_name].replace("@TargetDir@", dir_install)
        else:
            archive_dir_install = dir_install

        extract_progress(content, archive_name, archive_dir_install)

    qt_path = os.path.join(dir_install, f"{ver_maj}.{ver_min}.{ver_patch}")
    qt_archs = os.listdir(qt_path)
    if len(qt_archs) > 1:
        raise RuntimeWarning(f"more than one architecture found in {qt_path}, will use first: {qt_archs[0]}")
    return os.path.join(qt_path, qt_archs[0])


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--install_path', type=str, default=None)
    parser.add_argument('--export_variables', action="store_true")
    parser.add_argument('--skip-qtc', action="store_true")
    parser.add_argument('--skip-qt', action="store_true")
    args = parser.parse_args()

    print(f"system: {platform.system()}, machine: {platform.machine()}", flush=True)

    cfg = {}

    cfg['os'] = platform.system()
    cfg['arch'] = platform.machine()

    # macOS uses a universal binary that stores all architectures in the "x64" folder
    if cfg['os'] == "Darwin":
        cfg['arch'] = "x64"

    with open("versions.yaml", 'r', encoding="utf-8") as file:
        cfg['versions'] = yaml.safe_load(file)

    dir_install = args.install_path
    if not dir_install:
        dir_install = os.path.join(tempfile.gettempdir())

    dir_install = os.path.join(dir_install, "qtc-sdk")

    os.makedirs(dir_install, exist_ok=True)

    prefix_paths = []
    dir_qtc = None
    dir_qt = None

    if not args.skip_qtc:
        dir_qtc = qtc_download_check_extract(cfg, dir_install)
        prefix_paths.append(dir_qtc)

    if not args.skip_qt:
        dir_qt = qt_download_check_extract(cfg, dir_install)
        prefix_paths.append(dir_qt)

    cmd_setup = "cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=\"{prefix_paths}\""
    cmd_compile = "cmake --build build --target package"

    prefix_paths = ';'.join(prefix_paths)

    print("all dependencies have been extracted to", dir_install)
    print("to build the plugin:")
    print("\t" + cmd_setup.format(prefix_paths = prefix_paths))
    print("\t" + cmd_compile)

    if args.export_variables:
        with open("env", 'w', encoding="utf-8") as f:
            if dir_qtc: f.write(f"QTC_PATH={dir_qtc}\n")
            if dir_qt: f.write(f"QT_PATH={dir_qt}\n")
