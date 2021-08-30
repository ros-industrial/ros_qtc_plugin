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

url_repo_qtc_fmt = "https://download.qt.io/{release_type}_releases/qtcreator/{qtcv_maj}/{qtcv_full}/installer_source/{os}_{arch}/"

url_repo_qt_fmt = "https://download.qt.io/online/qtsdkrepository/{os}_{arch}/desktop/qt{ver_maj}_{ver_concat}/"

os_map = {"Linux": "linux",
          "Windows": "windows",
          "Darwin": "mac"}

arch_map = {"i386": "x86",
            "i686": "x86",
            "x86": "x86",
            "x86_64": "x64",
            "AMD64": "x64"}

os_compiler = {"Linux": "gcc",
               "Windows": "msvc2019",
               "Darwin": "clang"}

arch_bits = {"x86": "32",
             "x64": "64"}

def download_check_fail(url, expected_type):
    response = requests.get(url, allow_redirects=True)
    if not response.ok:
        raise RuntimeError("error retrieving "+response.url)
    if response.headers.get('content-type') != expected_type:
        raise RuntimeError("invalid format of "+response.url)
    return response

def qtc_download_check_extract(cfg, dir_install):
    # if the Qt Creator version contains '-beta' or '-rc' we have to
    # use the "development" repo, otherwise use the "official" repo
    qtc_ver = cfg['versions']['qtc_version']
    if qtc_ver.find("-beta") > 0 or qtc_ver.find("-rc") > 0:
        release = "development"
        qtc_ver_nr, qtc_ver_type = qtc_ver.split('-')
    else:
        release = "official"
        qtc_ver_nr = qtc_ver
        qtc_ver_type = None

    ver_split = qtc_ver_nr.split('.')
    qtc_ver_maj = "{}.{}".format(ver_split[0], ver_split[1])
    qtc_ver_full = "{}.{}.{}".format(ver_split[0], ver_split[1], 0)
    if qtc_ver_type:
        qtc_ver_full = "{ver}-{type}".format(ver = qtc_ver_full, type = qtc_ver_type)

    base_url = url_repo_qtc_fmt.format(release_type = release,
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

        print("download", url_archive)

        content = download_check_fail(url_archive, "application/x-7z-compressed").content

        if md5sums[archive_name] != hashlib.md5(content).hexdigest():
            raise RuntimeError(archive_name+" MD5 hash sum does not match")

        py7zr.SevenZipFile(io.BytesIO(content)).extractall(dir_install_qt)

    if cfg['os'] == "Darwin":
        dir_install_qt = os.path.join(dir_install_qt, "Qt Creator.app", "Contents", "Resources")

    return dir_install_qt

def qt_download_check_extract(cfg, dir_install):
    sys_os = os_map[cfg['os']]
    sys_arch = arch_map[cfg['arch']]

    # the windows repo stores 32bit and 64bit binaries under the same 32bit directory
    if cfg['os'] == "Windows":
        url_arch = "x86"
    else:
        url_arch = sys_arch

    qt_ver = cfg['versions']['qt_version']
    ver_maj, ver_min = qt_ver.split('.')
    ver_concat = "{}{}0".format(ver_maj, ver_min)

    base_url = url_repo_qt_fmt.format(
                        os = sys_os, arch = url_arch,
                        ver_maj = ver_maj,
                        ver_concat = ver_concat)

    # fetch meta data
    r = download_check_fail(base_url+"/Updates.xml", "application/xml")

    metadata = ElementTree.fromstring(r.text)

    compiler_bits = os_compiler[cfg['os']]+"_"+arch_bits[sys_arch]

    if cfg['os'] == "Windows":
        compiler = "win"+arch_bits[sys_arch]+"_"+compiler_bits
    else:
        compiler = compiler_bits

    base_package_name = "qt.qt{ver_maj}.{ver_concat}.{compiler}".format(
        ver_maj = ver_maj, ver_concat = ver_concat,
        compiler = compiler)

    base_package_version = None
    base_package_archives = None
    for package in metadata.iter("PackageUpdate"):
        if package.find("Name").text == base_package_name:
            base_package_version = package.find("Version").text
            archives = package.find("DownloadableArchives").text
            base_package_archives = list(
                csv.reader(io.StringIO(archives),
                           delimiter=',', skipinitialspace=True))[0]

    archives_match = []
    for module_name in cfg['versions']['qt_modules']:
        for archive_name in base_package_archives:
            if archive_name.startswith(module_name):
                archives_match.append(archive_name)

    for archive_name in archives_match:
        url_archive = base_url+'/'+base_package_name+'/'+base_package_version+archive_name

        print("download", url_archive)

        content = download_check_fail(url_archive, "application/x-7z-compressed").content

        sha1sum = download_check_fail(url_archive+".sha1", "application/x-7z-compressed").text

        if sha1sum != hashlib.sha1(content).hexdigest():
            raise RuntimeError(archive_name+" SHA1 hash sum does not match")

        py7zr.SevenZipFile(io.BytesIO(content)).extractall(dir_install)

    return os.path.join(dir_install, "{}.{}.0".format(ver_maj, ver_min), compiler_bits)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--install_path', type=str, default=None)
    parser.add_argument('--export_variables', action="store_true")
    args = parser.parse_args()

    cfg = {}

    cfg['os'] = platform.system()
    cfg['arch'] = platform.machine()

    with open("versions.yaml", 'r') as file:
        cfg['versions'] = yaml.safe_load(file)

    dir_install = args.install_path
    if not dir_install:
        dir_install = os.path.join(tempfile.gettempdir(), "qtc_sdk")

    os.makedirs(dir_install, exist_ok=True)

    prefix_paths = []
    dir_qtc = qtc_download_check_extract(cfg, dir_install)
    prefix_paths.append(dir_qtc)

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
        with open("env", 'w') as f:
            f.write("QTC_PREFIX_PATH=\"{}\"\n".format(prefix_paths))
