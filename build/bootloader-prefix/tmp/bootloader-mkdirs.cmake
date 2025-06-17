# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/vinibit/esp-idf-v5.1.2/components/bootloader/subproject"
  "/home/vinibit/Projetos/Highboy-main/build/bootloader"
  "/home/vinibit/Projetos/Highboy-main/build/bootloader-prefix"
  "/home/vinibit/Projetos/Highboy-main/build/bootloader-prefix/tmp"
  "/home/vinibit/Projetos/Highboy-main/build/bootloader-prefix/src/bootloader-stamp"
  "/home/vinibit/Projetos/Highboy-main/build/bootloader-prefix/src"
  "/home/vinibit/Projetos/Highboy-main/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/vinibit/Projetos/Highboy-main/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/vinibit/Projetos/Highboy-main/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
