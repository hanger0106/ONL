############################################################
#
# Upgrade Base Classes
#
############################################################
import logging
import logging.handlers
import platform as pp
import subprocess
import os
import sys
import shutil
import json
import string
import argparse
import yaml
from time import sleep
from onl.platform.current import OnlPlatform

class BaseUpgrade(object):

    # Customized by deriving class
    name = None
    Name = None
    atype = None
    current_version_key = None
    next_version_key = None
    def __init__(self):

        if not (self.name and self.Name and self.atype and self.title and
                self.current_version_key and self.next_version_key):
            raise Exception("Name descriptors must be provided by deriving class.")

        self.init_logger()
        self.init_argparser()
        self.load_config()
        self.arch = pp.machine()
        self.parch = dict(ppc='powerpc', x86_64='amd64', armv7l='armel')[self.arch]
        self.platform = OnlPlatform()
        self.init()

    def init(self):
        pass

    def load_config(self):
        pass


    def init_logger(self):
        fmt = '%(asctime)s.%(msecs)d %(levelname)s %(name)s: %(message)s'
        datefmt="%Y-%m-%d %H:%M:%S"
        formatter = logging.Formatter(fmt, datefmt)

        logging.basicConfig(format=fmt, datefmt=datefmt)

        self.logger = logging.getLogger(string.rjust("%s-upgrade" % self.name, 16))
        self.logger.setLevel(logging.INFO)
        if os.getenv("DEBUG"):
            self.logger.setLevel(logging.DEBUG)


    def auto_upgrade_default(self):
        return "advisory"

    def init_argparser(self):
        self.ap = argparse.ArgumentParser("%s-upgrade" % self.name)
        self.ap.add_argument("--enable", action='store_true', help="Enable updates.")
        self.ap.add_argument("--force", action='store_true', help="Force update.")
        self.ap.add_argument("--no-reboot", action='store_true', help="Don't reboot.")
        self.ap.add_argument("--check", action='store_true', help="Check only.")
        self.ap.add_argument("--auto-upgrade", help="Override auto-upgrade mode.", default=self.auto_upgrade_default())
        self.ap.add_argument("--summarize", action='store_true', help="Summarize only, no upgrades.")

    def banner(self):
        self.logger.info("************************************************************")
        self.logger.info("* %s" % self.title)
        self.logger.info("************************************************************")
        self.logger.info("")

    def finish(self, message=None, rc=0):
        if message:
            self.logger.info("")
            if rc == 0:
                self.logger.info(message)
            else:
                self.logger.error(message)
        self.logger.info("")
        self.logger.info("************************************************************")
        self.update_upgrade_status(self.current_version_key, self.current_version)
        self.update_upgrade_status(self.next_version_key, self.next_version)
        # Flush stdout
        sleep(.1)
        sys.exit(rc)

    def abort(self, message=None, rc=1):
        if message:
            message = "Error: %s" % message
        self.finish(message, rc)

    def fw_getenv(self, var):
        FW_PRINTENV="/usr/bin/fw_printenv"
        if os.path.exists(FW_PRINTENV):
            try:
                if var:
                    return subprocess.check_output("%s -n %s" % FW_PRINTENV, stderr=subprocess.STDOUT);
                else:
                    variables = {}
                    for v in subprocess.check_output("/usr/bin/fw_printenv", shell=True).split('\n'):
                        (name, eq, value) = v.partition('=')
                        variables[name] = value
                    return variables
            except Exception, e:
                return None
        else:
            return None

    def fw_setenv(self, var, value):
        FW_SETENV="/usr/bin/fw_setenv"
        if os.system("%s %s %s" % (FW_SETENV, var, value)) != 0:
            self.abort("Error setting environment variable %s=%s. Upgrade cannot continue." % (var, value))

    def copyfile(self, src, dst):
        self.logger.info("Installing %s -> %s..." % (src, dst))
        if not os.path.exists(src):
            self.abort("Source file '%s' does not exist." % src)

        try:
            shutil.copyfile(src, dst)
        except Exception, e:
            self.abort("Exception while copying: %s" % e)

    def reboot(self):
        if self.ops.no_reboot:
            self.finish("[ No Reboot ]")
        elif self.ops.check:
            self.finish("[ Check Abort ]")
        else:
            self.logger.info("The system will reboot to complete the update.")
            sleep(1)
            os.system("sync")
            sleep(1)
            while True:
                os.system("reboot -f")
                sleep(30)
                self.abort("The system did not reboot as expected.")


    def load_json(self, fname, key=None, default=None):
        if os.path.exists(fname):
            with open(fname) as f:
                data = json.load(f)
                if key:
                    return data.get(key, default)
                else:
                    return data
        else:
            return default


    UPGRADE_STATUS_JSON = "/lib/platform-config/current/upgrade.json"

    def update_upgrade_status(self, key, value):
        data = {}
        if os.path.exists(self.UPGRADE_STATUS_JSON):
            with open(self.UPGRADE_STATUS_JSON) as f:
                data = json.load(f)
        data[key] = value
        with open(self.UPGRADE_STATUS_JSON, "w") as f:
            json.dump(data, f)


    #
    # Initialize self.current_version, self.next_Version
    # and anything required for summarize()
    #
    def init_versions(self):
        raise Exception("init_versions() must be provided by the deriving class.")


    #
    # Perform actual upgrade. Provided by derived class.
    #
    def do_upgrade(self, forced=False):
        raise Exception("do_upgrade() must be provided by the deriving class.")

    #
    # Perform any clean up necessary if the system is current (no upgrade required.)
    #
    def do_no_upgrade(self):
        pass


    def init_upgrade(self):
        self.current_version = None
        self.next_version = None
        self.init_versions()
        self.update_upgrade_status(self.current_version_key, self.current_version)
        self.update_upgrade_status(self.next_version_key, self.next_version)

    def summarize(self):
        pass


    def __upgrade_prompt(self, instructions, default="yes"):
        valid = {"yes": True, "y": True, "ye": True,
                 "no": False, "n": False}
        if default is None:
            prompt = " [y/n] "
        elif default == "yes":
            prompt = " [Y/n] "
        elif default == "no":
            prompt = " [y/N] "
        else:
            raise ValueError("invalid default answer: '%s'" % default)

        notes = self.upgrade_notes()
        if notes:
            instructions = instructions + "\n" + notes + "\n"

        instructions = "\n\n" + instructions + "\n\nStart the upgrade? "

        while True:
            sys.stdout.write(instructions + prompt)
            choice = raw_input().lower()
            if default is not None and choice == '':
                return valid[default]
            elif choice in valid:
                return valid[choice]
            else:
                sys.stdout.write("Please respond with 'yes' or 'no' "
                                 "(or 'y' or 'n').\n")

    def upgrade_prompt(self, instructions, default='yes'):
        try:
            return self.__upgrade_prompt(instructions, default)
        except Exception, e:
            self.logger.error("")
            self.logger.error("Exception: %s" % e)
            self.abort("No upgrade will be performed.")
            return False
        except KeyboardInterrupt:
            return False

    def upgrade_notes(self):
        raise Exception("Must be provided by derived class.")

    def __upgrade_advisory(self):
        if not self.ops.enable:
            self.finish("%s updates are not enabled." % self.Name)
        self.__do_upgrade()
        self.finish()

    def __upgrade_force(self):
        self.logger.info("This system is configured for automatic %s updates." % self.Name)
        self.__do_upgrade()
        self.finish()

    def __do_upgrade(self):
        if self.ops.check:
            self.finish("[ Check abort. ]")
        self.do_upgrade()


    def __upgrade_optional(self):
        instructions = """%s upgrade should be performed before continuing for optimal
performance on this system.

While it is recommended that you perform this upgrade it is optional and you
may continue booting for testing purposes at this time.

Please note that you will be asked again each time at boot to perform
this upgrade. Automatic booting will not be possible until the upgrade
is performed.""" % self.atype

        if self.upgrade_prompt(instructions) == True:
            self.__do_upgrade()
            self.finish()
        else:
            self.finish("Upgrade cancelled.")

    def __upgrade_required(self):
        instructions = """%s upgrade must be performed before the software can run on this system.
If you choose not to perform this upgrade booting cannot continue.""" % self.atype

        if self.upgrade_prompt(instructions) == True:
            self.__do_upgrade()
            self.finish()
        else:
            self.logger.info("The system cannot continue without performing %s update. The system will now reboot if you would like to take alternative actions." % self.atype)
            self.reboot()


    def __upgrade(self):
        self.logger.info("%s upgrade is required." % self.atype)

        auto_upgrade = self.ops.auto_upgrade

        #
        # auto-upgrade modes are:
        #   advisory : Advisory only. This is the default.
        #   force    : Perform the upgrade automatically.
        #   optional : Ask the user whether an upgrade should be performed.
        #   required : Ask the user whether an upgrade should be performed.
        #             Reboot if the answer is 'no'.
        #
        if auto_upgrade == 'advisory':
            self.__upgrade_advisory()
        elif auto_upgrade == 'force':
            self.__upgrade_force();
        elif auto_upgrade == 'optional':
            self.__upgrade_optional()
        elif auto_upgrade == 'required':
            self.__upgrade_required()
        else:
            self.abort("auto-upgrade mode '%s' is not supported." % auto_upgrade)


    def upgrade_check(self):

        if self.current_version == self.next_version:
            # Versions match. Only continue if forced.
            if self.ops.force:
                self.logger.info("%s version %s is current." % (self.Name,
                                                                self.current_version))
                self.logger.info("[ Upgrade Forced. ]")
            else:
                self.logger.info("%s version %s is current." % (self.Name,
                                                                self.current_version))
                self.do_no_upgrade()
                self.finish()

        if self.next_version:
            self.__upgrade()


    def mount(self, location, partition=None, label=None):
        if not os.path.isdir(location):
            os.makedirs(location)

        if partition:
            cmd = "mount %s %s " % (partition,location)
            name = partition

        if label:
            cmd = "mount LABEL=%s %s" % (label, location)
            name = label

        if os.system(cmd) != 0:
            self.abort("Could not mount %s @ %s. Upgrade cannot continue." % (name, location))

    def umount(self, location):
        if os.system("umount %s" % location) != 0:
            self.abort("Could not unmount %s. Upgrade cannot continue." % location)


    def main(self):
        self.ops = self.ap.parse_args()
        self.banner()
        self.init_upgrade()
        self.summarize()
        if not self.ops.summarize:
            self.upgrade_check()




class BaseOnieUpgrade(BaseUpgrade):

    ONIE_UPDATER_PATH = "/mnt/flash2/onie-updater"

    def install_onie_updater(self, src_dir, updater):
        if type(updater) is list:
            # Copy all files in the list to /mnt/flash2
            for f in updater:
                src = os.path.join(src_dir, f)
                dst = os.path.join("/mnt/flash2", f)
                self.copyfile(src, dst)
        else:
            # Copy single updater to /mnt/flash2/onie-updater
            src = os.path.join(src_dir, updater)
            self.copyfile(src, self.ONIE_UPDATER_PATH)


    def initiate_onie_update(self):
        self.logger.info("Initiating %s Update." % self.Name)
        if self.arch == 'ppc':
            # Initiate update
            self.fw_setenv('onie_boot_reason', 'update')
            self.reboot()

        elif self.arch == 'x86_64':
            OB = "/mnt/onie-boot"
            self.mount(OB, label="ONIE-BOOT")
            if os.system("/mnt/onie-boot/onie/tools/bin/onie-boot-mode -o update") != 0:
                self.abort("Could not set ONIE Boot Mode to Update. Upgrade cannot continue.")
            self.umount(OB)

            SL = "/mnt/sl-boot"
            self.mount(SL, label="SL-BOOT")
            with open("/mnt/sl-boot/grub/grub.cfg", "a") as f:
                f.write("set default=ONIE\n")
            self.umount(SL)
            self.reboot()

        else:
            self.abort("Architecture %s unhandled." % self.arch)

    def clean_onie_updater(self):
        if os.path.exists(self.ONIE_UPDATER_PATH):
            self.logger.info("Removing previous onie-updater.")
            os.remove(self.ONIE_UPDATER_PATH)
