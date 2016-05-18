############################################################
# <bsn.cl fy=2013 v=onl>
#
#        Copyright 2013, 2014 Big Switch Networks, Inc.
#
# Licensed under the Eclipse Public License, Version 1.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#        http://www.eclipse.org/legal/epl-v10.html
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
# either express or implied. See the License for the specific
# language governing permissions and limitations under the
# License.
#
# </bsn.cl>
############################################################

# The loader is installed in the fat partition of the first USB storage device
platform_bootcmd="usb start; ext2load usb 0:1 0x10000000 $ONL_PLATFORM.itb; setenv bootargs console=\$consoledev,\$baudrate onl_platform=$ONL_PLATFORM; bootm 0x10000000#$ONL_PLATFORM"

platform_installer() {
    # Standard installation to usb storage
    installer_standard_blockdev_install sda 128M 128M 768M ""
}
