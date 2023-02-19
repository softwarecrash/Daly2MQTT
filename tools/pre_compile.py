Import("env")

env.Append(CPPDEFINES=[
    ("SWVERSION", env.StringifyMacro(env.GetProjectOption("custom_prog_version"))),
])

env.Replace(PROGNAME="Daly-BMS-to-MQTT_%s_%s" % (str(env["BOARD"]), env.GetProjectOption("custom_prog_version")))
