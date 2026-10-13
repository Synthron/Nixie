# PlatformIO extra-script: erase the entire flash before uploading.
# Used by the [env:esp32dev-fresh] environment to simulate a fresh device
# (empty SPIFFS -> AP-mode WiFi manager) for testing.

Import("env")


def erase_flash_before_upload(source, target, env):
    print(">>> Erasing entire flash before upload (fresh-device test) <<<")
    esptool = env.PioPlatform().get_package_dir("tool-esptoolpy") + "/esptool.py"
    chip = env.BoardConfig().get("build.mcu", "esp32")
    port = env.subst("$UPLOAD_PORT")
    py = env.subst("$PYTHONEXE")

    print("esptool : %s" % esptool)
    print("chip    : %s" % chip)
    print("port    : %s" % port)

    if not port or "*" in port:
        raise SystemExit("ERROR: upload port not resolved: '%s'" % port)

    cmd = '"%s" "%s" --chip %s --port %s erase_flash' % (py, esptool, chip, port)
    print("running: %s" % cmd)
    rc = env.Execute(cmd)
    if rc:
        raise SystemExit("ERROR: flash erase failed (rc=%s)" % rc)


env.AddPreAction("upload", erase_flash_before_upload)
