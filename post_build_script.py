import os
import shutil
import hashlib
import re


def calculate_md5(file_path):
    md5 = hashlib.md5()
    with open(file_path, 'rb') as f:
        while chunk := f.read(4096):
            md5.update(chunk)
    return md5.hexdigest()


def read_version_from_config(project_dir):
    version_files = [
        os.path.join(project_dir, 'src', 'version.h'),
        os.path.join(project_dir, 'src', 'config.h'),
    ]
    version_pattern = re.compile(r'#define\s+APP_VERSION\s+"(\d+)"')

    for version_path in version_files:
        try:
            with open(version_path, 'r', encoding='utf-8') as f:
                for line in f:
                    match = version_pattern.search(line)
                    if match:
                        return match.group(1)
        except FileNotFoundError:
            continue
        except Exception as e:
            print(f'Error reading {version_path}: {e}')

    return None


def post_build(source, target, env):
    try:
        build_dir = env.subst('$BUILD_DIR')
        project_dir = env.subst('$PROJECT_DIR')
        firmware_path = os.path.join(build_dir, 'firmware.bin')
        server_dir = os.path.join(project_dir, 'server_files')

        if not os.path.exists(server_dir):
            os.makedirs(server_dir)
            print(f'Created directory: {server_dir}')

        firmware_dest = os.path.join(server_dir, 'firmware.bin')
        if os.path.exists(firmware_path):
            shutil.copy(firmware_path, firmware_dest)
            print(f'Firmware copied to: {firmware_dest}')
        else:
            print(f'Error: firmware file not found: {firmware_path}')
            return

        version = read_version_from_config(project_dir)
        if not version:
            print('Error: APP_VERSION not found in src/version.h or src/config.h')
            return

        version_file = os.path.join(server_dir, 'firmware.txt')
        with open(version_file, 'w', encoding='utf-8') as f:
            f.write(version)
        print(f'Version file written: {version_file}')

        fw_hash = calculate_md5(firmware_dest)
        hash_file = os.path.join(server_dir, 'firmware.md5')
        with open(hash_file, 'w', encoding='utf-8') as f:
            f.write(fw_hash)
        print(f'MD5 file written: {hash_file}')

        print(f'Post-build artifacts updated in: {server_dir}')
    except Exception as e:
        print(f'Post-build script failed: {e}')


Import('env')
env.AddPostAction('buildprog', post_build)
