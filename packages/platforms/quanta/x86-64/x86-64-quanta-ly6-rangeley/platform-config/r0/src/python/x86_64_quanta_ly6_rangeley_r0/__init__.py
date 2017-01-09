from onl.platform.base import *
from onl.platform.quanta import *

class OnlPlatform_x86_64_quanta_ly6_rangeley_r0(OnlPlatformQuanta,
                                                OnlPlatformPortConfig_32x40):
    PLATFORM='x86-64-quanta-ly6-rangeley-r0'
    MODEL='LY6'
    SYS_OBJECT_ID='.6.1'

    def baseconfig(self):
        self.insmod("emerson700")
        self.insmod("quanta_hwmon")
        self.insmod("quanta-ly6-i2c-mux")
        self.insmod("quanta_switch", params=dict(platform="x86-64-quanta-ly6-rangeley"))

        # fixme
        try:
            files = os.listdir("%s/etc/init.d" % self.basedir_onl())
            for file in files:
                src = "%s/etc/init.d/%s" % (self.basedir_onl(), file)
                dst = "/etc/init.d/%s" % file
                os.system("cp -f %s %s" % (src, dst))
                os.system("/usr/sbin/update-rc.d %s defaults" % file)
        except:
            pass

        # make ds1339 as default rtc
        os.system("ln -snf /dev/rtc1 /dev/rtc")
        os.system("hwclock --hctosys")

        # fixme
        # set system led to green
        sled_check = self.basedir_onl('sbin', 'systemled')
        if os.path.exists(sled_check):
            sled = self.basedir_onl('sbin', 'systemled green')
            os.system(sled)

        return True
