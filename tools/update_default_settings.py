#!/usr/bin/env python3
#
# Copyright (C) 2017 ~ 2017 Deepin Technology Co., Ltd.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# Generate default settings for loongson and sw platforms.
# Execute this program after updating options in resources/default_settings.ini.

import configparser
import os
import shutil
import sys

SEC_NAME = "General"
def update_loongson_policy(settings_file):
    settings = (
            ("skip_virtual_machine_page", "true"),
            ("skip_select_language_page", "true"),
            ("skip_timezone_page", "true"),
            ("select_language_default_locale", "zh_CN"),
            ("system_info_disable_keyboard_page", "true"),
            ("system_info_default_keyboard_layout", '"us"'),
            ("timezone_default", '"Asia/Shanghai"'),
            ("timezone_use_regdomain", "false"),
            ("partition_skip_simple_partition_page", "true"),
            ("partition_enable_simple_disk_page", "true"),
            ("partition_enable_swap_file", "false"),
            ("partition_formatted_mount_points", '"/;/tmp;/var"'),
            ("partition_enable_os_prober", "false"),
            ("partition_boot_on_first_partition", "true"),
            ("partition_supported_fs", '"ext4;ext3;ext2;efi;linux-swap"'),
            ("partition_prefer_logical_partition", "false"),
    )
    parser = configparser.ConfigParser()
    parser.read(settings_file)
    for key, value in settings:
        parser.set(SEC_NAME, key, value)
    with open(settings_file, "w") as fh:
        parser.write(fh)

def update_sw_policy(settings_file):
    settings = (
        ("skip_virtual_machine_page", "true"),
        ("skip_select_language_page", "true"),
        ("skip_timezone_page", "true"),
        ("select_language_default_locale", "zh_CN"),
        ("system_info_disable_keyboard_page", "true"),
        ("system_info_default_keyboard_layout", '"us"'),
        ("timezone_default", '"Asia/Shanghai"'),
        ("timezone_use_regdomain", "false"),
        ("partition_enable_swap_file", "false"),
        ("partition_swap_partition_size", "8192"),
        ("partition_skip_simple_partition_page", "true"),
        ("partition_enable_simple_disk_page", "true"),
        ("partition_enable_swap_file", "false"),
        ("partition_formatted_mount_points", '"/;/tmp;/var"'),
        ("partition_enable_os_prober", "false"),
        ("partition_boot_on_first_partition", "true"),
        ("partition_supported_fs", '"ext4;ext3;ext2;efi;linux-swap"'),
        ("partition_prefer_logical_partition", "false"),
    )
    parser = configparser.ConfigParser()
    parser.read(settings_file)
    for key, value in settings:
        parser.set(SEC_NAME, key, value)
    with open(settings_file, "w") as fh:
        parser.write(fh)

def main():
    src_settings = "resources/default_settings.ini"
    loongson_settings = "resources/loongson_default_settings.ini"
    sw_settings = "resources/sw_default_settings.ini"
    if not os.path.exists(src_settings):
        print("Failed to find", src_settings)
        sys.exit(1)
    shutil.copy(src_settings, loongson_settings)
    shutil.copy(src_settings, sw_settings)

    update_loongson_policy(loongson_settings)
    update_sw_policy(sw_settings)

if __name__ == "__main__":
    main()
