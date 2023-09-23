set(COMPONENT_REQUIRES "m5stack" "m5unit")
idf_component_register(SRCS "main.c" "wifi.c" REQUIRES "driver" INCLUDE_DIRS "includes")