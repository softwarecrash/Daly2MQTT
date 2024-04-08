Import("env")

env.Append(CPPDEFINES=[
    ("SWVERSION", env.StringifyMacro(env.GetProjectOption("custom_prog_version"))),
    ("HWBOARD", env.StringifyMacro(env["BOARD"])),
])

env.Replace(PROGNAME="Daly2MQTT_%s_%s" % (str(env["BOARD"]), env.GetProjectOption("custom_prog_version")))
