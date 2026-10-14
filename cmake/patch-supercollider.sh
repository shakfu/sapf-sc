#!/bin/sh
# Lets SuperCollider's source build as a subdirectory: its CMake files locate sources from
# CMAKE_SOURCE_DIR, which is then sapf-sc's root, so they use their own PROJECT_SOURCE_DIR; and
# sclang, the platform files and the editors are left out. Run in SC's source folder. Idempotent.
set -e
find . \( -name 'CMakeLists.txt' -o -name '*.cmake' \) -exec sed -i.bak 's|\${CMAKE_SOURCE_DIR}|\${PROJECT_SOURCE_DIR}|g' {} +
sed -i.bak -e '/^add_subdirectory(lang)$/d' -e '/^add_subdirectory(platform)$/d' -e '/^add_subdirectory(editors)$/d' CMakeLists.txt
find . -name '*.bak' -delete
