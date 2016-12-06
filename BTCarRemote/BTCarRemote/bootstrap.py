from os import system
android = 1
if android:
    vm_address = "192.168.1.108"
    appname = "BTCarRemote-0.1-debug.apk"
    system("plink -ssh -pw p4a pszczelaszkov@"+vm_address + " \"cd /media/sf_BTCarRemote/ && p4a apk\"")
    system("adb install -rs " + appname)
    system("adb shell am start -n org.pszczelaszkov.btcar/org.kivy.android.PythonActivity")
    system("adb logcat -s python")
else:
    system("main.py")
