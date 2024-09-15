#!/usr/bin/env python3
import platform
import argparse
import yaml


qtc_deb_url_fmt = "https://download.qt.io/{release_type}_releases/qtcreator/{qtcv_maj}/{qtcv_full}/cpack_experimental/qtcreator-opensource-linux-{arch}-{qtcv_full}.deb"

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("-v", '--version', type=str, default=None)
    args = parser.parse_args()

    if args.version is None:
        # read version from file
        with open("versions.yaml", 'r') as file:
            versions = yaml.safe_load(file)
            qtc_version = versions['qtc_version']
            qtc_dev_tag = versions.get('qtc_dev_tag', str())
    else:
        # parse version from command line argument
        vv = args.version.split('-')
        qtc_version = vv[0]
        qtc_dev_tag = vv[1] if len(vv) > 1 else str()

    release = not qtc_dev_tag

    if not release and (qtc_dev_tag.find("beta") == -1 and qtc_dev_tag.find("rc") == -1):
        raise RuntimeWarning(f"Invalid development tag '{qtc_dev_tag}'. Valid tags contain 'beta' or 'rc'.")

    ver_split = qtc_version.split('.')
    qtc_ver_major = ver_split[0]
    qtc_ver_minor = ver_split[1] if len(ver_split)>1 else 0
    qtc_ver_patch = ver_split[2] if len(ver_split)>2 else 0
    qtc_ver_maj = f"{qtc_ver_major}.{qtc_ver_minor}"
    qtc_ver_full = f"{qtc_ver_maj}.{qtc_ver_patch}"

    if not release:
        qtc_ver_full += f"-{qtc_dev_tag}"

    arch = platform.machine()

    deb_url = qtc_deb_url_fmt.format(release_type = "official" if release else "development",
                                     qtcv_maj = qtc_ver_maj,
                                     qtcv_full = qtc_ver_full,
                                     arch = arch,)

    print(deb_url)
