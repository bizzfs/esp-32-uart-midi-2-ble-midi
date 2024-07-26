set(srcs "main.c" "lib/src/gatt.c" "lib/src/ble.c" "lib/src/parser.c" "lib/src/uuids.c" "lib/src/uart.c" "lib/src/transmitter.c" "lib/src/processor.c")

idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "." "lib/include")
