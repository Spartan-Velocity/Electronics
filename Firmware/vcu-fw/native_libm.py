# Link libm for the host test build. expf() lives there on glibc, and the
# library has to come after the objects on the link line, which build_flags
# cannot guarantee.
Import("env")
env.Append(LIBS=["m"])
