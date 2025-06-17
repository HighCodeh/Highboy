
# Consider dependencies only in project.
set(CMAKE_DEPENDS_IN_PROJECT_ONLY OFF)

# The set of languages for which implicit dependencies are needed:
set(CMAKE_DEPENDS_LANGUAGES
  "ASM"
  )
# The set of files for implicit dependencies of each language:
set(CMAKE_DEPENDS_CHECK_ASM
  "/home/vinibit/esp-idf-v5.1.2/components/esp_rom/patches/esp_rom_cache_writeback_esp32s3.S" "/home/vinibit/Projetos/Highboy-main/build/bootloader/esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_cache_writeback_esp32s3.S.obj"
  "/home/vinibit/esp-idf-v5.1.2/components/esp_rom/patches/esp_rom_longjmp.S" "/home/vinibit/Projetos/Highboy-main/build/bootloader/esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_longjmp.S.obj"
  )
set(CMAKE_ASM_COMPILER_ID "GNU")

# Preprocessor definitions for this target.
set(CMAKE_TARGET_DEFINITIONS_ASM
  "BOOTLOADER_BUILD=1"
  "ESP_PLATFORM"
  "IDF_VER=\"v5.1.2\""
  "SOC_MMU_PAGE_SIZE=CONFIG_MMU_PAGE_SIZE"
  "_GNU_SOURCE"
  )

# The include file search paths:
set(CMAKE_ASM_TARGET_INCLUDE_PATH
  "config"
  "/home/vinibit/esp-idf-v5.1.2/components/esp_rom/include"
  "/home/vinibit/esp-idf-v5.1.2/components/esp_rom/include/esp32s3"
  "/home/vinibit/esp-idf-v5.1.2/components/esp_rom/esp32s3"
  "/home/vinibit/esp-idf-v5.1.2/components/log/include"
  "/home/vinibit/esp-idf-v5.1.2/components/esp_common/include"
  "/home/vinibit/esp-idf-v5.1.2/components/esp_hw_support/include"
  "/home/vinibit/esp-idf-v5.1.2/components/esp_hw_support/include/soc"
  "/home/vinibit/esp-idf-v5.1.2/components/esp_hw_support/include/soc/esp32s3"
  "/home/vinibit/esp-idf-v5.1.2/components/esp_hw_support/port/esp32s3/."
  "/home/vinibit/esp-idf-v5.1.2/components/esp_hw_support/port/esp32s3/private_include"
  "/home/vinibit/esp-idf-v5.1.2/components/newlib/platform_include"
  "/home/vinibit/esp-idf-v5.1.2/components/xtensa/include"
  "/home/vinibit/esp-idf-v5.1.2/components/xtensa/esp32s3/include"
  "/home/vinibit/esp-idf-v5.1.2/components/soc/include"
  "/home/vinibit/esp-idf-v5.1.2/components/soc/esp32s3"
  "/home/vinibit/esp-idf-v5.1.2/components/soc/esp32s3/include"
  "/home/vinibit/esp-idf-v5.1.2/components/hal/esp32s3/include"
  "/home/vinibit/esp-idf-v5.1.2/components/hal/include"
  "/home/vinibit/esp-idf-v5.1.2/components/hal/platform_port/include"
  )

# The set of dependency files which are needed:
set(CMAKE_DEPENDS_DEPENDENCY_FILES
  "/home/vinibit/esp-idf-v5.1.2/components/esp_rom/patches/esp_rom_cache_esp32s2_esp32s3.c" "esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_cache_esp32s2_esp32s3.c.obj" "gcc" "esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_cache_esp32s2_esp32s3.c.obj.d"
  "/home/vinibit/esp-idf-v5.1.2/components/esp_rom/patches/esp_rom_crc.c" "esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_crc.c.obj" "gcc" "esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_crc.c.obj.d"
  "/home/vinibit/esp-idf-v5.1.2/components/esp_rom/patches/esp_rom_efuse.c" "esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_efuse.c.obj" "gcc" "esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_efuse.c.obj.d"
  "/home/vinibit/esp-idf-v5.1.2/components/esp_rom/patches/esp_rom_spiflash.c" "esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_spiflash.c.obj" "gcc" "esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_spiflash.c.obj.d"
  "/home/vinibit/esp-idf-v5.1.2/components/esp_rom/patches/esp_rom_sys.c" "esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_sys.c.obj" "gcc" "esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_sys.c.obj.d"
  "/home/vinibit/esp-idf-v5.1.2/components/esp_rom/patches/esp_rom_systimer.c" "esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_systimer.c.obj" "gcc" "esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_systimer.c.obj.d"
  "/home/vinibit/esp-idf-v5.1.2/components/esp_rom/patches/esp_rom_uart.c" "esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_uart.c.obj" "gcc" "esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_uart.c.obj.d"
  "/home/vinibit/esp-idf-v5.1.2/components/esp_rom/patches/esp_rom_wdt.c" "esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_wdt.c.obj" "gcc" "esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_wdt.c.obj.d"
  )

# Targets to which this target links which contain Fortran sources.
set(CMAKE_Fortran_TARGET_LINKED_INFO_FILES
  )

# Targets to which this target links which contain Fortran sources.
set(CMAKE_Fortran_TARGET_FORWARD_LINKED_INFO_FILES
  )

# Fortran module output directory.
set(CMAKE_Fortran_TARGET_MODULE_DIR "")
